# Lejuice 🍉

Lejuice it's a rename-driven tool that watches your filesystem and does... well, whatever your terminal is able to! Meant to be local, lightweight, modular and extensible. Did I mention fast?

You can convert photos, extract audio, turn notes into slides or compile code into an executable. All by renaming the file:

```text
photo.webp    →   photo.png
music.mp4     →   music.mp3
my_notes.md   →   my_notes.pptx
main.c        →   main.exe
```
<sub>(Yes, you'll finally be able to convert a png into a jpeg. Revolutionary.)</sup>

The limits are somewhere between your imagination and what's not implemented yet -- yet!

## What does it do?

Lejuice doesn't come with built-in converters because it doesn't need to. Instead, it connects your File Explorer's rename action directly to the CLI tools you are already familiar (`ffmpeg`, `magick`, `pandoc`, `gcc`, or your own scripts) through a `.ini` file.

You can also send arguments directly through the name by adding {...} right before the extension. Plus, you can use macros to make your life easier!

Here a simple example:

```ini
[rules]
png -> jpg : magick "${IN}" ${ARGS} "${OUT}"

[macros.magick]
gray   = -colorspace Gray
{VAR}q = -quality {VAR}
```

Then you can rename `vacation.png` to `vacation{gray, 85q}.jpg`.

Lejuice will turn this into `magick "vacation.png" -colorspace Gray -quality 85 "vacation.jpg"`.

And... *voilà!* You have your file.

## Your files, your rules

I know what you may be thinking.
"Okay, a background tool that isn't supposed to stop running. Are you sure it won't explode my files?"

Am I, this beast is safe by design. Here's why:
- **It never deletes any file anywhere in the code.** After a conversion, the original file is either kept (keep) or sent safely to your system's Recycle Bin/Trash (trash).
- **It won't overwrite accidentally.** If `photo.jpg` already exists in the folder, your new file automatically becomes `photo (1).jpg`.
- **It has a safe fail recovery.** If something goes wrong mid-way, Lejuice restores your files back to its original name and stops there.
- **There's a custom bypass!** In case you want to rename without triggering the program, just start the name with the escape prefix (`!` by default).

---
*Made with love. Powered by vitamin C.*
