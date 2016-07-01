#ifndef YB_XML_H__
#define YB_XML_H__

#include <string.h>
#include <map>
#include <string>

namespace xml {

template<class T,int N>
struct st_stack_t {
  struct T_ { char d[sizeof(T)]; };
  int n;
  T_ data[N];

  st_stack_t() { n = 0; }

  int size() const { return n; }
  bool empty() const { return n == 0; }
  T& at(int i) { return *reinterpret_cast<T*>( &data[i].d[0] ); }

  T& push() { return * new( &at(n++) ) T(); }
  void pop() { at(--n).~T(); }
  T& back() { return at(n-1); }
};

typedef std::map<std::string,std::string> AttrMap;

struct XTag {
  enum { Open, Close, SelfClose, PI, DTD, Comment };
  int type;
  std::string name;
  AttrMap attr;
  void print( std::ostream& s );
  void print( std::ostream& s, const char* p, size_t len );
};

struct XParser;

struct XException {
  const char* desc_;
  XParser* parser_;
  const char* p_;
  XException( const char* desc, XParser* parser, const char* p );
};

struct XParser {
  // cb
  struct Events {
    virtual void onTag( XParser&, XTag& ) {}
    virtual void onTagText( XParser&, XTag&, const char*, size_t ) {}
  };

  // data
  Events* ev_;
  const char* doc_;
  const char* tagpos_;
  st_stack_t<XTag,64> stack_;

  // utils
  static bool is_name1( char c );
  static bool is_name( char c );
  static bool is_space( char c ); 
  static const char* skip_space( const char* p );
  static const char* rskip_space( const char* p );

  // parsing
  const char* parse_comment( const char* p );
  const char* parse_dtd( const char* p ); 
  const char* parse_pi( const char* p );
  const char* parse_open_tag( const char* p );
  const char* parse_close_tag( const char* p );
  const char* parse_tag( XTag& tag, const char* p );
  const char* parse_text( const char* p );

  // public interface
  void parse( const char* s, Events* ev );
};

} // namesapce xml

#endif // #ifndef YB_XML_H__
