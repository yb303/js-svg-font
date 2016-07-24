"use strict";

function SvgPath(s) {
  function parse(s) {
    var flen = {m:2,l:2,h:1,v:1,c:6,s:4,q:4,t:2,z:0};
    var n = s.length;
    var i = 0;
    var ret = [];
    while( i < n ) {
      while( i < n && s.charAt(i) == ' ' ) i++;
      var cmd = s.charAt(i);
      if( !(cmd.toLowerCase() in flen) ) throw "bad command "+c;
      var t = [cmd];
      i++;
      for(;;) {
        while( i < n && s.charAt(i) == ' ' ) i++;
        var c, e = i;
        if( e < n && s.charAt(e) == '-' ) e++;
        while( e < n && (((c=s.charAt(e)) >= '0' && c <= '9') || c=='.') ) e++;
        if( e == i ) break;
        t.push( parseFloat(s.substr(i,e-i)) );
        i = e;
      }
      if( t.length-1 != flen[cmd.toLowerCase()] ) throw "wrong arg count "+c;
      ret.push(t);
    }
    return ret;
  }
  // ctor
  this.df = parse(s);

  this.draw = function(ctx,sx,sy,s) {
    var x=0, y=0, x1, y1, x2, y2;
    for( var i = 0; i < this.df.length; i++ ) {
      var f = this.df[i];
      switch(f[0]) {
      case 'M': x  = sx+f[1]*s; y  = sy-f[2]*s; ctx.moveTo(x,y); break;
      case 'm': x +=    f[1]*s; y -=    f[2]*s; ctx.moveTo(x,y); break;
      case 'L': x  = sx+f[1]*s; y  = sy-f[2]*s; ctx.lineTo(x,y); break;
      case 'l': x +=    f[1]*s; y -=    f[2]*s; ctx.lineTo(x,y); break;
      case 'H': x  = sx+f[1]*s;                 ctx.lineTo(x,y); break;
      case 'h': x +=    f[1]*s;                 ctx.lineTo(x,y); break;
      case 'V':                 y  = sy-f[1]*s; ctx.lineTo(x,y); break;
      case 'v':                 y -=    f[1]*s; ctx.lineTo(x,y); break;
      case 'Q': x1 = sx+f[1]*s; y1 = sy-f[2]*s;
                x  = sx+f[3]*s; y  = sy-f[4]*s; ctx.quadraticCurveTo(x1,y1, x,y); break;
      case 'q': x1 =  x+f[1]*s; y1 =  y-f[2]*s;
                x +=    f[3]*s; y -=    f[4]*s; ctx.quadraticCurveTo(x1,y1, x,y); break;
      case 'T': x1 =  x-(x1-x); y1 =  y-(y1-y);
                x  = sx+f[1]*s; y  = sy-f[2]*s; ctx.quadraticCurveTo(x1,y1, x,y); break;
      case 't': x1 =  x-(x1-x); y1 =  y-(y1-y);
                x +=    f[1]*s; y -=    f[2]*s; ctx.quadraticCurveTo(x1,y1, x,y); break;
      case 'C': x1 = sx+f[1]*s; y1 = sy-f[2]*s;
                x2 = sx+f[3]*s; y2 = sy-f[4]*s;
                x  = sx+f[5]*s; y  = sy-f[6]*s; ctx.bezierCurveTo(x1,y1, x2,y2, x,y); break;
      case 'c': x1 =  x+f[1]*s; y1 =  y-f[2]*s;
                x2 =  x+f[3]*s; y2 =  y-f[4]*s;
                x +=    f[5]*s; y -=    f[6]*s; ctx.bezierCurveTo(x1,y1, x2,y2, x,y); break;
      case 'S': x1 =  x-(x1-x); y1 =  y-(y1-y);
                x2 = sx+f[1]*s; y2 = sy-f[2]*s;
                x  = sx+f[3]*s; y  = sy-f[4]*s; ctx.bezierCurveTo(x1,y1, x2,y2, x,y); break;
      case 's': x1 =  x-(x2-x); y1 =  y-(y2-y);
                x2 =  x+f[1]*s; y2 =  y-f[2]*s;
                x +=    f[3]*s; y -=    f[4]*s; ctx.bezierCurveTo(x1,y1, x2,y2, x,y); break;
      case 'z': ctx.closePath(); break;
      }
    }
  };
}

function SvgFont(svg) {
  this.svg = svg;
  this.letters = svg.glyph;
  
  this.create = function() {
    // assign missing hadv and hkern=0
    for( var c in this.letters ) {
      var cur = this.letters[c];
      if( !('hadv' in cur) )
        cur.hadv = this.svg.hadv;
      cur.hkern = {};
      for( var c2 in this.letters )
        cur.hkern[c2] = 0;
    }
    // set specific hkern
    for( var i = 0; i < this.svg.hkern.length; i++ ) {
      var hk = this.svg.hkern[i];
      for( var j = 0; j < hk.g1.length; j++ ) {
        var g1hk = this.letters[hk.g1.charAt(j)].hkern;
        for( var k = 0; k < hk.g2.length; k++ ) {
          var g2 = hk.g2.charAt(k);
          g1hk[g2] = hk.k;
        }
      }
    }
    // parse d
    for( c in this.letters )
      this.letters[c].svg = new SvgPath(this.letters[c].d);
  };
  this.create();

  this.measureText = function(text,maxWidth,maxHeight) {
    var n = text.length;
    var th = this.svg.em, tw = 0, ph = 0; // total width, total height, page height
    var x = 0, k = 0;
    var cur = this.letters[' '];
    var wi = 0, wx = 0, lines = [], pages = [], tph = th;
    for( var i = 0; i < n; i++ ) {
      var c = text.charAt(i);
      if( maxWidth > 0 && x > maxWidth ) {
        if( wi > 0 ) { // break between words - use last space
          i = wi;
          x = wx;
        } else { // break mid-word. yuck!
          i -= 2;
          x -= -k + cur.hadv;
        }
        c = '\n';
      }
      if( c in this.letters ) {
        if( c == ' ' ) { wi = i; wx = x; } // store last space position and index
        k = cur.hkern[c];
        cur = this.letters[c];
        x += -k + cur.hadv;
      } else if( c == '\n' ) {
        if( tw < x ) tw = x;
        lines.push(i);
        if( maxHeight > 0 && tph + this.svg.em > maxHeight ) {
          ph = tph;
          tph = 0;
          pages.push(i+1);
        }
        th += this.svg.em; tph += this.svg.em;
        cur = this.letters[' '];
        x = wx = wi = 0;
      } else {
        cur = this.letters['--'];
        x += cur.hadv;
      }
    }
    if( tw < x ) tw = x;
    lines.push(n);
    pages.push(n);
    return {width:tw, height:th, lines:lines, pages:pages, pageHeight:ph, plhint:0};
  };

  this.drawGlyph = function(ctx,sx,sy,s,c) {
    //sx -= this.svg.bbox.x1 * s;
    sy += this.svg.bbox.y1 * s;
    ctx.beginPath();
    this.letters[c].svg.draw(ctx,sx,sy,s);
    ctx.fill();
  };
};

function ImgFont(svgfont,px,opt) {
  this.opt = opt;
  this.em = px;
  this.svgfont = svgfont;
  this.img = {};
  this.scale = px/svgfont.svg.em;
  this.canvas = document.createElement("canvas");
  this.ctx = null;
  this.alpha = 1;

  // public
  this.align = "left";
  this.baseline = "bottom";

  this.create = function() {
    // calc scaled widths and kerns and adjust
    var sx = 1, sy = 1;
    var letters = this.svgfont.letters;
    for( var c in letters ) {
      var cur = this.img[c] = {}, orig = letters[c];
      cur.hkern = {};
      var curhkern = cur.hkern, orighkern = orig.hkern;
      for( var c2 in letters )
        curhkern[c2] = orighkern[c2] * this.scale;
      cur.hadv = orig.hadv * this.scale;
      cur.sy = 0;
      cur.sx = sx;
      //TODO: add something more sensible from bounding box
      sx += Math.floor(cur.hadv + 4.99); // round next char to pixel
    }
    // optimize to a square
    var qsize = Math.sqrt(sx * this.em);
    var size = Math.pow( 2, Math.ceil( Math.log(qsize)/Math.log(2) ) );
    for( var t = 0; t < 2; t++) {
      sx = 1;
      for( var c in letters ) {
        var cur = this.img[c], orig = letters[c];
        cur.sy = sy;
        cur.sx = sx;
        sx += Math.floor(cur.hadv + 4.99); // round next char to pixel
        if( sx >= size ) {
          cur.sy = (sy += this.em + 2);
          cur.sx = 1; sx = Math.floor(cur.hadv + 4.99);
        }
      }
      if( sy + this.em + 2 < size ) break;
      size *= 2;
    }
    this.canvas.width = size;
    this.canvas.height = size;
    var ctx = this.ctx = this.canvas.getContext("2d");
    ctx.fillStyle = "#ffffff";
    for( var c in this.img )
      this.svgfont.drawGlyph( ctx, this.img[c].sx, this.em+1+this.img[c].sy, this.scale, c );

    ctx.globalCompositeOperation = "source-in";
  };
  this.create();

  this.measureText = function(text,maxWidth,maxHeight) {
    if( typeof(maxWidth) == 'undefined' ) maxWidth = 0;
    if( typeof(maxHeight) == 'undefined' ) maxHeight = 0;
    var r = this.svgfont.measureText( text, maxWidth/this.scale, maxHeight/this.scale );
    r.width *= this.scale;
    r.height *= this.scale;
    r.pageHeight *= this.scale;
    return r;
  };
  this.setStyle = function(style,alpha) {
    if( this.ctx.fillStyle == style && this.alpha == alpha ) return;
    this.ctx.fillStyle = style;
    this.alpha = alpha;
    this.ctx.fillRect(0,0,this.canvas.width,this.canvas.height);
  };

  this.topalign = this.svgfont.svg.bbox.y1*this.scale+2;
  this.drawText = function(ctx,x,y,text,maxWidth,maxHeight) {
    this.drawTextEx( ctx, x, y, text+"", this.measureText(text+"",maxWidth,maxHeight), 0 );
  };
  this.drawTextEx = function(ctx,x,y,text,textProps,pg) {
    var n = text.length;
    var start = pg>0 ? textProps.pages[pg-1] : 0;
    var end = textProps.pages[pg];
    var l = textProps.plhint, next_i = textProps.lines[l];
    while( textProps.lines[l] < start ) l++;
    while( l > 0 && textProps.lines[l-1] >= start ) l--;
    var ha = this.align, va = this.baseline, w = textProps.width, h = textProps.height;
    x -= ha=="right" ? w : ha=="center" ? w/2 : 0;
    y -= va=="bottom" ? h+1 : va=="top" ? this.topalign : va=="middle" ? (this.topalign+h+1)/2 : 0;
    var ise = ctx.imageSmoothingEnabled;
    var ga = ctx.globalAlpha;
    ctx.imageSmoothingEnabled = false;
    ctx.globalAlpha = this.alpha;
    var px = x, py = y;
    var sh = this.em+2;
    var cur = this.img[' '];
    for( var i = start; i < end; i++ ) {
      var c = text.charAt(i);
      if( i == next_i ) {
        next_i = textProps.lines[++l];
        py += this.em;
        cur = this.img[' '];
        px = x;
        if( c==' ' || c == '\n' ) continue;
      }
      if( c in this.img ) {
        px -= cur.hkern[c];
        cur = this.img[c];
        var sw = cur.hadv+1;
        ctx.drawImage( this.canvas,  cur.sx, cur.sy, sw, sh,  px, py, sw, sh );
        px += sw-1;
      } else {
        cur = this.img['--'];
        var sw = cur.hadv+1;
        ctx.drawImage( this.canvas,  cur.sx, cur.sy, sw, sh,  px, py, sw, sh );
        px += sw-1;
      }
    }
    ctx.imageSmoothingEnabled = ise;
    ctx.globalAlpha = ga;
    textProps.plhint = l; // for the next page
  };
  this.imgText = function(text,maxWidth) {
    var canvas = document.createElement("canvas");
    var r = this.measureText(text,maxWidth);
    canvas.width = r.width;
    canvas.height = r.height;
    var ctx = canvas.getContext("2d");
    this.align = "left";
    this.baseline = "bottom";
    this.drawText(ctx,0,r.height,text);
    return canvas;
  };
  this.drawImg = function(ctx,x,y,img) {
    var ha = this.align, va = this.baseline, w = img.width;
    x -= ha=="right" ? w : ha=="center" ? w/2 : 0;
    y -= va=="bottom" ? this.em+1 : va=="top" ? this.topalign : va=="middle" ? (this.topalign+this.em+1)/2 : 0;
    var ise = ctx.imageSmoothingEnabled;
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(img, x, y );
    ctx.imageSmoothingEnabled = ise;
  };
}

