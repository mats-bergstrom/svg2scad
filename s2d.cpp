//                              -*- Mode: C++ -*- 
// Copyright (C) 2026, Mats
// 
// File name       : s2d.cpp
// Description     : svg2scad using text in svg file.
// 
// Author          : Mats
// Created On      : Wed Mar 11 17:49:53 2026
// 
// Last Modified By: Mats
// Last Modified On: Sat Mar 28 14:06:37 2026
// Update Count    : 43
// 



#include "str.hpp"
#include "s2d.hpp"


// -----------------------------------------------------------------------------
// Misc


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




// -----------------------------------------------------------------------------
// S2D is private

class s2D;



// -----------------------------------------------------------------------------
// Visitor related classes



class MyVisitor : public XMLVisitor {
 public:
    unsigned	lvl;			// Hierarchical level of elements

    s2D*	s2d;
    

    // - - - - - - - - - - - - - - -
    ~MyVisitor();
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

// -----------------------------------------------------------------------------








// -----------------------------------------------------------------------------
// Classes to parse SVG and hold SVG data


ostream& operator<<(ostream& os, const SVGPathToken& x) {
    return x.cvs(os);
}



ostream& operator<<(ostream& os, const SVGPathTokens& x) {
    return x.cvs(os);
}


inline void
skip_ws_c( const char* &s )
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
	skip_ws_c(s);
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



ostream& operator<<( ostream& os, const XYPoint& x) {
    return x.cvs(os);
}

bool operator== (const XYPoint& a, const XYPoint& b) {
    return ((a.x == b.x) && (a.y==b.y));
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



void
XYSegment::append(const XYPoint& p)
{
    if ( !pl.empty() ) {
	if ( pl.back() == p ) {
	    if ( opt_d ) {
		cout << "pl.back() == p : " << pl.back() << "==" << p << endl;
	    }
	    return;
	}
	else if ( opt_d ) {
		cout << "pl.back() != p : " << pl.back() << "!=" << p << endl;
	}
    }
    pl.push_back( p );
}



void
XYSegment::remove_duplicates()
{
    if ( type == s2D_ST_XYPath ) {
	XYPointListIter_t i =   pl.begin();
	XYPointListIter_t i_end = pl.end();
	XYPointListIter_t p = i;
	if ( i != i_end ) {
	    ++i;
	}
	while ( i != i_end ) {
	    if ( *i == *p )
		pl.erase(p);
	    p = i;
	    ++i;
	};
	    
    }
    else {
	// Ignore for now.
    }
}





class s2D {
    // svg2scad 2D data.
    // . namespace stack
    // . path list
public:
    typedef deque<string>  NSStack_t;	// Stack of all namespaces.

    NSStack_t		ns_stack;
    XYPath_List_t	path_list;

    void enter_namespace(const char* s);
    void exit_namespace();

    void add_path(const char* name, const char* d_str);

    ostream& unparse_scad(ostream& os) const;
};



ostream&
XYPath::unparse_scad( ostream& os ) const
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
    XYPath_List_t::const_iterator i = path_list.begin();
    XYPath_List_t::const_iterator i_max = path_list.end();

    os << "// Generated code, svg2scad 2D." << endl;
    while (i != i_max ) {
	i->unparse_scad( os );
	++i;
    }
    return os;
}



// -----------------------------------------------------------------------------

int
XYSegment::append( const XYSegment& seg)
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
	    append( cP ); // pl.push_back( cP );
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
	    append( cP ); //pl.push_back( cP );
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
	    append( *i ); //pl.push_back( *i );
	    ++i;
	}
	break;

    case s2D_ST_L:			// Lineto ABS
    case s2D_ST_M:			// Moveto ABS, As Lineto!
	while ( i != end ) {
	    append( *i ); //pl.push_back( *i );
	    ++i;
	}
	break;

    case s2D_ST_l:			// Lineto, rel
    case s2D_ST_m:			// Moveto Rel, as l!
	while ( i != end ) {
	    XYPoint p = *i;
	    p += cP;
	    append( p ); //pl.push_back( p );
	    ++i;
	    cP = p;
	}
	break;
	
    case s2D_ST_Z:			// Closepath
	if ( i != end ) {
	    cout << "Warning.  Z/z with points should not happend!" << endl;
	}
	if ( !pl.empty() ) {
	    append( pl.front() ); //pl.push_back( pl.front() );
	}
	else {
	    cout << "Warning: Closepath of empty path!" << endl;
	}
	break;

    case s2D_ST_H:
	while ( i != end ) {
	    XYPoint p = cP;
	    p.x = i->x;
	    append( p ); //pl.push_back( p );
	    ++i;
	}
	break;
	
    case s2D_ST_h:
	while ( i != end ) {
	    XYPoint p = cP;
	    p.x += i->x;
	    append( p ); //pl.push_back( p );
	    ++i;
	    cP = p;
	}
	break;

    case s2D_ST_V:
	while ( i != end ) {
	    XYPoint p = cP;
	    p.y = i->y;
	    append( p ); //pl.push_back( p );
	    ++i;
	}
	break;

    case s2D_ST_v:
	while ( i != end ) {
	    XYPoint p = cP;
	    p.y += i->y;
	    append( p ); //pl.push_back( p );
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
		append( p ); //pl.push_back( p );
	    }
	    append( P3 ); //pl.push_back( P3 );
	    
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

		append( p ); //pl.push_back( p );
	    }
	    append( P3 ); //pl.push_back( P3 );
	    cP = P3;
	}
	break;

    default:
	cout << "Path tag not implemented: " << seg.type << endl;
	break;
    }

    return 1;
}



int
XYPoint::parse(SVGPathTLIter_t i, SVGPathTLIter_t end)
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
XYPoint::parse(SVGPathTLIter_t i, SVGPathTLIter_t end,
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
XYSegment::parse(SVGPathTLIter_t i, SVGPathTLIter_t end)
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
XYPath::calc_bb()
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
XYPath::set( const char* s )
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
    XYSegmentListIter_t s_i = seg_list.begin();
    XYSegmentListIter_t s_end = seg_list.end();
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

    XYPath path;
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


MyVisitor::~MyVisitor()
{
    if (s2d)
	delete s2d;
    s2d=0;
}


MyVisitor::MyVisitor()
    : XMLVisitor(), lvl(0), s2d(0)
{
    s2d = new s2D;
    // EMPTY
};



bool
MyVisitor::VisitEnter(const XMLDocument& x)
{    
    DD( lvl );
    ++lvl;
    s2d->enter_namespace("s2d");
    return true;
}



bool
MyVisitor::VisitExit(const XMLDocument& x)  
{
    if(lvl) --lvl;
    DD( lvl );
    s2d->exit_namespace();
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
    s2d->enter_namespace( str );

    PLE(lvl, s2d->ns_stack.front() );
}



void MyVisitor::VisitExitGroup(const XMLElement& x)
{
    PLE(lvl, s2d->ns_stack.front() );
    if (lvl) --lvl;
    if ( opt_d ) {
	PP(lvl);
	cout << __PRETTY_FUNCTION__ << endl;
    }
    s2d->exit_namespace();
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

    s2d->add_path( path_name, d_str );
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
    return s2d->unparse_scad(os);
}



// -----------------------------------------------------------------------------
// Internal S2D 

class S2D : public S2S {
public:
    ~S2D();
    S2D() : myvisitor(0) { myvisitor=new MyVisitor(); };
    virtual int visit( const XMLDocument& doc );
    virtual int unparse( ostream& ost );

    MyVisitor* myvisitor;
    
};



S2D::~S2D()
{
    LL();
    if ( myvisitor ) {
	delete( myvisitor );
	myvisitor = 0;
    }
}


int
S2D::visit( const XMLDocument& doc )
{
    LL();
    doc.Accept( myvisitor );
    return 0;
}


int
S2D::unparse( ostream& ost )
{
    LL();
    myvisitor->unparse_scad( ost );
    return 0;
}




S2S*
S2D_Create()
{
    S2S* p = new S2D();
    return p;
}



