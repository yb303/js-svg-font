var ScrolledText = function(font,text,width,height) {
  this.sy = 0;
  this.pages = [];
  this.width = width;
  this.height = height;
  this.scroll = 0;
  this.done = false;

  this.prerender = function(font,text) {
    this.font = font;
    this.text = text;
    font.baseline = "top";
    font.align = "left";
    var maxHeight = Math.pow(2, Math.ceil(Math.log(width)/Math.log(2)));
    this.props = font.measureText(text, this.width, maxHeight);
    this.pageHeight = this.props.pageHeight;
    this.totalHeight = this.props.height - this.height;
  };
  this.renderPage = function(i) {
    //console.log("rendering page "+i+" out of "+this.props.pages.length);
    var page = document.createElement("canvas");
    this.pages.push(page);
    page.width = this.props.width;
    page.height = this.props.pageHeight;
    var pageCtx = page.getContext("2d");
    this.font.drawTextEx(pageCtx, 0, 0, this.text, this.props, i);
  };
  this.advance = function() {
    if( this.done ) return;
    this.renderPage(this.pages.length);
    this.done = ( this.pages.length == this.props.pages.length );
  };
  this.prerender(font,text);

  this.draw = function(ctx, x, y) {
    var y2 = y + this.height;
    var pi = Math.floor(this.scroll/this.pageHeight);
    var y_cut = this.scroll - pi * this.pageHeight;
    while( y < y2 && pi < this.pages.length ) {
      if( this.pageHeight - y_cut > 0 ) {
        var h = this.pageHeight - y_cut;
        if( y + h > y2 )
          h = y2 - y;
        ctx.drawImage(this.pages[pi], 0, y_cut, this.width, h, x, y, this.width, h );
      }
      pi++;
      y += this.pageHeight - y_cut;
      y_cut = 0;
    }
  };

  this.scrolling = false;
  this.scroll0 = 0;
  this.touchdown = function(y) {
    this.scrolling = this.done;
    this.scroll0 = this.scroll + y;
  };
  this.touchmove = function(y) {
    if( this.scrolling ) {
      this.scroll = this.scroll0 - y;
      if( this.scroll < 0 ) {
        this.scroll = 0;
        this.scroll0 = y;
      }
      else if( this.scroll > this.totalHeight ) {
        this.scroll = this.totalHeight;
        this.scroll0 = this.scroll + y;
      }
    }
  };
  this.touchup = function(y) {
    this.scrolling = false;
  };
};

