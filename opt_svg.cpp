#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <iostream>

#include "xml.h"

using namespace xml;

int from_hex_1(char c) {
  if( c >= '0' && c <= '9' ) return c-'0';
  if( c >= 'a' && c <= 'f' ) return c-'a'+10;
  if( c >= 'A' && c <= 'F' ) return c-'A'+10;
  throw "bad hex";
}
int from_hex_2(const char* s) {
  return from_hex_1(s[0]) * 16 + from_hex_1(s[1]);
}
bool need_q(char c) {
  return c=='\\' || c=='\'' || c=='"';
}

std::vector<std::string> split( const std::string& s, char delim ) {
  std::vector<std::string> v;
  int prev = 0;
  for(;;) {
    size_t p = s.find(delim,prev);
    if( p != (size_t)-1 ) {
      v.push_back( s.substr(prev,p-prev) );
      prev = p+1;
    } else {
      v.push_back( s.substr(prev) );
      break;
    }
  }
  return v;
}

struct Opt : XParser::Events {
  typedef std::pair<int,int> KKey;
  typedef std::map< KKey, std::string > KMap;

  struct HKern {
    std::string g1,g2,u1,u2;
    int k;
  };
  struct Glyph {
    std::string name;
    std::string d;
    int hadv = 0;
    int id = 0;
  };

  std::vector<HKern> hkerns;
  std::vector<Glyph> glyphs;

  KMap kmap;
  std::map<std::string,int> glyph_map;

  std::string id;
  int default_hadv = 0;
  int em_size = 0;
  int ascent = 0;
  int descent = 0;
  int xheight = 0;
  int cap_height = 0;
  std::vector<std::string> bbox;
  int ul_width = 0;
  int ul_pos = 0;

  int get_char( const std::string& s ) {
    int id = 0;
    if( s.size() == 1 )
      id = s.c_str()[0];
    else if( s.size() == 6 && strncmp(s.c_str(),"&#x",3)==0 )
      id = from_hex_2(s.c_str()+3);
    return id;
  }

  void onTag( XParser&, XTag& tag ) override {
    if( (tag.type == XTag::Open || tag.type == XTag::SelfClose) && tag.name == "hkern" ) {
      HKern k;
      k.g1 = tag.attr["g1"];
      k.g2 = tag.attr["g2"];
      k.u1 = tag.attr["u1"];
      k.u2 = tag.attr["u2"];
      k.k = atoi(tag.attr["k"].c_str());
      hkerns.push_back(k);
    }
    else if( (tag.type == XTag::Open || tag.type == XTag::SelfClose) &&
      (tag.name == "glyph" || tag.name == "missing-glyph")
      ) {
      Glyph g;
      if( tag.name == "missing-glyph" ) {
        g.id = 0;
        g.name = "--";
      }
      else if( (g.id = get_char( tag.attr["unicode"] )) ) {
        g.name = tag.attr["glyph-name"];
      }
      else {
        //throw "bad char";
        return;
      }

      if( tag.attr.find("horiz-adv-x") != tag.attr.end() )
        g.hadv = atoi(tag.attr["horiz-adv-x"].c_str());

      g.d = tag.attr["d"];
      char* s = (char*)g.d.c_str();
      //char* p = s;
      //char* p0 = p;
      //while( *s ) { if( *s != '\n' && *s != '\r') *p++ = *s; s++; } g.d.resize(p-p0);
      while( *s ) { if( *s == '\n' || *s == '\r') *s = ' '; s++; }

      if( (int)glyphs.size() <= g.id )
        glyphs.resize(g.id+1);
      glyphs[g.id] = g;
      glyph_map[g.name] = g.id;
    }
    else if( (tag.type == XTag::Open || tag.type == XTag::SelfClose) && tag.name == "font-face" ) {
      em_size = atoi(tag.attr["units-per-em"].c_str());
      ascent  = atoi(tag.attr["ascent"].c_str()); // ="800"
      descent = atoi(tag.attr["descent"].c_str()); // ="-200"
      xheight = atoi(tag.attr["x-height"].c_str()); // ="540"
      cap_height = atoi(tag.attr["cap-height"].c_str()); // ="729"
      bbox = split(tag.attr["bbox"],' '); // ="-22 -219 947 763"
      ul_width = atoi(tag.attr["underline-thickness"].c_str()); // ="69"
      ul_pos = atoi(tag.attr["underline-position"].c_str()); // ="-155"
    }
    else if( tag.type == XTag::Close && tag.name == "font" ) {
      id = tag.attr["id"];
      default_hadv = atoi(tag.attr["horiz-adv-x"].c_str());
    }
  }

  void filter_glyphs( std::vector<std::string>& v ) {
    int n = v.size();
    for( int i = 0; i < n; i++ ) {
      if( glyph_map.find(v[i]) == glyph_map.end() )
        v[i--] = v[--n];
    }
    v.resize(n);
  }

  void crunch() {
    // map g1+k => multi g2
    for( auto& k : hkerns ) {
      auto g1v = split(k.g1,',');
      auto g2v = split(k.g2,',');
      filter_glyphs(g1v);
      filter_glyphs(g2v);
      int u1 = get_char(k.u1);
      if( u1 && u1 < (int)glyphs.size() && glyphs[u1].id == u1 )
        g1v.push_back(glyphs[u1].name);
      int u2 = get_char(k.u2);
      if( u2 && u2 < (int)glyphs.size() && glyphs[u2].id == u2 )
        g2v.push_back(glyphs[u2].name);
      for( auto& g1 : g1v ) {
        KKey kk;
        kk.first = glyph_map[g1];
        kk.second = k.k;
        auto& a = kmap[kk];
        for( auto& g2 : g2v ) {
          char u = glyph_map[g2];
          if( a.find(u) == (size_t)-1 ) {
            if( need_q(u) ) a += '\\';
            a += u;
          }
        }
      }
    }
    // reverse map
    std::map<std::string,std::vector<KKey>> rm;
    for( auto& kk : kmap )
      rm[kk.second].push_back( kk.first );
    // missing hadv
    for( auto& g : glyphs ) {
      if( !g.id ) continue;
      if( !g.hadv ) g.hadv = default_hadv;
    }
    
    // print
    std::cout << "var svg_" << id << "={\n"
      << "hadv: " << default_hadv << ",\n"
      << "em: " << em_size << ",\n"
      << "ascent: " << ascent << ",\n"
      << "descent: " << descent << ",\n"
      << "xheight: " << xheight << ",\n"
      << "cap_height: " << cap_height << ",\n"
      << "bbox:{x1:" << bbox[0]
          << ", y1:" << bbox[1]
          << ", x2:" << bbox[2]
          << ", y2:" << bbox[3] << "},\n"
      << "ul_width: " << ul_width << ",\n"
      << "ul_pos: " << ul_pos << ",\n"

      << "glyph: {\n";
    for( auto& g : glyphs ) {
      if( g.id )
        std::cout << "  '" << (need_q(g.id) ? "\\" : "") << (char)g.id;
      else if( g.name == "--" )
        std::cout << "  '--";
      else
        continue;
      std::cout << "':{ hadv:" << g.hadv << ", d:'" << g.d << "'},\n";
    }
    std::cout << "  '\\t':{ hadv:" << glyphs[' '].hadv << ", d:'' }\n"; // add \t?
    std::cout << "},\n"
      << "hkern: [\n";
    for( auto& k : kmap ) {
      auto& kkv = rm[k.second];
      if( k.first != kkv[0] )
        continue;
      std::cout << "  {g1:'";
      for( auto& kk : kkv )
        std::cout << (need_q(kk.first) ? "\\" : "") << (char)kk.first;
      std::cout << "', g2:'" << k.second << "', k:" << k.first.second << "},\n";
    }
    std::cout << "  {g1:'', g2:'', k:0}\n"
      << "]\n"
      << "}\n";
  }
};

int main( int argc, char* argv[] ) {

  if( argc < 2 ) {
    fprintf(stderr,"usage: %s <xml file>\n", argv[0]);
    return 1;
  }

  struct stat st;
  stat(argv[1],&st);
  int n = st.st_size;

  FILE* f = fopen(argv[1],"rb");
  if( !f ) {
    fprintf(stderr,"cannot open file\n");
    return 1;
  }

  char* buf = (char*)malloc(n+1);
  if( fread(buf,n,1,f) != 1 ) {
    fprintf(stderr,"cannot read file\n");
    return 1;
  }
  buf[n] = '\0';
  fclose(f);

  XParser xp;
  Opt opt;

  try {
    xp.parse(buf,&opt);
    opt.crunch();
  }
  catch( XException& e ) {
    int pos = e.parser_->tagpos_ - e.parser_->doc_;
    fprintf(stderr, "%s at position %d (%.*s...)\n", e.desc_, pos, 40, e.parser_->tagpos_ );
  }
  catch( const char* s ) {
    int pos = xp.tagpos_ - xp.doc_;
    fprintf(stderr, "'%s' at position %d (%.*s...)\n", s, pos, 40, xp.tagpos_ );
  }
  catch( ... ) {
    int pos = xp.tagpos_ - xp.doc_;
    fprintf(stderr, "? at position %d (%.*s...)\n", pos, 40, xp.tagpos_ );
  }



  return 0;
}




