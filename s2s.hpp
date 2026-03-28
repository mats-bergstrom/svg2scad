//                              -*- Mode: C++ -*- 
// Copyright (C) 2026, Mats
// $Id$
// 
// File name       : s2s.hpp
// Description     : svg2scad 2D path
// 
// Author          : Mats
// Created On      : Wed Mar 11 17:52:07 2026
// 
// Last Modified By: Mats
// Last Modified On: Sat Mar 28 16:22:53 2026
// Update Count    : 36
// Status          : $State$
// 
// $Locker$
// $Log$
// 

#ifndef __S2S_HPP__
#define __S2S_HPP__ (1)


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


extern int opt_d;				// Debug prints.
extern int opt_v;				// Verbose mode.
extern int opt_fixme;				// Print FIXMEs.

extern double opt_dL;			// 




#define LLE(x) cout << #x << " = " << x << endl

#define LL() do{cout<<__PRETTY_FUNCTION__<<endl;}while(0)
#define DLL() do{if(opt_d)cout<<__PRETTY_FUNCTION__<<endl;}while(0)

#define PP(X) do{for(unsigned i=(X);i;--i)cout << "  ";}while(0)
#define DD(X) do{if(opt_d)cout<<__PRETTY_FUNCTION__<<" "<<#X<<"="<<X<<endl;}while(0)
#define PPD(L,X) do{if(opt_d){PP(L);DD(X);}}while(0)
#define PLE(L,X) do{if(opt_d){PP(L);LLE(X);}}while(0)






// -----------------------------------------------------------------------------
// Classes to parse SVG path and hold SVG data

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

ostream& operator<<(ostream& os, const SVGPathToken& x);


typedef deque<SVGPathToken> SVGPathTokenList_t;
typedef SVGPathTokenList_t::const_iterator SVGPathTLIter_t;


// -----------------------------------------------------------------------------

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

ostream& operator<<(ostream& os, const SVGPathTokens& x);


// -----------------------------------------------------------------------------

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
typedef XYPointList_t::iterator XYPointListVarIter_t;



ostream& operator<<( ostream& os, const XYPoint& x);
ostream& operator<<( ostream& os, const XYPointList_t& x);

bool operator== (const XYPoint& a, const XYPoint& b);



// -----------------------------------------------------------------------------

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

    // Append a point. Avoid Duplicates.
    void append(const XYPoint& p);
	
    void remove_duplicates();

    ostream& cvs( ostream& os ) const {
	return os << type << pl;
    }


    ostream& unparse_scad(ostream& os) const {
	return os << pl;
    }

};


typedef deque<XYSegment> XYSegmentList_t;
typedef XYSegmentList_t::const_iterator XYSegmentListIter_t;
typedef XYSegmentList_t::iterator XYSegmentListVarIter_t;


ostream& operator<<( ostream& os, const XYSegment& x);
ostream& operator<<( ostream& os, const XYSegmentList_t& x);



// -----------------------------------------------------------------------------

class XYPath {
    // An XY Path suitable to be unparsed to a scad file.
public:
    string		name;
    XYSegment		seg;
    XYPoint		P_min;
    XYPoint		P_max;

    int set(const char* s);
    void calc_bb();
	
    ostream& unparse_scad(ostream& os) const;
};
typedef deque<XYPath> XYPath_List_t;






// -----------------------------------------------------------------------------
// Generic Base for Svg 2 Scad.

class S2S {
public:
    virtual ~S2S();
    virtual int visit( const XMLDocument& doc );
    virtual int compile();
    virtual int unparse( ostream& ost );
};


#endif
