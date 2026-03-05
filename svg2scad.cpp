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
// Last Modified On: Thu Mar  5 20:46:47 2026
// Update Count    : 534
// 


#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <deque>
#include <cstdlib>
#include <cmath>

#include<tinyxml2.h>

using namespace std;
using namespace tinyxml2;


// -----------------------------------------------------------------------------
// Misc

int opt_d = 0;				// Debug prints.
int opt_v = 0;				// Verbose mode.
int opt_fixme = 0;			// Print FIXMEs.

#if 1
#define MYASSERT( P ) do{myassert(P,#P,__PRETTY_FUNCTION__);}while(0)
#else
#define MYASSERT( P ) do{}while(0)
#endif
void myassert(int predicate, const char* pstr, const char* s)
{
    if ( !predicate ) {
	cout << endl << "ASSERT(" << pstr << ") failed at " << s << endl;
	abort();
    }
}

#define FIXME(X) do{if(opt_fixme)cout<<"FIXME "<<__PRETTY_FUNCTION__<<" : "<<X<<endl;}while(0)


#define LLE(x) cout << #x << " = " << x << endl

#define LL() do{cout<<__PRETTY_FUNCTION__<<endl;}while(0)
#define PP(X) do{for(unsigned i=(X);i;--i)cout << "  ";}while(0)
#define DD(X) do{if(opt_d)cout<<__PRETTY_FUNCTION__<<" "<<#X<<"="<<X<<endl;}while(0)
#define PPD(L,X) do{if(opt_d){PP(L);DD(X);}}while(0)
#define PLE(L,X) do{if(opt_d){PP(L);LLE(X);}}while(0)

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

class SVGPathTokens {
public:
    SVGPathTokenList_t tl;

    // - - - - - - - - - - - - - - - - - - - - - - - - 
    SVGPathTokens() : tl() {};

    // Parse the path "d" attribute into a list of tokens.
    void parse( const char* s );
    
    ostream& cvs(ostream& os) const {
	SVGPathTLIter_t i;
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
// svg2scad 2D handling.

// Default step size in curves is 1mm
double opt_dL = 1.0;			// Step size in curves.


namespace s2s {
    // Namespace for general entities
   
    class XYPoint {
	// Used for both SVG Path segment points and XY Segments
    public:
	double x;
	double y;
	XYPoint(double ax=0, double ay=0) : x(ax), y(ay) {};

	int parse(SVGPathTLIter_t i,
		  SVGPathTLIter_t end);

	static int parse(SVGPathTLIter_t i,
			 SVGPathTLIter_t end,
			 unsigned N, XYPoint* p);

	XYPoint& operator += (const XYPoint& q) {
	    x += q.x;
	    y += q.y;
	    return *this;
	};
	XYPoint& operator -= (const XYPoint& q) {
	    x -= q.x;
	    y -= q.y;
	    return *this;
	};
	double abs() const {
	    double z = x*x + y*y;
	    z = sqrt( z );
	    return z;
	};

	ostream& cvs( ostream& os ) const {
	    return os << '[' << x << ',' << y << ']';
	};

    };
    typedef deque<XYPoint> XYPointList_t;
    typedef XYPointList_t::const_iterator XYPointListIter_t;

    
    ostream& operator<<( ostream& os, const XYPoint& x) {
	return x.cvs(os);
    }


    
    ostream& operator<<( ostream& os, const XYPointList_t& x) {
	XYPointListIter_t i = x.begin();
	XYPointListIter_t end = x.end();
	while ( i != end ) {
	    os << ' ' << *i;
	    ++i;
	}
	return os;
    }


    
    class XYSegment {
    public:
	// Used for both SVG Path Segments and XY Segments
	char		type;		// Segment type.
	XYPointList_t	pl;		// Point list.

	// Segment types.
	// XY Segments
#define s2D_ST_None	'?'	// Empty
#define s2D_ST_XYPath	'P'	// Path.  Abs. P+, As M, decoupled from SVG

	// SVG Segment types.
#define s2D_ST_M	'M'	// MoveTo Abs: P+
#define s2D_ST_m	'm'	// MoveTo Rel 
#define s2D_ST_Z	'Z'	// Closepath: -
#define s2D_ST_L	'L'	// Lineto Abs: P+
#define s2D_ST_l	'l'	// Lineto Rel
#define s2D_ST_H	'H'	// Horizontal LineTo Abs
#define s2D_ST_h	'h'	// Horizontal LineTo Rel
#define s2D_ST_V	'V'	// Vertical LineTo Abs
#define s2D_ST_v	'v'	// Vertical LineTo Rel
#define s2D_ST_C	'C'	// CubicCurveTo Abs: (P1 P2 P)+
#define s2D_ST_c	'c'	// CubicCurveTo Rel
#define s2D_ST_S	'S'	// Shorthand CubicCurveTo Abs: (P2 P)+
#define s2D_ST_s	's'	// Shorthand CubicCurveTo Rel
#define s2D_ST_Q	'Q'	// QuadraticCurveTo Abs: (P1 P)+
#define s2D_ST_q	'q'	// QuadraticCurveTo Rel
#define s2D_ST_T	'T'	// Shorthand QuadraticCurveTo Abs: P+
#define s2D_ST_t	't'	// Shorthand QuadraticCurveTo Rel
#define s2D_ST_A	'A'	// Elliptical Arc Abs: (R a f f f P)+
#define s2D_ST_a	'a'	// Elliptical Arc rel: (R a f f f P)+

	XYSegment(char t=s2D_ST_None) : type(t), pl() {};

	int parse(SVGPathTLIter_t i,
		  SVGPathTLIter_t end);

	// Append other segments to this and convert to XY Segments.
	int append(const XYSegment& seg); 

	ostream& cvs( ostream& os ) const {
	    return os << type << pl;
	}

	ostream& unparse_scad(ostream& os) const {
	    return os << pl;
	}

    };
    typedef deque<XYSegment> XYSegmentList_t;
    typedef XYSegmentList_t::const_iterator XYSegmentListIter_t;

    
    ostream& operator<<( ostream& os, const XYSegment& x) {
	return x.cvs(os);
    }

    

    ostream& operator<<( ostream& os, const XYSegmentList_t& x) {
	XYSegmentListIter_t i = x.begin();
	XYSegmentListIter_t end = x.end();
	os << '[';
	while (i != end) {
	    os << '[';
	    os << *i;
	    os << ']';
	    ++i;
	    if ( i != end )
		os << ", ";
	}
	os << ']';
	return os;
    }


    
    class XYPath {
	// An XY Path suitable to be unparsed to a scad file.
    public:
	string		name;
	XYSegment	seg;
	XYPoint		P_min;
	XYPoint		P_max;

	int set(const char* s);
	void calc_bb();
	
	ostream& unparse_scad(ostream& os) const;
    };
    typedef deque<XYPath> XYPath_List_t;


}



class s2D {
    // svg2scad 2D data.
    // . namespace stack
    // . path list
public:
    typedef deque<string>  NSStack_t;	// Stack of all namespaces.

    NSStack_t		ns_stack;
    s2s::XYPath_List_t	path_list;

    void enter_namespace(const char* s);
    void exit_namespace();

    void add_path(const char* name, const char* d_str);

    ostream& unparse_scad(ostream& os) const;
};



ostream&
s2s::XYPath::unparse_scad( ostream& os ) const
{
    XYPointListIter_t i = seg.pl.begin();
    XYPointListIter_t i_max = seg.pl.end();

    os << endl
       << "// " << name << " " << seg.pl.size() << " points." << endl;
    if ( i != i_max ) {
	os << name << "_min = " << P_min << ";" << endl;
	os << name << "_max = " << P_max << ";" << endl;

	os << name << "_pts =" << endl
	   << "\t[" << endl;
	while ( i != i_max ) {
	    os << "\t\t" << *i;
	    ++i;
	    if ( i != i_max )
		os << ",";
	    os << endl;
	}
	os << "\t];" << endl;
    }
    else {
	os << "// EMPTY!" << endl
	   << endl;
    }
    return os;
}


ostream&
s2D::unparse_scad(ostream& os) const
{
    s2s::XYPath_List_t::const_iterator i = path_list.begin();
    s2s::XYPath_List_t::const_iterator i_max = path_list.end();

    os << "// Generated code, svg2scad 2D." << endl;
    while (i != i_max ) {
	i->unparse_scad( os );
	++i;
    }
    return os;
}



// -----------------------------------------------------------------------------

int
s2s::XYSegment::append( const s2s::XYSegment& seg)
// Append an XYSegment to this segment transformed to an XYPath.
{
    if ( type != s2D_ST_XYPath ) {
	// *this must be an XYPath
	cout << "Warning append to non XYPath.  IGNORED!" << endl;
	return -1;
    }

    // Current point for the segment to add.
    // Use last point unless empty.
    XYPoint cP(0,0);
    if ( !pl.empty() )
	cP = pl.back();

    // If the segment to add is empty it must be a "closepath".
    if ( seg.pl.empty() && (seg.type != s2D_ST_Z) )
	return 0;

    
    XYPointListIter_t i = seg.pl.begin();
    XYPointListIter_t end = seg.pl.end();

    // M/m starts a new path segment.
    if ( (seg.type == s2D_ST_M) ||
	 (seg.type == s2D_ST_m) ) {
	if ( pl.empty() ) {
	    // If there is no other path segment just add initial point.
	    // Both M and m uses absolute coordinates in this case.
	    cP = *i;
	    pl.push_back( cP );
	    ++i;
	    // Then catch the rest of the points (if any) below.
	}
	else {
	    // Start of an additional path segment.
	    // This is not implemented correctly!
	    cout << "Warning: additional path segment." << endl;
	    if ( seg.type == s2D_ST_M) {
		cP = *i;
	    }
	    else {
		cP += *i;
	    }
	    // This is not handled corretly...
	    pl.push_back( cP );
	    ++i;
	}
    }

    // Points for Splines
    XYPoint P0, P1, P2, P3;
    
    switch ( seg.type ) {
    case s2D_ST_None:			// Ignore
	break;

    case s2D_ST_XYPath:			// Just append as is.
	while ( i != end ) {
	    pl.push_back( *i );
	    ++i;
	}
	break;

    case s2D_ST_L:			// Lineto ABS
    case s2D_ST_M:			// Moveto ABS, As Lineto!
	while ( i != end ) {
	    pl.push_back( *i );
	    ++i;
	}
	break;

    case s2D_ST_l:			// Lineto, rel
    case s2D_ST_m:			// Moveto Rel, as l!
	while ( i != end ) {
	    XYPoint p = *i;
	    p += cP;
	    pl.push_back( p );
	    ++i;
	    cP = p;
	}
	break;
	
    case s2D_ST_Z:			// Closepath
	if ( i != end ) {
	    cout << "Warning.  Z/z with points should not happend!" << endl;
	}
	if ( !pl.empty() ) {
	    pl.push_back( pl.front() );
	}
	else {
	    cout << "Warning: Closepath of empty path!" << endl;
	}
	break;

    case s2D_ST_H:
	while ( i != end ) {
	    XYPoint p = cP;
	    p.x = i->x;
	    pl.push_back( p );
	    ++i;
	}
	break;
	
    case s2D_ST_h:
	while ( i != end ) {
	    XYPoint p = cP;
	    p.x += i->x;
	    pl.push_back( p );
	    ++i;
	    cP = p;
	}
	break;

    case s2D_ST_V:
	while ( i != end ) {
	    XYPoint p = cP;
	    p.y = i->y;
	    pl.push_back( p );
	    ++i;
	}
	break;

    case s2D_ST_v:
	while ( i != end ) {
	    XYPoint p = cP;
	    p.y += i->y;
	    pl.push_back( p );
	    ++i;
	    cP = p;
	}
	break;

    case s2D_ST_C:			// Cubic Spline, Abs.
	while ( i != end ) {
	    P0 = cP;
	    P1 = *i;
	    ++i; if ( i == end ) break;
	    P2 = *i;
	    ++i; if ( i == end ) break;
	    P3 = *i;
	    ++i;

	    double t;
	    double dL = opt_dL;
	    XYPoint dP = P3;
	    dP -= P0;
	    double L = dP.abs();
	    unsigned N = ceil( L/dL ) + 1;
	    double dt = 1.0/N;

	    for ( t = dt; t <= 1; t += dt ) {
		double s = (1-t);
		double t2 = t*t;
		double s2 = s*s;
		XYPoint p;
		p.x = P0.x * s*s2 + 3*P1.x * t*s2 + 3*P2.x * t2*s + P3.x * t2*t;
		p.y = P0.y * s*s2 + 3*P1.y * t*s2 + 3*P2.y * t2*s + P3.y * t2*t;
		pl.push_back( p );
	    }
	    
	    cP = P3;
	}
	break;
	
    case s2D_ST_c:			// Cubic Spline, Rel.
	while ( i != end ) {
	    P0 = cP;
	    
	    P1 = *i;
	    ++i; if ( i == end ) break;
	    P2 = *i;
	    ++i; if ( i == end ) break;
	    P3 = *i;
	    ++i;

	    P1 += cP;
	    P2 += cP;
	    P3 += cP;

	    double t;
	    double dL = opt_dL;
	    XYPoint dP = P3;
	    dP -= P0;
	    double L = dP.abs();
	    unsigned N = ceil( L/dL ) + 1;
	    double dt = 1.0/N;

	    for ( t = dt; t < 1; t += dt ) {
		double s = (1-t);
		double t2 = t*t;
		double s2 = s*s;
		XYPoint p;
		p.x = P0.x * s*s2 + 3*P1.x * t*s2 + 3*P2.x * t2*s + P3.x * t2*t;
		p.y = P0.y * s*s2 + 3*P1.y * t*s2 + 3*P2.y * t2*s + P3.y * t2*t;

		pl.push_back( p );
	    }
	    pl.push_back( P3 );
	    cP = P3;
	}
	break;

    default:
	cout << "Path tag not implemented: " << seg.type << endl;
	break;
    }

    FIXME("Add S/s curve.");
    FIXME("Add Q/q curve.");
    FIXME("Add T/t curve.");
    FIXME("Add A/a curve.");

    return 1;
}



int
s2s::XYPoint::parse(SVGPathTLIter_t i, SVGPathTLIter_t end)
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
s2s::XYPoint::parse(SVGPathTLIter_t i, SVGPathTLIter_t end,
		    unsigned N, XYPoint* p)
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
s2s::XYSegment::parse(SVGPathTLIter_t i, SVGPathTLIter_t end)
{
    // We cannot start with a number.
    if ( i->c == 0 ) {
	type = s2D_ST_None;
	return 1;
    }

    else if ( in_set( i->c, "Zz" ) ) {
	// Not followed by a number.
	type = s2D_ST_Z;
	return 1;
    }
    
    else if ( in_set( i->c, "MmLlTtCcSsQq" ) ) { // <X> (3xPoint)+
	int n = 0;
	int m = 0;
	int k;
	int k_max = 3;
	XYPoint p[3];

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
	
	type = i->c;
	++i;
	++n;

	MYASSERT( k <= 3 );

	k = XYPoint::parse(i,end, k_max, p);
	if ( k != 2*k_max ) return -1;
	n += k;
	i += k;
	for ( k = 0; k < k_max; ++k )
	    pl.push_back( p[k] );

	while ( i != end ) {
	    if ( i->c != 0 ) break;
	    k = XYPoint::parse(i,end, k_max, p);
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
	XYPoint p;

	// Fixme replace to L or l
	type = i->c;
	++i;
	++n;

	if ( i->c != 0 )
	    return -1;

	if ( type == 'H' || type == 'h' ) {
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



void
s2s::XYPath::calc_bb()
// Calculate Bounding Box for the XYPath.
{
    P_min = XYPoint(0,0);
    P_max = XYPoint(0,0);

    XYPointListIter_t i = seg.pl.begin();
    XYPointListIter_t i_max = seg.pl.end();

    if ( i != i_max ) {
	P_min = *i;
	P_max = *i;
    }
    // 
    while ( i != i_max ) {
	if( i->x < P_min.x ) P_min.x = i->x;
	if( i->y < P_min.y ) P_min.y = i->y;
	if( i->x > P_max.x ) P_max.x = i->x;
	if( i->y > P_max.y ) P_max.y = i->y;
	++i;
    }
}



int
s2s::XYPath::set( const char* s )
{
    // 1: Tokenise the string.
    SVGPathTokens pt;
    pt.parse(s);

    if ( opt_d ) {
	cout << "\tParsing path" << endl
	     << "\t\tTokens = " << pt.tl.size() << endl
	     << "\t\td=" << '"' << s << '"' << endl
	     << "\t\tpath tokens = " << pt << endl;
    }

    if ( pt.tl.empty() )
	return 0;

    // 2: Parse the token string into segments and add to a segment list.
    XYSegmentList_t seg_list;
    SVGPathTLIter_t i = pt.tl.begin();
    SVGPathTLIter_t end = pt.tl.end();
    while ( i != end ) {
	XYSegment s;
	//	SVGPathSection ps;
	int n;
	n = s.parse( i, end );
	if ( n <= 0 )
	    break;
	i += n;
	seg_list.push_back( s );
    }

    if ( opt_d ) {
	cout << "\t\tpath segments = ";
	cout << seg_list << endl;
    }
    
    // Transform each segment to be one simple XYPath
    seg.type = s2D_ST_XYPath;
    seg.pl.clear();
    s2s::XYSegmentListIter_t s_i = seg_list.begin();
    s2s::XYSegmentListIter_t s_end = seg_list.end();
    while ( s_i != s_end ) {
	int j = seg.append( *s_i );
	if ( j < 0 )
	    break;
	++s_i;
    }

    calc_bb();
    
    if ( opt_d ) {
	cout << "\t\tXYPath = " << seg << endl;
    }
    
    return seg.pl.size();
}



void
s2D::add_path(const char* name, const char* d_str)
{
    string full_name;
    if ( !ns_stack.empty() ) {
	full_name = ns_stack.front();
	full_name += '_';
	full_name += name;
    }
    else {
	full_name = name;
    }
    DD(full_name);

    s2s::XYPath path;
    path.name = full_name;

    int i = path.set( d_str );

    if ( !i ) {
	cout << "Warning: "
	     << __PRETTY_FUNCTION__
	     << " Empty path \"" << full_name << "\"" << endl;
	return;
    }

    path_list.push_back( path );

    if ( opt_v ) {
	unsigned n;
	unsigned n_max = ns_stack.size() + 1;
	cout << "Path     : ";
	for ( n = 0; n < n_max; ++n ) cout << "  ";
	cout << path.name << '(' << path.seg.pl.size() << ')'<< endl;	
    }
}



void s2D::enter_namespace(const char* s)
{
    if ( !ns_stack.empty() ) {
	ns_stack.push_front( ns_stack.front() );
	if ( s ) {
	    ns_stack.front() += "_";
	    ns_stack.front() += s;
	}
    }
    else {
	if ( s ) {
	    ns_stack.push_front( s );
	}
	else {
	    ns_stack.push_front( "_" );
	}
    }
    if ( opt_v ) {
	unsigned n;
	unsigned n_max = ns_stack.size();
	cout << "Namespace: ";
	for ( n = 0; n < n_max; ++n ) cout << "  ";
	cout << ns_stack.front() << endl;
    }
}



void s2D::exit_namespace()
{
    if ( opt_d ) {
	if ( !ns_stack.empty() ) {
	    string namespace_str = ns_stack.front();
	    // DD(namespace_str);
	}
	else {
	    // cout << __PRETTY_FUNCTION__ << " POPPING EMPTY NS!" << endl;
	}
    }
    ns_stack.pop_front();
}



// -----------------------------------------------------------------------------
// Visitor related classes


class MyVisitor : public XMLVisitor {
 public:
    unsigned	lvl;			// Hierarchical level of elements

    s2D		s2d;
    

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

    void VisitEnterGroup(const XMLElement& x, const XMLAttribute* a);
    void VisitExitGroup(const XMLElement& x);

    void VisitEnterPath(const XMLElement& x, const XMLAttribute* a);
    void VisitExitPath(const XMLElement& x);

    ostream& unparse_scad(ostream& os) const;
    
 private:
    MyVisitor(const MyVisitor& );
    MyVisitor& operator=( const MyVisitor& );
};



MyVisitor::MyVisitor()
    : XMLVisitor(), lvl(0), s2d()
{
    // EMPTY
};



bool
MyVisitor::VisitEnter(const XMLDocument& x)
{    
    DD( lvl );
    ++lvl;
    s2d.enter_namespace("s2d");
    return true;
}



bool
MyVisitor::VisitExit(const XMLDocument& x)  
{
    if(lvl) --lvl;
    DD( lvl );
    s2d.exit_namespace();
    return true;
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



void MyVisitor::VisitEnterGroup(const XMLElement& x, const XMLAttribute* a)
{
    if ( opt_d ) {
	PP(lvl);
	cout << __PRETTY_FUNCTION__ << endl;
    }
    
    ++lvl;

    if ( opt_d ) {
	while (a) {
	    PP(lvl);
	    cout << a->Name() << "=" << '"' << a->Value() << '"' << endl;
	    a = a->Next();
	}
    }

    const char* str = x.Attribute("inkscape:label");
    s2d.enter_namespace( str );

    PLE(lvl, s2d.ns_stack.front() );
}



void MyVisitor::VisitExitGroup(const XMLElement& x)
{
    PLE(lvl, s2d.ns_stack.front() );
    if (lvl) --lvl;
    if ( opt_d ) {
	PP(lvl);
	cout << __PRETTY_FUNCTION__ << endl;
    }
    s2d.exit_namespace();
}



void MyVisitor::VisitEnterPath(const XMLElement& x, const XMLAttribute* a)
{
    if ( opt_d ) {
	PP(lvl);
	cout << __PRETTY_FUNCTION__ << endl;
    }
    
    ++lvl;
    
    if ( opt_d ) {
	while (a) {
	    PP(lvl);
	    cout << a->Name() << "=" << '"' << a->Value() << '"' << endl;
	    a = a->Next();
	}
    }

    const char* path_name = x.Attribute("inkscape:label");

    // Ignore paths w/o name.
    if ( !path_name )
	return;

    const char* d_str = x.Attribute("d");
    if ( !d_str ) {
	cout << "Warning: Path " << path_name << " without path data." << endl;
	return;
    }

    s2d.add_path( path_name, d_str );
}



void MyVisitor::VisitExitPath(const XMLElement& x)
{
    const char* group_id = x.Attribute("inkscape:label");
    if (lvl) --lvl;
    if ( opt_d ) {
	PP(lvl);
	DD(group_id);
    }
}



bool MyVisitor::VisitEnter(const XMLElement& x, const XMLAttribute* a)
{
    const char* name = x.Name();
    if ( !strcmp(name,"svg") ||
	 !strcmp(name,"sodipodi:namedview") ||
	 !strcmp(name,"inkscape:grid") ||
	 !strcmp(name,"text") ||
	 !strcmp(name,"tspan") ||
	 !strcmp(name,"defs")
	      ) {
	// IGNORE
	return true;
    }

    if ( !strcmp(name,"g") ) {		// group ------------------------------
	VisitEnterGroup(x,a);
    }
    else if ( !strcmp(name,"path") ) {	// path -------------------------------
	VisitEnterPath(x,a);
    }
    else {				// unknown ----------------------------
	PPD(lvl,name);
	cout << endl
	     << "Unrecognised element: " << name << endl;
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
    if ( !strcmp(name,"svg") ||
	      !strcmp(name,"sodipodi:namedview") ||
	      !strcmp(name,"inkscape:grid") ||
	      !strcmp(name,"text") ||
	      !strcmp(name,"tspan") ||
	      !strcmp(name,"defs")
	      ) {
	return true;
    }

    if ( !strcmp(name,"g") ) {
	VisitExitGroup(x);
    }
    else if ( !strcmp(name,"path") ) {
	VisitExitPath(x);
    }
    else {
	PP(lvl);
	cout << "Unrecognised element: " << name << endl;
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
MyVisitor::unparse_scad( ostream& os ) const
{
    return s2d.unparse_scad(os);
}




// -----------------------------------------------------------------------------

// TODO: Proper argument handling.

string argv0;

void
do_help()
{
    cout << argv0 << "[-h][-d][-v] <input>" << endl
	 << "\t-h		Print help. (This message)." << endl
	 << "\t-d		Turn on debug prints." << endl
	 << "\t-v		Turn on verbose mode." << endl
	 << "\t-dL <val>	Step in curves, default = 1.0" << endl;
}

int
main(int argc, const char** argv)
{
    XMLDocument doc;
    XMLError err;
    MyVisitor myvisitor;

    argv0 = *argv;
    ++argv;
    --argc;

    while ( argc ) {
	int n = 0;
	string arg( *argv );

	if ( !arg.compare("-h") ) {
	    do_help();
	    n = 1;
	    exit( EXIT_SUCCESS );
	}
	if ( !arg.compare("-d") ) {
	    opt_d = 1;
	    opt_v = 1;
	    n = 1;
	}
	if ( !arg.compare("-v") ) {
	    opt_v = 1;
	    n = 1;
	}

	if ( !arg.compare("-dL") ) {
	    if ( argc < 2 ) {
		cout << "Error: Argument for option -dL missing!" << endl;
		do_help();
		exit( EXIT_FAILURE );
	    }
	    opt_dL = strtod(argv[1],0);
	    if ( opt_dL <= 0 ) {
		cout << "Error: Incvvalid argument for option -dL missing!"
		     << endl;
		do_help();
		exit( EXIT_FAILURE );
	    }
	    n = 2;
	}
	
	if ( !n )
	    break;
	argv += n;
	argc -= n;
    };

    if ( !argc ) {
	cout << "Input file missing." << endl;
	do_help();
	exit(EXIT_FAILURE);
    }

    string ifname( *argv );
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
	cout << "Loading:" << ifname << endl;

    err = doc.LoadFile( ifname.c_str() );

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
    
    
    if ( opt_d ) {
	cout << "======================================================"<< endl;
    }
    
    myvisitor.unparse_scad( ofs );

    return 0;
}
