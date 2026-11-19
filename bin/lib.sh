# Shared by bin/flatpak, bin/release, and bin/flathub. Sourced, not run.

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
APP=io.github.kjlape.Flip
GITHUB_REPO=kjlape/flip
# Overridable so the scripts can be rehearsed against a local bare repo.
GIT_URL="${FLIP_GIT_URL:-https://github.com/$GITHUB_REPO.git}"
FLATHUB_RUNTIME=org.kde.Platform//6.11
FLATHUB_SDK=org.kde.Sdk//6.11

die() {
  echo "error: $*" >&2
  exit 1
}

# Asks before anything other people will see. FLIP_YES=1 answers yes.
confirm() {
  [ "${FLIP_YES:-}" = 1 ] && return 0
  printf '%s [y/N] ' "$1"
  read -r answer
  case "$answer" in y | Y | yes) return 0 ;; *) die "stopped" ;; esac
}

need() {
  command -v "$1" >/dev/null || die "$1 is not installed. $2"
}

# Sets BUILDER and LINT. The Flathub runtime and SDK come from the host's
# flatpak, which can always see Flathub; the sandboxed builder sometimes can't.
setup_builder() {
  need flatpak "Install it from your distribution's packages."
  flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
  flatpak install --user -y --noninteractive flathub "$FLATHUB_RUNTIME" "$FLATHUB_SDK" org.flatpak.Builder
  # In its sandbox "user" means its own data directory; point it at ours.
  user_dir="${XDG_DATA_HOME:-$HOME/.local/share}/flatpak"
  BUILDER="flatpak run --env=FLATPAK_USER_DIR=$user_dir --command=flatpak-builder org.flatpak.Builder"
  LINT="flatpak run --command=flatpak-builder-lint org.flatpak.Builder"
}

# The version in flip.pro, which the binary reports.
current_version() {
  sed -n 's/.*FLIP_VERSION=\\\\\\"\([0-9.]*\)\\\\\\".*/\1/p' "$ROOT/flip.pro"
}
