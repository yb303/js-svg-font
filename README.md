# js-svg-font
A lightweight javascript support for svg fonts

The js is about 300 lines of code. 10.5KB. (freetype.so/.dll is like 4MB)
A js font file (converted from svg) can be like 20KB. (A ttf file is a lot bigger)
Obviously, the SvgPath class can be used to parse and draw svg paths ("d" attribute) unrelated to fonts.

### Pre-process:

1. Find a free font you like. Get the ttf file
2. Convert it online to svg
3. Use some text editor to remove all the languages you don't care about.
4. Use opt-svg to convert the svg xml into json. It optimizes the kerning records while at it.

### Run-time:

1. Include svg-font.js and your_font.js
2. sf = new SvgFont(theFont);   // create SvgFont<br/>
   xf = new ImgFont(sf,15);     // create ImgFont for the desired size. this pre-renders the chars
3. Use it: xf.drawText(context, x, y, text);


