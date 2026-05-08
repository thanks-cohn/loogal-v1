Click.Drop.Done
==========================================
## LOOGAL 


WHAT IS LOOGAL?

Loogal is like:

  Google Images search
  + Finder / File Explorer
  + your own memory

But entirely local.

It's as easy as selecting an image, dropping it in the loogal window and...



Boom.

Your system tells you everywhere it has ever seen it.

--------------------------------------------------------------------------------

THE MOMENT

You open:

  ./loogal-window

You drag an image in.

No prompt.
No form.
No setup.

Boom.

A new window opens.

And there it is:

  98%  ~/Downloads/telegram/meme_copy.jpg
  
  96%  ~/Pictures/old/screenshots/same_meme.webp
  
  93%  ~/Documents/archive/report.pdf:page-17:image-002

Folders you forgot.
Files you didn’t name.
A PDF you didn’t even remember existed.

And somehow...

it found all of them.

--------------------------------------------------------------------------------



“I always knew this should be possible.”

Not cloud.
Not AI magic.
Not uploading your life somewhere.

Just your machine…

finally remembering what it has seen.

It's about time! 

--------------------------------------------------------------------------------

WHAT LOOGAL IS

Loogal is a local visual memory engine.

It replaces:

  manual visual recall

You don’t search by name.

You don’t search by path.

You show it the image.

It answers.

--------------------------------------------------------------------------------

WHY THIS IS DIFFERENT

Every other system asks you to remember:

  filenames
  folders
  tags
  where you saved something

Loogal flips it:

  you remember what it looked like

That’s enough.

Windows Quick Start

## Download:

## https://github.com/thanks-cohn/loogal/releases


## Loogal Windows v3

Download:

## loogal-windows.zip

Extract it anywhere.

Example:
```text
Desktop/
└── loogal-windows/
    ├── LoogalWindow.exe
    ├── loogal.exe
    └── ...
```
# Important:

Extract the ZIP first.

Do not run LoogalWindow.exe from inside the ZIP preview.

Open the extracted folder.

# Double-click:

## LoogalWindow.exe

Click:

Add Folders (Index)

Choose folders you want Loogal to remember.

Good starting folders:

Pictures

Downloads

Screenshots

Memes

Art

Research

After indexing finishes:

Click:

Open Image...

Choose any image.

Loogal searches your visual memory and shows similar matches instantly.

## Double-click a result to reveal it in Windows Explorer.




--------------------------------------------------------------------------------

WHAT MAKES THIS WORK

No cloud.
No accounts.
No uploads.

Search runs on:

  a binary index

That’s it.

No JSON parsing.
No database overhead.
No network calls.

Just:

  image → hash → match → results

Fast.
Deterministic.
Local.

--------------------------------------------------------------------------------

AND THEN YOU REALIZE

You don’t need to organize your images anymore.

You don’t need perfect filenames.

You don’t need to remember where things are.

Because:

  you can always find them again.

--------------------------------------------------------------------------------


```text
╔══════════════════════════════════════════════════════════════╗
║                     CLICK. DROP. DONE.                       ║
╚══════════════════════════════════════════════════════════════╝

You do not search by filename anymore.

You search by MEMORY.

──────────────────────────────────────────────────────────────

INDEX YOUR WORLD

  ./loogal index ~/Pictures

Scans your image universe.
Builds a local visual memory engine.
Turns chaos into searchable recall.

The beginning.

──────────────────────────────────────────────────────────────

OPEN THE WINDOW

  ./loogal-window

Drag.
Drop.
Boom.

A visual search engine for your own machine appears.

No cloud.
No upload.
No accounts.

Just:
your image → your memory → your results

──────────────────────────────────────────────────────────────

SEARCH BY IMAGE

  ./loogal search meme.png

Find every version.
Every crop.
Every repost.
Every rename.
Every forgotten duplicate.

Even if you buried it months ago.

──────────────────────────────────────────────────────────────

SEARCH EVERYTHING YOU'VE INDEXED

  ./loogal search screenshot.jpg --memory

Not a folder.

Your ENTIRE visual memory.

──────────────────────────────────────────────────────────────

FIND NEAR DUPLICATES

  ./loogal similar vacation_photo.png

Compressed copies.
Screenshots.
Crops.
Saves from Discord.
Telegram reposts.
Tiny edits.

Loogal still remembers.

──────────────────────────────────────────────────────────────

BUILD A LIVING MEMORY

  ./loogal watch-add ~/Pictures --hourly

Tell Loogal where your life happens.

Screenshots.
Downloads.
Art folders.
Meme vaults.
Research dumps.

──────────────────────────────────────────────────────────────

START THE DAEMON

  ./loogal watch-start

Now forget indexing exists.

Loogal quietly watches your world evolve.

Every new image:
remembered.

Every modification:
tracked.

Every duplicate:
understood.

Forever.

──────────────────────────────────────────────────────────────

DEDUPE THE ABYSS

  ./loogal dedupe --dry-run

See how much visual clutter you've accumulated.

The machine reveals the echoes.

──────────────────────────────────────────────────────────────

THE FEELING

You remember what it looked like.

That should have been enough.

Now it finally is.

──────────────────────────────────────────────────────────────

THE PROMISE

If the image exists anywhere on your machine...

Loogal should lead you back to it.
```

---------------------------------


WHAT COMES NEXT ! PDF MAVERICK

Right now, Loogal finds images.

Next, it finds images inside PDFs.

You will be able to:

  ./loogal index ~/Documents --include pdf
  ./loogal search query.png ~/Documents --include pdf

Loogal will:

  extract images from PDFs
  index them
  remember which PDF they came from

Even if that PDF is:

  renamed
  moved
  reorganized

Search still resolves.

Because Loogal tracks the documen, not just the path.

--------------------------------------------------------------------------------
