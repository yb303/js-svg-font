#include <string.h>
#include <ostream>
#include "xml.h"

namespace xml {

XException::XException( const char* desc, XParser* parser, const char* p ) {
  desc_ = desc;
  parser_ = parser;
  p_ = p;
}

void XTag::print( std::ostream& s ) {
  if( type == XTag::Open || type == XTag::SelfClose ) {
    s << '<' << name;
    for( auto& a : attr )
      s << ' ' << a.first << '=' << '"' << a.second << '"';
    if( type == XTag::SelfClose )
      s << "/>";
    else
      s << '>';
  }
  else if( type == XTag::Close ) {
    s << "</" << name << '>';
  }
  else if( type == XTag::PI ) {
    s << "<?" << name;
    for( auto& a : attr )
      s << ' ' << a.first << '=' << '"' << a.second << '"';
    s << "?>";
  }
}
void XTag::print( std::ostream& s, const char* p, size_t len ) {
  if( type == XTag::DTD ) {
    s << "<!" << std::string(p,len) << '>';
  }
  else if( type == XTag::Comment ) {
    s << "<!-- " << std::string(p,len) << " -->";
  }
}

// utils
bool XParser::is_name1( char c ) {
  return( (c >= 'a' && c <= 'z') || c == '_' || (c >= 'A' && c <= 'Z') );
}
bool XParser::is_name( char c ) {
  return( (c >= 'a' && c <= 'z') || c == '-' || (c >= '0' && c <= '9')
      || c == '_' || c == '.' || c == ':' || (c >= 'A' && c <= 'Z') );
}
bool XParser::is_space( char c ) {
  return( c == ' ' || c == '\r' || c == '\n' || c == '\t' );
}
const char* XParser::skip_space( const char* p ) {
  while( is_space(*p) ) p++;
  return p;
}
const char* XParser::rskip_space( const char* p ) {
  while( is_space(*p) ) p--;
  return p;
}

// parsing
const char* XParser::parse_comment( const char* p ) {
  XTag& tag = stack_.push();
  tag.type = XTag::Comment;

  const char* start = p;
  p = strstr(p, "-->");
  if( !p )
    throw XException("unterminated comment",this,p);

  ev_->onTagText( *this, tag, start, p-start );
  stack_.pop();
  return skip_space(p+3);
}

const char* XParser::parse_dtd( const char* p ) {
  XTag& tag = stack_.push();
  tag.type = XTag::DTD;

  const char* start = p;
  int depth = 0;
  for(;;p++) {
    if( *p == '>' && depth==0 ) {
      break;
    } else if( *p == '[' ) {
      depth++;
    } else if( *p == ']' ) {
      depth--;
    } else if( !*p ) {
      throw XException("unterminated dtd",this,p);
    }
  }
  ev_->onTagText( *this, tag, start, p-start );
  stack_.pop();
  return skip_space(p+1);
}

const char* XParser::parse_pi( const char* p ) {
  XTag& tag = stack_.push();
  tag.type = XTag::PI;

  p = parse_tag( tag, p );

  if( p[0] == '?' && p[1] == '>' ) {
    ev_->onTag( *this, tag );
    stack_.pop();
    return skip_space(p+2);
  }
  else {
    throw XException("bad PI",this,p);
  }
}

const char* XParser::parse_open_tag( const char* p ) {
  XTag& tag = stack_.push();

  p = parse_tag( tag, p );

  if( p[0] == '/' && p[1] == '>' ) {
    tag.type = XTag::SelfClose;
    ev_->onTag( *this, tag );
    stack_.pop();
    return skip_space(p+2);
  }
  else if( *p == '>' ) {
    tag.type = XTag::Open;
    ev_->onTag( *this, tag );
    return skip_space(p+1);
  }
  else {
    throw XException("bad tag",this,p);
  }
}

const char* XParser::parse_close_tag( const char* p ) {
  XTag& tag = stack_.back();

  const char* start = p;
  while( is_name(*p) ) p++;

  if( tag.name != std::string(start, p-start) )
    throw XException("wrong closing tag",this,p);

  if( *p == '>' ) {
    tag.type = XTag::Close;
    ev_->onTag( *this, tag );
    stack_.pop();
  }
  else {
    throw XException("bad close tag",this,p);
  }
  return skip_space(p+1);
}

const char* XParser::parse_tag( XTag& tag, const char* p ) {
  const char* start = p++;
  while( is_name(*p) ) p++;
  tag.name.assign(start, p-start);

  p = skip_space(p);

  // attributes
  while( is_space(p[-1]) && is_name(*p) ) {
    // key
    start = p++;
    while( is_name(*p) ) p++;
    auto pib = tag.attr.insert( AttrMap::value_type(std::string(start,p-start),"") );
    if( p[0] == '=' && p[1] == '"' ) {
      // value
      p += 2;
      start = p;
      const char* p2 = strchr(p,'"');
      if( !p2 )
        throw XException("unterminated attr",this,p);
      (*pib.first).second.assign( start, p2-start );
      p = p2+1;
    }
    p = skip_space(p);
  }

  return p;
}

const char* XParser::parse_text( const char* p ) {
  XTag& tag = stack_.back();
  const char* p2 = strchr(p,'<');
  if( !p2 )
    throw XException("unterminated text",this,p);

  ev_->onTagText( *this, tag, p, p2-p );
  return p2;
}

void XParser::parse( const char* s, Events* ev ) {
  doc_ = s;
  ev_ = ev;
  tagpos_ = s;

  const char* p = s;
  p = skip_space(p);
  while( *p ) {
    tagpos_ = p;
    // tag
    if( *p == '<' ) {
      p++;

      if( is_name1(*p) ) {
        p = parse_open_tag( p );
      }
      else if( *p == '/' ) {
        p = parse_close_tag( p+1 );
      }
      else if( *p == '!' ) {
        p++;
        if( p[0] == '-' && p[1] == '-' ) {
          p = parse_comment( p+2 );
        }
        else {
          p = parse_dtd( p );
        }
      }
      else if( *p == '?' ) {
        p = parse_pi( p+1 );
      }
      else {
        throw XException("unknown animal",this,p);
      }
    }
    else {
      if( stack_.empty() )
        throw XException("free text",this,p);
      p = parse_text( p );
    }
  }
  if( !stack_.empty() )
    throw XException("unterminated tags",this,p);
}

} // namespace xml

