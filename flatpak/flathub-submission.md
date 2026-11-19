Adds **Flip** (`io.github.kjlape.Flip`), a frame-by-frame animation app: draw
on frames with an onion skin of the frames around them, play them back, and
export a looping GIF or an MP4. Source: https://github.com/kjlape/flip

I'm the developer of Flip, and the app ID's GitHub account is mine. I built
and ran this manifest locally with `flatpak-builder`, and
`flatpak-builder-lint` passes for the manifest and the build.

Notes for review:

- **ffmpeg and x264 are built in the manifest.** Exports run the `ffmpeg`
  command, which the runtimes don't ship, and MP4 export needs the x264
  encoder, which the runtime's codecs don't include. ffmpeg is built with
  `--disable-autodetect`, without docs, ffplay, ffprobe, or networking.
- **Two read-only filesystem permissions**, `~/.local/state/omarchy:ro` and
  `~/.config/omarchy:ro`. On the Omarchy desktop, Flip follows its live
  accent color and interface language from these. Elsewhere they don't
  exist; Flip then uses its default color and the system language. It
  writes nothing there.
- Files are opened, saved, and exported through the FileChooser portal, so
  there's no other filesystem access.
