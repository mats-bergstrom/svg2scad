//                              -*- Mode: C++ -*- 
// Copyright (C) 2026, Mats
// 
// File name       : svg2scad.cpp
// Description     : Create openscad files from paths in an svg
// 
// Author          : Mats
// Created On      : Sun Jan 11 21:02:01 2026
// 
// Last Modified By: Mats
// Last Modified On: Fri Jan 16 22:23:01 2026
// Update Count    : 170
// 


#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <deque>

#include<tinyxml2.h>

using namespace std;
using namespace tinyxml2;


#define LLE(x) cout << #x << " = " << x << endl


class SVGPathToken {
public:
    SVGPathToken() : c(0), d(0) {};
    SVGPathToken(char cc) : c(cc), d(0) {};
    SVGPathToken(double dd) : c(0), d(dd) {};

    ostream& cvs(ostream& os) const {
	if ( c ) {
	    os << c;
	}
	else {
	    os << d;
	}
	return os;
    };
    
    char c;				// 0 or one of MmLlHhVvCcZz
    double d;				// number iff c==0
};

ostream& operator<<(ostream& os, const SVGPathToken& x) {
    return x.cvs(os);
}



typedef deque<SVGPathToken> SVGPathTokenList_t;

class SVGPathTokens {
public:
    SVGPathTokens() : tl() {};

    void parse( const char* s );
    
    ostream& cvs(ostream& os) const {
	SVGPathTokenList_t::const_iterator i;
	os << "{ ";
	for(i=tl.begin(); i != tl.end();++i ) {
	    os << *i << ' ';
	}
	os << '}';
	return os;
    };

    SVGPathTokenList_t tl;
};

ostream& operator<<(ostream& os, const SVGPathTokens& x) {
    return x.cvs(os);
}

int
in_set(char c, const char* s)
{
    while ( *s ) {
	if ( c == *s ) return 1;
	++s;
    }
    return 0;
}

void
skip_ws( const char* &s )
{
    while ( in_set( *s, " \t\n\r," ) )
	++s;
}


void
SVGPathTokens::parse( const char* s )
{
    int n = 0;
    char state = ' ';			// Next token must be a command.
    while( *s ) {
	skip_ws(s);
	if ( state == ' ' ) {
	    // We MUST have a command
	    if ( in_set( *s, "Zz" ) ) {		// Z -
		SVGPathToken t('Z');		// Z and z are identical...
		tl.push_back(t);
		++n;
		state = ' ';
		++s;
	    }
	    else if ( in_set(*s,"MmLlHhVvCc") ) {	// <X> number+
		SVGPathToken t(*s);
		tl.push_back(t);
		++n;
		state = *s;
		++s;
	    }
	    else {
		SVGPathToken t('?');
		tl.push_back(t);
		++n;
		state = '?';
		++s;
		return;
	    }
	}
	else {
	    // Either a command or a number.
	    
	    if ( in_set(*s, "ZzMmLlHhVvCc") ) {
		// A command, set state and loop.
		state = ' ';
	    }
	    else {
		// Must be a number
		char* s_end = 0;
		double d = strtod( s, &s_end );
		if ( s_end == s ) {
		    // Not a number! ERROR!
		    SVGPathToken t('?');
		    tl.push_back(t);
		    ++n;
		    state = '?';	// Mark that it's bad.
		    ++s;
		    return;
		}
		else {
		    SVGPathToken t(d);
		    tl.push_back(t);
		    ++n;
		    s = s_end;
		}
	    }
	}
    }
}


class SVGPoint {
public:
    SVGPoint(double ax=0, double ay=0) : x(ax), y(ay) {};
    double x;
    double y;

    int parse(SVGPathTokenList_t::const_iterator i,
	      SVGPathTokenList_t::const_iterator end);

    static int parse(SVGPathTokenList_t::const_iterator i,
		     SVGPathTokenList_t::const_iterator end,
		     unsigned N, SVGPoint* p);
    
    ostream& cvs( ostream& os ) const {
	return os << '(' << x << ',' << y << ')';
    };
};

ostream& operator<<( ostream& os, const SVGPoint& x) {
    return x.cvs(os);
}

int
SVGPoint::parse(SVGPathTokenList_t::const_iterator i,
		SVGPathTokenList_t::const_iterator end)
// Create a point from two numbers in a token list.  Return no of tokens conusumed (0 or 2).
{

    if ( i == end ) return 0;
    if ( i->c ) return 0;
    x = i->d;
    ++i;

    if ( i == end ) return 0;
    if ( i->c ) return 0;
    y = i->d;

    return 2;
}

int
SVGPoint::parse(SVGPathTokenList_t::const_iterator i,
		SVGPathTokenList_t::const_iterator end,
		unsigned N, SVGPoint* p)
{
    int n = 0;
    int j;
    for ( j = 0; j < N; ++j ) {
	int k = p->parse(i,end);
	if ( k != 2 ) return 0;
	++p;
	n += 2;
	i += 2;
    }
    return n;
}

    



typedef deque<SVGPoint> SVGPointList_t;
ostream& operator<<( ostream& os, const SVGPointList_t& x) {
    SVGPointList_t::const_iterator i = x.begin();
    SVGPointList_t::const_iterator end = x.end();
    while ( i != end ) {
	os << ' ' << *i;
	++i;
    }
    return os;
}

class SVGPathSection {
public:
    
#define PST_NONE	'?'		// pl.size() == 0
#define PST_Moveto	'M'		// pl.size() == N
#define PST_MovetoR	'm'		// pl.size() == N
#define PST_Lineto	'L'		// pl.size() == N
#define PST_LinetoR	'l'		// pl.size() == N
#define PST_Closepath	'Z'		// pl.size() == 0
#define PST_HLineto	'H'		// NOT USED Converted to Lineto
#define PST_HLinetoR	'h'		// NOT USED Converted to LinetoR
#define PST_VLineto	'V'		// NOT USED Converted to Lineto
#define PST_VLinetoR	'v'		// NOT USED Converted to LinetoR

#define PST_Cubic	'C'		// pl.size() == 3*N
#define PST_CubicR	'c'		// pl.size() == 3*N
#define PST_ShortCubic	'S'		// pl.size() == 2*N
#define PST_ShortCubicR	's'		// pl.size() == 2*N
#define PST_Quadratic	'Q'		// pl.size() == 2*N
#define PST_QuadraticR	'q'		// pl.size() == 2*N
#define PST_TQuadratic	'T'		// pl.size() == 1*N
#define PST_TQuadraticR	't'		// pl.size() == 1*N

    
    SVGPathSection(char t=PST_NONE) : pst(t), pl() {};

    int parse(SVGPathTokenList_t::const_iterator i,
	      SVGPathTokenList_t::const_iterator end);
    
    ostream& cvs( ostream& os ) const {
	return os << pst << pl;
    }

    char pst;
    SVGPointList_t pl;
};
ostream& operator<<( ostream& os, const SVGPathSection& x) {
    return x.cvs(os);
}


int
SVGPathSection::parse(SVGPathTokenList_t::const_iterator i,
		      SVGPathTokenList_t::const_iterator end)
{
    // We cannot start with a number.
    if ( i->c == 0 ) {
	pst = PST_NONE;
	return 1;
    }

    else if ( in_set( i->c, "Zz" ) ) {
	// Not followed by a number.
	pst = PST_Closepath;
	return 1;
    }

    else if ( in_set( i->c, "MmLlTt" ) ) { // <X> Point+
	int n = 0;			 // No of tokens consumed.
	int m = 0;
	
	pst = i->c;
	++i;
	++n;

	SVGPoint p;
	m = p.parse(i,end);
	if ( m != 2 ) return -1;
	i += 2;
	n += 2;
	pl.push_back( p );

	while ( i != end ) {
	    m = p.parse(i,end);
	    if ( m == 2 ) {
		i += 2;
		n += 2;
		pl.push_back( p );
	    }
	    else if ( m == 0 ) {
		// OK end of points.
		break;
	    }
	    else {
		// Should not happen.
		abort();
	    }
	}
	return n;
    }

    else if ( in_set( i->c, "CcSsQq" ) ) { // <X> (3xPoint)+
	int n = 0;
	int m = 0;
	int k;
	int k_max = 3;
	SVGPoint p[3];

	switch ( i->c ) {
	case 'C':
	case 'c':
 	    k_max = 3;
	    break;
	case 'S':
	case 's':
	case 'Q':
	case 'q':
	    k_max = 2;
	    break;
	default:
	    abort();
	};
	
	pst = i->c;
	++i;
	++n;


	k = SVGPoint::parse(i,end, k_max, p);
	if ( k != 2*k_max ) return -1;
	n += k;
	i += k;
	for ( k = 0; k < k_max; ++k )
	    pl.push_back( p[k] );

	while ( i != end ) {
	    if ( i->c != 0 ) break;
	    LLE(n);
	    k = SVGPoint::parse(i,end, k_max, p);
	    LLE(k);
	    if ( k != 2*k_max ) return -1;
	    n += k;
	    i += k;
	    for ( k = 0; k < k_max; ++k )
		pl.push_back( p[k] );
	}
	return n;
    }

    return 0;
}



typedef deque<SVGPathSection> SVGPathSectionList_t;

class SVGPath {
public:    
    SVGPath() {};

    int parse( const SVGPathTokens& pt);
    
    void clear() {
	psl.clear();
    };
    
    SVGPathSectionList_t	psl;

    ostream& cvs(ostream& os) const {
	SVGPathSectionList_t::const_iterator i;
	os << "{ ";
	for(i=psl.begin(); i != psl.end();++i ) {
	    os << *i << ' ';
	}
	os << '}';
	return os;
    };
};
ostream& operator<<( ostream& os, const SVGPath& x) {
    return x.cvs(os);
}

int
SVGPath::parse(const SVGPathTokens& pt)
{
    int rv = 0;
    SVGPathTokenList_t::const_iterator i = pt.tl.begin();
    SVGPathTokenList_t::const_iterator end = pt.tl.end();
    while ( i != end ) {
	SVGPathSection ps;
	int n;
	n = ps.parse( i, end );
	if ( n <= 0 )
	    break;
	i += n;
	rv=+n;
	psl.push_back( ps );
    }
    return rv;
    
}






class Context {
public:
    Context() : s(), prev(0) {};
    
    string s;

    Context* prev;
};



class MyVisitor : public XMLVisitor {
 public:
    MyVisitor();
    bool VisitEnter(const XMLDocument& x);
    bool VisitExit(const XMLDocument& x);
    bool VisitEnter(const XMLElement& x, const XMLAttribute* a);
    bool VisitExit(const XMLElement& x);
    bool Visit(const XMLDeclaration& x);
    bool Visit(const XMLText& x);
    bool Visit(const XMLComment& x);
    bool Visit(const XMLUnknown& x);

    void group(const XMLAttribute* a);
    void rect(const XMLAttribute* a);
    void path(const XMLElement& e, const XMLAttribute* a);

    
    unsigned lvl;

    Context* ctx;
    
 private:
    MyVisitor(const MyVisitor& );
    MyVisitor& operator=( const MyVisitor& );
};

MyVisitor::MyVisitor() : XMLVisitor(), lvl(0), ctx(0) {};

#define LL() do{cout<<__PRETTY_FUNCTION__<<endl;}while(0)
#define PP(X) do{for(unsigned i=(X);i;--i)cout << "  ";}while(0)

bool MyVisitor::VisitEnter(const XMLDocument& x)
{
    while ( ctx ) {
	Context* x = ctx;
	ctx = x->prev;
	delete x ;
    }
    ctx = new Context();
    
    lvl =0;
    cout << "-------------------------------------------------------" << endl
	 << "BEGIN Document (" << lvl << ")" << endl;
    ++lvl;
    return true;
}

bool MyVisitor::VisitExit(const XMLDocument& x)  
{
    if(lvl) --lvl;

    while ( ctx ) {
	Context* x = ctx;
	ctx = x->prev;
	delete x ;
    }

    cout << "END Document (" << lvl << ")" << endl
	 << "-------------------------------------------------------" << endl;
    return true;
}



void
MyVisitor::group(const XMLAttribute* a)
{    
    while (a) {
	const char* aname = a->Name();
	do {
	    if ( !strcmp(aname,"style") ) break;
	    if ( !strcmp(aname,"sodipodi:nodetypes") ) break;
	    
	    PP(lvl);
	    cout << a->Name() << "=" << '"' << a->Value() << '"' << endl;
	} while(0);
	a = a->Next();
    }
}


void
MyVisitor::rect(const XMLAttribute* a)
{
    while (a) {
	const char* aname = a->Name();
	do {
	    if ( !strcmp(aname,"style") ) break;
	    if ( !strcmp(aname,"sodipodi:nodetypes") ) break;
	    
	    PP(lvl);
	    cout << a->Name() << "=" << '"' << a->Value() << '"' << endl;
	} while(0);
	a = a->Next();
    }
}



void
MyVisitor::path(const XMLElement& e, const XMLAttribute* a)
{
    const char* str = e.Attribute("d");

    // Temp, print attributes.
    while (a) {
	const char* aname = a->Name();
	do {
	    if ( !strcmp(aname,"style") ) break;
	    if ( !strcmp(aname,"sodipodi:nodetypes") ) break;
	    
	    PP(lvl);
	    cout << a->Name() << "=" << '"' << a->Value() << '"' << endl;
	} while(0);
	a = a->Next();
    }

    // Handle the path data
    if( str ) {
	SVGPathTokens pt;
	pt.parse( str );
	cout << "d=" << '"' << str << '"' << endl
	     << "path tokens = " << pt << endl;

	SVGPath p;
	p.parse( pt );
	cout << "path = " << p << endl;
    }
}

void
remove_ws( string& s )
{
    string::iterator i = s.begin();
    while ( i != s.end() ) {
	if ( isspace( *i ) )
	    *i = '_';
	++i;
    }
}


bool MyVisitor::VisitEnter(const XMLElement& x, const XMLAttribute* a)
{
    const char* name = x.Name();

    PP(lvl);
    cout << "BEGIN " << ctx->s << " " << x.Name() << endl;
    ++lvl;

    if ( !strcmp(name,"g") ) {
	Context* new_ctx = new Context();
	new_ctx->s = ctx->s;
	if  ( !ctx->s.empty() )
	    new_ctx->s += "_";
	const char* id = x.Attribute("inkscape:label");
	if ( !id)
	    id = x.Attribute("id");
	if ( id )
	    new_ctx->s += id;
	new_ctx->prev= ctx;
	ctx = new_ctx;
	remove_ws( ctx->s );
	group(a);
    }
    else if ( !strcmp(name,"rect") ) {
	rect(a);
    }
    else if ( !strcmp(name,"path") ) {
	path(x,a);
    }
    else if ( !strcmp(name,"svg") ||
	      !strcmp(name,"sodipodi:namedview") ||
	      !strcmp(name,"inkscape:grid") ||
	      !strcmp(name,"text") ||
	      !strcmp(name,"tspan") ||
	      !strcmp(name,"defs")
	      ) {
	// IGNORE
    }
    else {
	while (a) {
	    PP(lvl);
	    cout << a->Name() << "=" << '"' << a->Value() << '"' << endl;
	    a = a->Next();
	}	
    }
    return true;
}

bool MyVisitor::VisitExit(const XMLElement& x)
{
    const char* name = x.Name();
    if ( !strcmp(name,"g") ) {
	if ( ctx) {
	    Context* x = ctx;
	    ctx = x->prev;
	    delete( x );
	}
    }
    --lvl;
    PP(lvl);
    cout << "END " << ctx->s << " " << x.Name() << endl;
    return true;
}

bool MyVisitor::Visit(const XMLText& x) 
{
    PP(lvl);
    cout << '"' << x.Value()  << '"' << endl;
    return true;
}


bool MyVisitor::Visit(const XMLDeclaration& x)
{
    return true;
}

bool MyVisitor::Visit(const XMLComment& x) 
{
    return true;
}

bool MyVisitor::Visit(const XMLUnknown& x) 
{
    return true;
}




int
main(int argc, const char** argv)
{
    XMLDocument doc;
    XMLError err;
    MyVisitor myvisitor;
    XMLPrinter printer( stdout);
    
    if ( argc != 2 ) {
	cout << "Argument missing." << endl;
	exit(-1);
    }
    
    cout << "Loading:" << argv[1] << endl;
    err = doc.LoadFile( argv[1] );
    cout<< "err = " << err << endl;
    cout << "doc======================================================"<< endl;
  // doc.Accept( &printer );
  // cout << "========================================================="<< endl;
    doc.Accept( &myvisitor );
    cout << "========================================================="<< endl;
    
    return 0;
}
