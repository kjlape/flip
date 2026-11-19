# Publishing Flip

Flip is published from `github.com/kjlape/flip` as a Flatpak, so it installs
on any Linux distribution, not only Omarchy. There are three channels:

| Channel | What people get | Scripted |
| --- | --- | --- |
| **GitHub releases** | A single `flip-<version>.flatpak` file to download and install | Yes: `bin/release <version> --publish` |
| **Flatpak repository on GitHub Pages** | One install command, then updates through `flatpak update` and the software center | Yes: `bin/flatpak-repo --publish` |
| **Flathub** | Listing in GNOME Software, KDE Discover, and flathub.org | **No**, by Flathub's rules; see [Flathub](#flathub) |

For Omarchy itself, `bin/install` builds and installs the Arch package as
before; none of this changes that.

## What each script does

| Script | Leaves the machine? | Does |
| --- | --- | --- |
| `bin/release <version>` | No | Sets the version everywhere, adds AppStream release notes, runs the tests, commits, and tags `v<version>` |
| `bin/release <version> --publish` | Yes, after asking | Also pushes the branch and tag, builds `flip-<version>.flatpak`, and makes a GitHub release with it attached |
| `bin/flatpak` | No | Builds the Flatpak from this checkout, installs it for you, and writes `flip.flatpak` |
| `bin/flatpak-repo` | No | Adds the checked-out release to a signed Flatpak repository in `build/pages` |
| `bin/flatpak-repo --publish` | Yes, after asking | Also force-pushes that repository to the `gh-pages` branch and turns on GitHub Pages |

Anything that publishes asks first. `FLIP_YES=1` answers yes for you.

## One-time setup

### 1. Install the tools

On Omarchy or Arch:

```sh
sudo pacman -S --needed github-cli flatpak ostree gnupg python appstream
```

The scripts install the rest (the KDE 6.11 runtime and SDK, and
`org.flatpak.Builder`) into your user Flatpak installation the first time.
That's a download of a couple of gigabytes.

### 2. Check that git can push

Your global git hooks (`~/.config/git/hooks`) include Git LFS hooks, but
`git-lfs` isn't installed, so **every `git push` currently fails** with:

```text
This repository is configured for Git LFS but 'git-lfs' was not found on your path.
```

Pick one:

- If you use LFS in other repositories, install it: `sudo pacman -S git-lfs`,
  then `git lfs install`.
- If you don't, remove the LFS hooks. Look first; each one mentions `git lfs`:

  ```sh
  grep -l "git lfs" ~/.config/git/hooks/*
  ```

  Then delete the files it lists (`pre-push`, `post-checkout`,
  `post-commit`, and `post-merge`).

The other hooks print the same warning on commits and checkouts but don't
stop them. The scripts' own bookkeeping on `gh-pages` doesn't run hooks at
all, so it isn't affected.

### 3. Log in to GitHub

```sh
gh auth login      # GitHub.com, HTTPS, and yes to authenticating git
gh auth setup-git  # lets git push with the same login
```

### 4. Look over what becomes public

Everything in the repository's history becomes public, including:

- The name and email on every commit (`git log --format='%an <%ae>' | sort -u`).
  To use GitHub's private no-reply address instead, rewrite the history
  before the first push.
- `LICENSE`, `README.md`, and `docs/screenshot.png`. The screenshot shows
  made-up content.
- The app was written with an AI assistant (Claude). Nothing requires you to
  say so on GitHub, but it's good practice, and Flathub does require it (see
  [Flathub](#flathub)). A line in the README covers it.

### 5. Create the GitHub repository

```sh
cd ~/Work/flip
gh repo create kjlape/flip --public --source=. --remote=origin \
  --description "Draw frame by frame and play it back as an animation" --push
gh repo edit kjlape/flip --homepage https://kjlape.github.io/flip \
  --add-topic flatpak --add-topic animation --add-topic qt --add-topic omarchy
```

Keep Issues turned on. The app's AppStream metadata links to them as its bug
tracker.

### 6. Make a signing key for the Flatpak repository

Flatpak only installs from a repository it can verify, so the repository on
GitHub Pages is signed. Use a key just for this, not your personal key:

```sh
gpg --quick-gen-key "Flip Flatpak repository" rsa4096 sign never
gpg --list-secret-keys --with-colons "Flip Flatpak repository" | awk -F: '/^fpr/{print $10; exit}'
```

The second command prints the key's fingerprint. Put it in your shell
profile so the scripts can find it:

```sh
export FLIP_GPG_KEY=<the fingerprint>
```

A passphrase is fine. gpg asks for it while publishing.

Back the key up somewhere safe and offline, such as a password manager:

```sh
gpg --export-secret-keys --armor "$FLIP_GPG_KEY" > flip-repository-key.asc
```

Never commit it. If you lose it, nobody gets updates until they add the
repository again with a new key. If it leaks, someone else could sign updates
that installs would trust. Either way, see [Changing the signing
key](#changing-the-signing-key).

## Cutting a release

### 1. Make the release locally

```sh
bin/release 0.2.0
```

This changes nothing outside your checkout. It:

1. Refuses to run with uncommitted changes, or if `v0.2.0` exists on another commit.
2. Sets the version in `flip.pro` (what `flip --version` prints), the
   `PKGBUILD`, and the AppStream metadata's screenshot URL, which points at
   the tag so the screenshot always matches the release.
3. Adds a `<release>` entry to `flatpak/io.github.kjlape.Flip.metainfo.xml`.
   Its notes are the commit subjects since the last tag. Software centers show
   them as "What's new."
4. Runs the tests and validates the AppStream metadata. If either fails, it
   puts every file back and stops.
5. Commits `Release v0.2.0` and makes an annotated tag `v0.2.0`.

**Writing the notes yourself.** Commit subjects make terse release notes. To
write them by hand, add the `<release version="0.2.0" date="…">` entry to the
metainfo yourself and commit it before running `bin/release`. The script sees
the entry, only updates its date, and keeps your text.

**Look before you publish:** `git show` and `build/flip --version` after `bin/build`.

**Undo a release you haven't published:**

```sh
git tag -d v0.2.0
git reset --hard HEAD~1   # only if bin/release made a "Release v0.2.0" commit
```

**Version numbers** are `major.minor.patch`, and each must be higher than the
last. Flatpak and software centers compare them. The first release is
`0.1.0`, which the project already says, so `bin/release 0.1.0` only tags it.

### 2. Publish it on GitHub

```sh
bin/release 0.2.0 --publish
```

After you confirm, it pushes the branch and the tag, builds the Flatpak
(`bin/flatpak`), and makes the GitHub release
`https://github.com/kjlape/flip/releases/tag/v0.2.0` with:

- `flip-0.2.0.flatpak` attached
- the commit subjects as notes
- install instructions

If it stops partway, say the network drops, run the same command again. It
sees the tag already exists on this commit and picks up from publishing.

To edit the notes afterwards, use the release page on GitHub, or
`gh release edit v0.2.0 --notes-file <file>`.

## GitHub releases

People with Flatpak (Fedora, Mint, Pop!_OS, and most others have it) install
the downloaded file with:

```sh
flatpak install --user flip-0.2.0.flatpak
flatpak run io.github.kjlape.Flip
```

Flatpak fetches the KDE runtime from Flathub the first time. That's a large
one-off download, shared with every other KDE app.

Single files don't update themselves. People install the next file over the
old one. Point them at the Flatpak repository for updates.

## Flatpak repository on GitHub Pages

This is the channel to recommend: one command to install, then updates like
any other Flatpak app.

### Publish a release to it

Right after `bin/release <version> --publish`, while the release's tag is
still checked out:

```sh
bin/flatpak-repo            # builds it into build/pages; nothing leaves the machine
bin/flatpak-repo --publish  # pushes it to GitHub Pages
```

The script refuses to run unless the current commit has a release tag and
there are no uncommitted changes. To publish an older release, check its tag
out first (`git checkout v0.2.0`).

Before publishing, you can look at `build/pages/index.html`. That's the page
people will see at `https://kjlape.github.io/flip`.

The first `--publish` turns GitHub Pages on for the `gh-pages` branch. The
site goes live a minute or two later. Watch "pages build and deployment"
under the repository's Actions tab. After that, every publish redeploys it.

### Install it

What to tell people:

```sh
flatpak install --user https://kjlape.github.io/flip/io.github.kjlape.Flip.flatpakref
```

Or, to add the repository first and then install from it:

```sh
flatpak remote-add --user --if-not-exists flip https://kjlape.github.io/flip/flip.flatpakrepo
flatpak install --user flip io.github.kjlape.Flip
```

New releases then arrive with `flatpak update`, and in GNOME Software or KDE
Discover with everything else.

**Check the first publish yourself** on a machine or VM that has never had
Flip: run the install command above, then `flatpak run io.github.kjlape.Flip`.

### How it works

- `build/pages` is a checkout of the `gh-pages` branch, which holds:
  - `repo/`, an OSTree repository. That's the format Flatpak installs from.
  - `flip.flatpakrepo` and `io.github.kjlape.Flip.flatpakref`, which carry
    the repository's URL and the public key.
  - `flip.gpg`, the public key.
  - an `index.html` with install instructions.
- Each run starts from what's already on GitHub, adds the new build as a
  signed commit, makes deltas so updates download only what changed, and
  keeps the last three releases.
- `gh-pages` is force-pushed as a single commit each time. Release history
  lives inside the OSTree repository, so the branch doesn't grow forever.
- Each release adds about 15 MB, well inside GitHub Pages' limits (1 GB per
  site, 100 GB of traffic a month).
- Builds are for x86_64 only. ARM machines can't install Flip yet.

### Changing the signing key

If you have to replace the key:
1. Make the new one ([step 6](#6-make-a-signing-key-for-the-flatpak-repository)).
2. Set `FLIP_GPG_KEY` to it.
3. Publish a release.

Existing installs trust only the old key, so they stop getting updates.
People fix that by re-adding the repository:

```sh
flatpak remote-modify --user --gpg-import=<(curl -sL https://kjlape.github.io/flip/flip.gpg | gpg --dearmor) flip
```

Say so in the release notes. If the old key leaked, also say not to trust
updates signed before the switch.

## Flathub

**Flathub's rules don't allow an AI assistant to do this part, so there's no
script for it.** Flathub's [Generative AI
policy](https://docs.flathub.org/docs/for-app-authors/requirements#generative-ai-policy)
and the `AGENTS.md` in `github.com/flathub/flathub` say that AI tools must
not:

- generate or edit Flathub manifests, even with disclosure
- open or automate submission pull requests, or write their descriptions,
  commit messages, comments, or replies to reviewers

They also require you to disclose AI-generated material in the app and its
packaging, naming the affected parts and roughly how much. For Flip, that's
most of it:
- the C++ and QML source
- the tests
- the scripts
- the documentation
- the AppStream metadata, desktop file, and icon
- `flatpak/io.github.kjlape.Flip.yml`

That last file is this repository's own manifest for the builds above. It
can't be what you submit to Flathub. Their manifest has to be written by you.

Flathub also requires *"a meaningful development history, evidence of
real-world use, and a clear commitment to ongoing maintenance."* A project
that's days old doesn't have that yet. The two channels above are the way to
build it: releases, users, and issues on GitHub.

When Flip is ready, the process belongs to you, done by hand:
1. Read Flathub's [requirements](https://docs.flathub.org/docs/for-app-authors/requirements)
   and [submission guide](https://docs.flathub.org/docs/for-app-authors/submission).
2. Write the manifest.
3. Open the pull request yourself, following the template's checklist,
   including the AI disclosure.
4. Answer the reviewers yourself.

Don't ask Claude or any other AI assistant to write or edit the manifest, the
pull request, or your replies. Reviewers may reject a submission for that,
and repeat violations can get an account banned from Flathub.

## Troubleshooting

| Problem | Fix |
| --- | --- |
| A push fails with "git-lfs was not found" | [Check that git can push](#2-check-that-git-can-push). |
| `this commit isn't a release` | Run `bin/release <version>` first, or `git checkout v<version>`. |
| `commit or stash your changes first` | Releases are built from committed code only; commit or `git stash`. |
| `gpg has no secret key` | `FLIP_GPG_KEY` is wrong, or `GNUPGHOME` points at another keyring. |
| `the tests or the AppStream check failed` | Run `bin/test`, and `appstreamcli validate --no-net flatpak/io.github.kjlape.Flip.metainfo.xml`. |
| `kjlape.github.io/flip` shows 404 | Give the first deploy a few minutes. Then check Settings → Pages: source "Deploy from a branch", `gh-pages`, `/ (root)`. |
| Someone gets "Can't check signature: public key not found" | Their remote has a different key than the repository. They re-add it from `flip.flatpakrepo`, or you published with the wrong `FLIP_GPG_KEY`. |
| `flatpak-builder` can't find `org.kde.Sdk` | The scripts point the sandboxed builder at your user installation. If you've set `FLATPAK_USER_DIR` yourself, unset it. |

## Rehearsing without GitHub

You can run the whole flow against stand-ins without publishing anything:

```sh
git clone --bare ~/Work/flip /tmp/flip-origin.git
git clone /tmp/flip-origin.git /tmp/flip-rehearsal && cd /tmp/flip-rehearsal
bin/release 0.1.0
FLIP_PAGES_URL=http://127.0.0.1:8765 bin/flatpak-repo
(cd build/pages && python3 -m http.server 8765 --bind 127.0.0.1) &
FLATPAK_USER_DIR=/tmp/flip-client flatpak remote-add --user test http://127.0.0.1:8765/flip.flatpakrepo
FLATPAK_USER_DIR=/tmp/flip-client flatpak install --user --no-deps test io.github.kjlape.Flip
```

Set `FLATPAK_USER_DIR` only on the client commands, as above. Set for the
whole session, the build installs the multi-gigabyte SDK into that directory
as well.

## Settings

| Variable | Used by | Meaning |
| --- | --- | --- |
| `FLIP_GPG_KEY` | `bin/flatpak-repo` | Fingerprint of the key that signs the repository (required) |
| `FLIP_YES` | `bin/release`, `bin/flatpak-repo` | `1` answers yes to every "publish?" question |
| `FLIP_PAGES_URL` | `bin/flatpak-repo` | Where the repository is served; defaults to `https://kjlape.github.io/flip` |
| `FLIP_STATE_DIR` | `bin/flatpak` | flatpak-builder's cache; defaults to `.flatpak-builder` in the checkout |
| `GNUPGHOME` | gpg | Which keyring holds `FLIP_GPG_KEY`, as usual |
