---
name: flip
description: Make and export frame-by-frame animations with Flip, from the command line. Use when asked to create a flipbook or .flip animation, turn a series of images into an animation, inspect a .flip file, pull frames out of one, or export one as a GIF or MP4.
---

# Flip

Flip is a desktop app for drawing animations frame by frame. A `.flip` file
holds equally sized transparent frames over one paper color, plus a speed in
frames per second. These commands need no display; each exits 0 on success
and 1 on failure, with the error on stderr, and takes `--json` for output a
program can read.

| Command | Does |
| --- | --- |
| `flip info <animation>` | Frames, fps, size, and length in seconds |
| `flip import <animation.flip> <image>... [--fps N] [--force]` | One frame per image, in the order given; the first image sets the size and the rest are fitted inside it |
| `flip frames <animation> <directory> [--paper]` | Every frame as `0001.png`, `0002.png`, ...; transparent unless `--paper` |
| `flip export <animation> <output.gif\|output.mp4> [--fps N]` | A looping GIF or an H.264 MP4, through ffmpeg |
| `flip open [animation]` | Opens the editor window for a person |

To change an animation: `flip frames` it to a directory, edit or add PNGs,
then `flip import` them into a new `.flip` file. Import refuses to replace an
existing file unless you pass `--force`; never replace the user's own
animation without asking. Speeds run from 1 to 60 fps; 12 is the default and
reads as hand-drawn animation.
