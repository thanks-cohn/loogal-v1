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

THE PROMISE

If the image exists anywhere on your system

Loogal should lead you back to it.

--------------------------------------------------------------------------------
