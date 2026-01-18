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
// Last Modified On: Sun Jan 18 13:32:59 2026
// Update Count    : 251
// 


#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <deque>
#include <cstdlib>

#include<tinyxml2.h>

using namespace std;
using namespace tinyxml2;


// -----------------------------------------------------------------------------
// Misc

int opt_d = 1;				// Debug prints.
int opt_v = 1;				// Verbose mode.


#if 1
#define MYASSERT( P ) do{myassert(P,#P,__PRETTY_FUNCTION__);}while(0)
#define DOUT if(1)cout
#else
#define MYASSERT( P ) do{}while(0)
#define DOUT if(0)cout
#endif
void myassert(int predicate, const char* pstr, const char* s)
{
    if ( !predicate ) {
	cout << endl << "ASSERT(" << pstr << ") failed at " << s << endl;
	abort();
    }
}



#define LLE(x) cout << #x << " = " << x << endl

#define LL() do{cout<<__PRETTY_FUNCTION__<<endl;}while(0)
#define PP(X) do{for(unsigned i=(X);i;--i)cout << "  ";}while(0)


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


// -----------------------------------------------------------------------------
// Classes to parse SVG and hold SVG data

class SVGPathToken {
    // Path Data token :: ([MmLl...] | number)
public:
    char c;				// 0 or one of MmLlHhVvCcZzQq...
    double d;				// number iff c==0

    // - - - - - - - - - - - - - - - - - - - - - - - - 
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
    
};

ostream& operator<<(ostream& os, const SVGPathToken& x) {
    return x.cvs(os);
}


typedef deque<SVGPathToken> SVGPathTokenList_t;
typedef SVGPathTokenList_t::const_iterator SVGPathTLIter_t;

class SVGPathTokens {			// TODO: Remove just use SVGPathTokenList_t
public:
    SVGPathTokenList_t tl;

    // - - - - - - - - - - - - - - - - - - - - - - - - 
    SVGPathTokens() : tl() {};

    // Parse the path "d" attribute into a list of tokens.
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

};






ostream& operator<<(ostream& os, const SVGPathTokens& x) {
    return x.cvs(os);
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



// -----------------------------------------------------------------------------
// Classes for internal SVG represenation

class SVGPoint {
public:
    SVGPoint(double ax=0, double ay=0) : x(ax), y(ay) {};
    double x;
    double y;

    // - - - - - - - - - - - - - - - - - - - - - - - - 
    int parse(SVGPathTokenList_t::const_iterator i,
	      SVGPathTokenList_t::const_iterator end);

    static int parse(SVGPathTokenList_t::const_iterator i,
		     SVGPathTokenList_t::const_iterator end,
		     unsigned N, SVGPoint* p);

    SVGPoint& operator += (const SVGPoint& q) {
	x += q.x;
	y += q.y;
	return *this;
    }
    
    ostream& cvs( ostream& os ) const {
	return os << '(' << x << ',' << y << ')';
    };
};


typedef deque<SVGPoint> SVGPointList_t;

ostream& operator<<( ostream& os, const SVGPoint& x) {
    return x.cvs(os);
}
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
    char pst;				// Type of path section PST_XXX
    SVGPointList_t pl;			// List of points of the path.

    // - - - - - - - - - - - - - - - - - - - - - - - - 
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
};





ostream& operator<<( ostream& os, const SVGPathSection& x) {
    return x.cvs(os);
}



int
SVGPoint::parse(SVGPathTokenList_t::const_iterator i,
		SVGPathTokenList_t::const_iterator end)
// Create a point from two numbers in a token list.
// Return no of tokens conusumed (0 or 2).
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
// Create N points from 2*N tokens in a token list.
// Returns no of tokens consumed, 0 or 2*N
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
    
    else if ( in_set( i->c, "MmLlTtCcSsQq" ) ) { // <X> (3xPoint)+
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
	case 'M':
	case 'm':
	case 'L':
	case 'l':
	case 'T':
	case 't':
	    k_max = 1;
	    break;
	default:
	    abort();
	};
	
	pst = i->c;
	++i;
	++n;

	MYASSERT( k <= 3 );

	k = SVGPoint::parse(i,end, k_max, p);
	if ( k != 2*k_max ) return -1;
	n += k;
	i += k;
	for ( k = 0; k < k_max; ++k )
	    pl.push_back( p[k] );

	while ( i != end ) {
	    if ( i->c != 0 ) break;
	    k = SVGPoint::parse(i,end, k_max, p);
	    if ( k != 2*k_max ) return -1;
	    n += k;
	    i += k;
	    for ( k = 0; k < k_max; ++k )
		pl.push_back( p[k] );
	}
	return n;
    }

    else if (in_set( i->c, "HhVv" )) {
	int n = 0;
	SVGPoint p;
	
	pst = i->c;
	++i;
	++n;

	if ( i->c != 0 )
	    return -1;

	if ( pst == 'H' || pst == 'h' ) {
	    p.x = i->d;
	    p.y = 0;
	}
	else {
	    p.x = 0;
	    p.y = i->d;
	}
	++i;
	++n;

	pl.push_back( p );
	
	return n;
    }
    else {
	abort();
    }

    return 0;
}



typedef deque<SVGPathSection> SVGPathSectionList_t;

class SVGPath {
public:
    string			name;
    SVGPathSectionList_t	psl;

    // - - - - - - - - - - - - - - - - - - - - - - - - 
    SVGPath() {};

    int parse( const SVGPathTokens& pt);
    
    void clear() {
	psl.clear();
    };

    
    ostream& print_scad( ostream& os ) const;
    
    ostream& cvs(ostream& os) const {
	SVGPathSectionList_t::const_iterator i;
	os << name << " { ";
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



// -----------------------------------------------------------------------------
// Visitor related classes


typedef deque<string>  NSStack_t;	// Stack of all namespaces.
typedef deque<SVGPath> PathList_t;	// List of all SVGPatchs.

class MyVisitor : public XMLVisitor {
 public:
    unsigned	lvl;			// Hierarchical level of elements
    NSStack_t	nss;			// Napespace Stack.
    PathList_t  pl;			// List of all SVGPaths.
    
    // List of paths here.

    // - - - - - - - - - - - - - - - 
    MyVisitor();
    bool VisitEnter(const XMLDocument& x);
    bool VisitExit(const XMLDocument& x);
    bool VisitEnter(const XMLElement& x, const XMLAttribute* a);
    bool VisitExit(const XMLElement& x);
    bool Visit(const XMLDeclaration& x);
    bool Visit(const XMLText& x);
    bool Visit(const XMLComment& x);
    bool Visit(const XMLUnknown& x);

    void print_group(const XMLAttribute* a);
    void print_rect(const XMLAttribute* a);
    void print_path(const XMLElement& e, const XMLAttribute* a);

    ostream& print_scad(ostream& os) const;
    
 private:
    MyVisitor(const MyVisitor& );
    MyVisitor& operator=( const MyVisitor& );
};



MyVisitor::MyVisitor()
: XMLVisitor(), lvl(0), nss()
{
    // EMPTY
};



bool
MyVisitor::VisitEnter(const XMLDocument& x)
{
    MYASSERT( nss.empty() );

    nss.push_front( string("") );
    lvl =0;

    if ( opt_d ) {
	DOUT << "------------------------------------------------------" << endl
	     << "BEGIN Document (" << lvl << ")" << endl;
    }
    
    ++lvl;
    return true;
}



bool
MyVisitor::VisitExit(const XMLDocument& x)  
{
    if(lvl) --lvl;

    nss.clear();

    if ( opt_d ) {
	DOUT << "END Document (" << lvl << ")" << endl
	     << "-----------------------------------------------------" << endl;
    }

    return true;
}



void
MyVisitor::print_group(const XMLAttribute* a)
{
    if ( !opt_d )
	return;
    
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
MyVisitor::print_rect(const XMLAttribute* a)
{
    if ( !opt_d )
	return;

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
MyVisitor::print_path(const XMLElement& e, const XMLAttribute* a)
{
    const char* str = e.Attribute("d");

    if ( opt_d ) {
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

    if ( opt_d ) {
	PP(lvl);
	cout << "BEGIN '" << nss.front() << "' " << x.Name() << endl;
    }
    
    ++lvl;

    if ( !strcmp(name,"g") ) {

	const char* id = x.Attribute("inkscape:label");
	if ( id ) {
	    string s = nss.front();
	    s += '_';
	    s += id;
	    remove_ws( s );
	    nss.push_front( string( s ) );
	}
	else{
	    // If this group is not a namespace, use the previous.
	    nss.push_front( nss.front() );
	}

	if ( opt_v ) {
	    PP(lvl);
	    cout << "group " << nss.front() << endl;
	}
	print_group(a);
    }

    else if ( !strcmp(name,"rect") ) {
	print_rect(a);
    }

    else if ( !strcmp(name,"path") ) {
	const char* id = x.Attribute("inkscape:label");
	const char* d_str = x.Attribute("d");
	if ( id && d_str ) {
	    SVGPathTokens pt;
	    pt.parse( d_str );
	    if ( opt_d )
		cout << "d=" << '"' << d_str << '"' << endl
		     << "path tokens = " << pt << endl;

	    SVGPath p;
	    p.name = nss.front();
	    while ( !p.name.empty() && p.name[0] == '_' )
		p.name = p.name.substr(1);
	    p.name += '_';
	    p.name += id;
	    p.parse( pt );

	    if ( !p.psl.empty() )
		pl.push_front( p );
		
	    if ( opt_d )
		cout << "path = " << p << endl;

	    if ( opt_v ) {
		PP(lvl);
		cout << "path " << id << " : " << pl.front().name
		     << " length=" << pl.front().psl.size() << endl;
	    }
	
	}
	print_path(x,a);
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
	if ( opt_v ) {
	    cout << "Unrecognised element: " << endl;
	    while (a) {
		PP(lvl);
		cout << a->Name() << "=" << '"' << a->Value() << '"' << endl;
		a = a->Next();
	    }
	}
    }
    return true;
}



bool MyVisitor::VisitExit(const XMLElement& x)
{
    const char* name = x.Name();
    if ( !strcmp(name,"g") ) {
	nss.pop_front();
    }
    --lvl;

    if ( opt_d ) {
	PP(lvl);
	cout << "END " << nss.front() << " " << x.Name() << endl;
    }
    
    return true;
}

bool MyVisitor::Visit(const XMLText& x) 
{
    if ( opt_d ) {
	PP(lvl);
	cout << '"' << x.Value()  << '"' << endl;
    }
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



// -----------------------------------------------------------------------------
// scad output

ostream&
SVGPath::print_scad( ostream& os ) const
{
    SVGPoint	cp;			// Current point.
    SVGPoint	ip;			// Initial Point
    SVGPoint    p;
    SVGPoint	p_min;
    SVGPoint	p_max;
    
    bool	has_ip = false;

    
    SVGPathSectionList_t::const_iterator i = psl.begin();
    SVGPathSectionList_t::const_iterator imax = psl.end();
    os << name << " =  [" << endl;
    while( i != imax ) {
	SVGPointList_t::const_iterator j =i->pl.begin();
	SVGPointList_t::const_iterator jmax =i->pl.end();
	while ( j != jmax ) {
	    p = *j;
	    if ( i->pst == PST_NONE ) {
		os << "  // '?'" << endl;
	    }
	    else if ( i-> pst == PST_Moveto ) {

	    }
	    else if ( i-> pst == PST_MovetoR ) {
		p += cp;
	    }
	    else {
		os << "  // " << i->pst << " NOT handled" << endl;		
	    }
	    
	    if ( !has_ip ) {
		has_ip = true;
		ip = p;
		p_min = p;
		p_max = p;
	    }
	    else {
		if ( p.x < p_min.x ) p_min.x = p.x;
		if ( p.y < p_min.y ) p_min.y = p.y;
		if ( p.x > p_max.x ) p_max.x = p.x;
		if ( p.y > p_max.y ) p_max.y = p.y;
	    }
	    
	    os << "  " << '[' << p.x << ',' << p.y << "],\t// " << i->pst << endl;

	    cp = p;
	    ++j;
	}
	cp = p;
	++i;
    }
    os << "];" << endl;
    os << name << "_min" << " = [" << p_min.x << ',' << p_min.y << "];" << endl;
    os << name << "_max" << " = [" << p_max.x << ',' << p_max.y << "];" << endl;
    return os;
}

ostream&
MyVisitor::print_scad( ostream& os ) const
{
    PathList_t::const_iterator i = pl.begin();
    PathList_t::const_iterator end = pl.end();
    while ( i != end ) {
	os << "// " << *i << endl;
	i->print_scad(os) << endl;
	++i;
    }
    return os;
}




// -----------------------------------------------------------------------------

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

    string ifname( argv[1] );
    string ofname;
    
    // Remove any initial path components
    size_t i = ifname.rfind( '/' );
    if ( i != string::npos ) {
	ofname = ifname.substr(i+1);
    }
    else {
	ofname = ifname;
    }
    ofname += ".scad";
    
    
    if ( opt_d )
	cout << "Loading:" << argv[1] << endl;

    err = doc.LoadFile( argv[1] );

    if ( opt_d ) {
	cout<< "err = " << err << endl;
	cout << "doc==================================================="<< endl;
    }

    doc.Accept( &myvisitor );

    ofstream ofs;
    ofs.open( ofname );
    if ( ofs.rdstate() & ofstream::failbit ) {
	cerr << "Failed to open " << ofname << endl;
	exit( EXIT_FAILURE );
    }
    
    myvisitor.print_scad( ofs );
    
    if ( opt_d ) {
	cout << "======================================================"<< endl;
    }
    
    return 0;
}
