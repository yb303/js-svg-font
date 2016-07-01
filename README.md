# js-svg-font
A lightweight javascript support for svg fonts

The js is about 250 lines of code. 8KB of text. A js font file (converted from svg) can be like 20KB.

### Pre-process:

1. Find a free font you like. Get the ttf file
2. Convert it online to svg
3. Use opt-svg to converts the svg xml into json. It optimizes the kerning records while at it.

### Run-time:

1. Include svg-font.js and your_font.js
2. Create SvgFont and ImgFont for the desired size.
3. Use it: imgFont.drawText( context, x, y, text );
