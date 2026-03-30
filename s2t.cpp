//                              -*- Mode: C++ -*- 
// Copyright (C) 2026, Mats
// 
// File name       : s2t.cpp
// Description     : svg2scad using text in svg file.
// 
// Author          : Mats
// Created On      : Wed Mar 11 17:49:53 2026
// 
// Last Modified By: Mats
// Last Modified On: Mon Mar 30 19:59:25 2026
// Update Count    : 243
// 



#include "s2t.hpp"
#include "str.hpp"

#include <map>
#include <sstream>



// -----------------------------------------------------------------------------
// Internal S2T

class S2TVisitor;

typedef map<string,string> S2T_Setting_Map_t;
typedef map<string,string> S2T_Setting_MapIter_t;

class S2T_Setting : public S2T_Setting_Map_t {
public:
    int add( const string& str );

    bool has(const char* key) const;
    const string& get(const char* key, const char* def_val) const;
    double get_d(const char* key, double def_val) const;

};


class S2T_svg {				// Data related to the svg element
public:
    double width;
    double height;

    S2T_svg() : width(210), height(297) {}; // Default is A4 page.
};


class S2T_ctx {				// Context (group)
public:
    string name;
    bool transform_placeholder;
    bool is_visible;
};


class S2T_Text {
public:
    enum Kind_e {
	TK_None = 0,
	TK_Settings,
	TK_Code,
	TK_MAX
    };
    StrList str;
    Kind_e kind;
    XYPoint pos;

    S2T_Text(XYPoint p = XYPoint(0,0) ) : str(), kind(TK_None), pos(p) {};
};

class S2T_Path {
public:
    XYPath pth;
};

typedef deque<S2T_ctx> S2T_ctx_list_t;
typedef S2T_ctx_list_t::const_iterator S2T_ctx_list_iter_t;

typedef deque<S2T_Text> S2T_Text_list_t;
typedef S2T_Text_list_t::const_iterator S2T_Text_list_iter_t;
typedef S2T_Text_list_t::iterator S2T_Text_list_viter_t;

typedef deque<S2T_Path> S2T_Path_list_t;
typedef S2T_Path_list_t::const_iterator S2T_Path_list_iter_t;
typedef S2T_Path_list_t::iterator S2T_Path_list_var_iter_t;


class S2T : public S2S {
public:
    S2TVisitor* visitor;

    S2T_svg svg_data;
    S2T_ctx_list_t ctx_list;
    S2T_Text_list_t textl;
    S2T_Path_list_t pathl;
    
    bool in_text;

    enum OriginVal {
	svg_val,
	scad_val
    };
    OriginVal origin;
    
    ~S2T();

    S2T();
    
    virtual int visit( const XMLDocument& doc );
    virtual int compile();
    virtual int unparse( ostream& ost );


    bool do_svg_start(const XMLElement& x, const XMLAttribute* a);
    bool do_svg_end(const XMLElement& x);
    bool do_g_start(const XMLElement& x, const XMLAttribute* a);
    bool do_g_end(const XMLElement& x);
    bool do_text_start(const XMLElement& x, const XMLAttribute* a);
    bool do_text_end(const XMLElement& x);
    bool do_text(const XMLText& x);
    bool do_path_start(const XMLElement& x, const XMLAttribute* a);
    bool do_path_end(const XMLElement& x);

    void push_ctx( const S2T_ctx& ctx );
    void pop_ctx();

    void handle_directives();

    bool handle_metavar_min(const string& arg, string& val);
    bool handle_metavar_max(const string& arg, string& val);
    bool handle_metavar_path(const string& arg, string& val);
    bool handle_metavar_min2zero(const string& arg, string& val);    
    bool handle_metavar_max2zero(const string& arg, string& val);    
    bool handle_metavar_com2zero(const string& arg, string& val);    
    bool handle_metavar(const string& var, const string& arg, string& val);
    void handle_code( StrList& strlist );
    void handle_settings( StrList& strlist );

    void handle_setting_origin(const string& val);
    void handle_setting(const string& tag, const string& val);

    void apply_origin_scad();

    const S2T_Path* get_path( const string& name );

};



// -----------------------------------------------------------------------------
class S2TVisitor : public XMLVisitor {
public:
    S2T&	s2t;

    ~S2TVisitor();
    S2TVisitor(S2T& x);


    bool VisitEnter(const XMLDocument& x);
    //bool VisitExit(const XMLDocument& x);
    bool VisitEnter(const XMLElement& x, const XMLAttribute* a);
    bool VisitExit(const XMLElement& x);
    //bool Visit(const XMLDeclaration& x);
    bool Visit(const XMLText& x);
    //bool Visit(const XMLComment& x);
    //bool Visit(const XMLUnknown& x);

private:
    S2TVisitor(const S2TVisitor&);
    S2TVisitor& operator=(const S2TVisitor&);
};



S2TVisitor::~S2TVisitor()
{
    DLL();
}



S2TVisitor::S2TVisitor(S2T& x)
    : XMLVisitor(), s2t(x)
{
    DLL();
}


bool
S2TVisitor::VisitEnter(const XMLDocument& x)
{
    DLL();
    return true;
}


bool
S2TVisitor::VisitEnter(const XMLElement& x, const XMLAttribute* a)
{
    const char* name = x.Name();

    // Handle SVG Elements
    if ( !strcmp("svg",name) ) {
	return s2t.do_svg_start(x,a);
    }
    if ( !strcmp("g",name) ) {
	return s2t.do_g_start(x,a);
    }
    if ( !strcmp("text",name) ) {
	return s2t.do_text_start(x,a);
    }
    if ( !strcmp("tspan",name) ) {
	return true;
    }
    if ( !strcmp("path",name) ) {
	return s2t.do_path_start(x,a);
    }

    // Ignore Elements
    if ( !strcmp("sodipodi:namedview",name) ||
	 !strcmp("defs",name) ) {
	return true;
    }
    
    DD(name);
    return true;
}

bool
S2TVisitor::VisitExit(const XMLElement& x)
{
    const char* name = x.Name();

    // Handle SVG Elements
    if ( !strcmp("svg",name) ) {
	return s2t.do_svg_end(x);
    }
    if ( !strcmp("g",name) ) {
	return s2t.do_g_end(x);
    }
    if ( !strcmp("text",name) ) {
	return s2t.do_text_end(x);
    }
    if ( !strcmp("tspan",name) ) {
	return true;
    }
    if ( !strcmp("path",name) ) {
	return s2t.do_path_end(x);
    }

    // Ignore Elements
    if ( !strcmp("sodipodi:namedview",name) ||
	 !strcmp("defs",name) ) {
	return true;
    }
    
    DD(name);
    return true;
}

bool
S2TVisitor::Visit(const XMLText& x)
{
    DLL();
    s2t.do_text( x );
    return true;
}



// -----------------------------------------------------------------------------
S2T::~S2T()
{
    DLL();
    if ( visitor ) {
	delete visitor;
	visitor = 0;
    }
}



S2T::S2T()
    : svg_data(), in_text(false), origin( svg_val )
{
    DLL();
    visitor = new S2TVisitor(*this);
};



int
S2T::visit( const XMLDocument& doc )
{
    DLL();
    doc.Accept( visitor );

    return 0;
}



int
S2T::unparse( ostream& ost )
{
    DLL();

    unsigned n;

    ost << "// Generated by svg2scad. "
	<< "(https://github.com/mats-bergstrom/svg2scad)." << endl
	<< endl;

    ost << "// SVG Data"			<< endl
	<< "// width  = " << svg_data.width	<< endl
	<< "// height = " << svg_data.height	<< endl
	<< endl;

    ost << "// Settings" << endl
	<< "// origin = " << origin << endl
	<< endl;


    ost << "// Path elements" << endl;

    S2T_Path_list_iter_t pi = pathl.begin();
    S2T_Path_list_iter_t pi_max = pathl.end();

    n = 0;
    while ( pi != pi_max ) {
	string path_name = "s2s_";
	path_name += pi->pth.name;
	    
	ost << "// Path  " << n << " \"" << pi->pth.name 
	    << "\" (" << pi->pth.seg.pl.size() << ')' << endl;

	ost << path_name << "_min = " << pi->pth.P_min << ';' << endl
	    << path_name << "_max = " << pi->pth.P_max << ';' << endl;
	
	ost << path_name << "_path = [" << endl;

	XYPointListIter_t i     = pi->pth.seg.pl.begin();
	XYPointListIter_t i_end = pi->pth.seg.pl.end();
	while ( i != i_end ) {
	    ost << '\t' << *i;
	    ++i;
	    if ( i!=i_end )
		ost << ',';
	    ost << endl;
	}
	ost << "];" << endl
	    << endl;

	++pi;
    }

    ost << endl;
    


    S2T_Text_list_iter_t ti = textl.begin();
    S2T_Text_list_iter_t ti_max = textl.end();

    n = 0;
    while ( ti != ti_max ) {
	if ( ti->kind == S2T_Text::TK_Code ) {
	    ost << "// Code Element " << n << endl
		<< "// p = " << ti->pos		<< endl
		<< ti->str
		<< endl
		<< endl;
	}
	++ti;
	++n;
    }

    return 0;
}




static double
cvt_size(const char* attr, const char* val, double defval )
{
    if ( !val || !*val ) {
	cout << "Warning: svg " << attr << " width attribute missing!" << endl;
	return defval;
    }

    string str = val;
    size_t pos = str.find("mm");
    if ( pos == string::npos ) {
	cout << "Warning: svg units not mm!" << endl;	    
	return defval;
    }

    str.erase( pos, 2 );

    double d = std::strtod(str.c_str(),0);

    if ( opt_d ) {
	cout << "\t\t" << attr << "=" << d << '(' << val <<')' << endl;
    }
    return d;
}


static void
print_attributes( const XMLAttribute* a)
{
    while (a) {
	cout << '\t' << a->Name() << "=" << '"' << a->Value() << '"'
	     << endl;
	a = a->Next();
    }
}

bool
S2T::do_svg_start(const XMLElement& x, const XMLAttribute* a)
{
    DLL();
    if ( 0&& opt_d )
	print_attributes(a);

    svg_data.width  = cvt_size( "width", x.Attribute("width"), svg_data.width );
    svg_data.height = cvt_size( "height",x.Attribute("height"),svg_data.height);

    S2T_ctx ctx;
    ctx.name = "svg0";
    ctx.transform_placeholder = true;
    ctx.is_visible = true;

    push_ctx( ctx );
    
    // Also clear text collection variables
    in_text = false;
    textl.clear();
    
    return true;
}

bool
S2T::do_svg_end(const XMLElement& x)
{
    DLL();
    pop_ctx();

    // Add check of context stack
    // Add check of text collection
    
    return true;
}



bool
S2T::do_g_start(const XMLElement& x, const XMLAttribute* a)
{
    DLL();
    if ( 0&& opt_d )
	print_attributes(a);

    S2T_ctx ctx;

    const char* s = x.Attribute( "inkscape:label" );
    if ( !s )
	s = x.Attribute( "id" );

    ctx.name = s;
    ctx.transform_placeholder = true;
    ctx.is_visible = true;

    if ( !ctx_list.empty() )
	ctx.is_visible &= ctx_list.front().is_visible;
    
    
    push_ctx( ctx );
    
    return true;
}

bool
S2T::do_g_end(const XMLElement& x)
{
    DLL();

    pop_ctx();
    
    return true;
}

bool
S2T::do_path_start(const XMLElement& x, const XMLAttribute* a)
{
    DLL();
    if ( opt_d )
	print_attributes(a);

    S2T_Path path;
    
    const char* s = x.Attribute( "inkscape:label" );
    if ( !s )
	return true;

    path.pth.name = s;

    const char* d_str = x.Attribute("d");
    if ( !d_str ) {
	cout << "Warning: Path " << path.pth.name << " without path data."
	     << endl;
	return false;
    }

    int i = path.pth.set( d_str );
    if ( !i ) {
	cout << "Warning: "
	     << __PRETTY_FUNCTION__
	     << " Empty path \"" << path.pth.name << "\"" << endl;
	return false;
    }


    pathl.push_back( path );

    if ( opt_d ) {
	cout << "\t\tPath: \"" << path.pth.name << "\" len="
	     << path.pth.seg.pl.size() << endl;
    }

    return true;
}

bool
S2T::do_path_end(const XMLElement& x)
{
    DLL();

    return true;
}

bool
S2T::do_text_start(const XMLElement& x, const XMLAttribute* a)
{
    DLL();
    if ( opt_d )
	print_attributes(a);

    // Clear text_val and set in_text=true to direct all text data
    // into text_val until we reach end tag.
    if ( in_text ) {
	cout << "Warning: Text in Text" << endl;
    }
    in_text = true;
    textl.push_back( S2T_Text() );

    // Add position to text object.

    return true;
}


bool
S2T::do_text(const XMLText& x)
{
    DLL();

    const char* s = x.Value();
    if ( s && in_text && !textl.empty() ) {
	S2T_Text& t = textl.back();
	bool directive_initial_line = false;
	
	if ( t.kind == S2T_Text::TK_None ) {
	    if ( !strcmp(s,"$settings") ) {
		t.kind = S2T_Text::TK_Settings;
		directive_initial_line = true;
	    }
	    else if ( !strcmp(s,"$code") ) {
		t.kind = S2T_Text::TK_Code;
		directive_initial_line = true;
	    }
	}

	if ( !directive_initial_line)
	    t.str.push_back( string(s) );

	if ( opt_d) {
	    cout << "TXT: " << s << endl;
	}
    }
    
    return true;
}



bool
S2T::do_text_end(const XMLElement& x)
{
    DLL();
    if ( opt_d ) {

	cout << "\tTEXT: Start" << endl
	     << textl.back().str << endl
	     << "\tTEXT: End" << endl;
    }
    
    in_text = false;
    
    return true;
}


void
S2T::push_ctx( const S2T_ctx& ctx )
{
    ctx_list.push_front(ctx);
    if ( opt_d ) {
	S2T_ctx_list_iter_t i     = ctx_list.begin();
	S2T_ctx_list_iter_t i_max = ctx_list.end();
	cout << "\t\t[";
	while ( i != i_max ) {
	    cout << '{' << i->name <<','<< i->is_visible<< '}';
	    ++i;
	    if ( i != i_max )
		cout << ',';
	}
	cout << ']' << endl;
    }
}

void
S2T::pop_ctx()
{
    ctx_list.pop_front();
}



void
S2T::handle_setting_origin(const string& val)
{
    if ( !val.compare("svg") ) {
	if ( opt_v )
	    cout << "Using origin svg." << endl;
	origin = svg_val;
    }
    else
    if ( !val.compare("scad") ) {
	if ( opt_v )
	    cout << "Using origin scad." << endl;
	origin = scad_val;
    }
    else {
	cout << "Warning: Invalid origin value \"" << val << "\" Ignored!"
	<< endl;
    }
}



void
S2T::handle_setting(const string& tag, const string& val)
{
    if ( opt_d )
	cout << "Setting: " << '"' << tag << '"'
	     <<       " : " << '"' << val << '"' << endl;

    if (!tag.compare("origin") ) {
	handle_setting_origin( val );
    }

    else {
	cout << "Warning: bad setting \""
	     << tag << ':' << val
	     << "\" not recognised."
	     << endl;
    }
}



void
S2T::handle_settings( StrList& strlist )
{
    string_list_citer_t i = strlist.begin();
    string_list_citer_t i_max = strlist.end();
    while ( i != i_max ) {
	string str = *i;
	size_t pos = str.find("//");
	if ( pos != string::npos ) {
	    str.erase(pos);
	}
	StrList::chomp(str);	
	TagValMarker tvm;
	bool b = tvm.find( str );
	while ( b ) {
	    string tag = str.substr(tvm.t0_pos, (tvm.t1_pos-tvm.t0_pos)+1 );
	    string val = str.substr(tvm.v0_pos, (tvm.v1_pos-tvm.v0_pos)+1 );
	    handle_setting(tag,val);
	    
	    str.erase(0,tvm.v1_pos+1);
	    b = tvm.find( str );
	}
	
	++i;
    }
}



const S2T_Path*
S2T::get_path( const string& name )
{
    S2T_Path_list_iter_t i = pathl.begin();
    S2T_Path_list_iter_t i_max = pathl.end();
    while ( i != i_max ) {
	if ( i->pth.name == name ) {
	    return &(*i);
	}
	++i;
    }
    return 0;
}


bool
S2T::handle_metavar_path( const string& arg, string& val )
{
    const S2T_Path* path = get_path( arg );
    if ( !path )
	return false;

    val = "s2s_";
    val += path->pth.name;
    val += "_path";
    return true;
}


bool
S2T::handle_metavar_min( const string& arg, string& val )
{
    const S2T_Path* path = get_path( arg );
    if ( !path )
	return false;

    val = "s2s_";
    val += path->pth.name;
    val += "_min";
    return true;
}


bool
S2T::handle_metavar_max( const string& arg, string& val )
{
    const S2T_Path* path = get_path( arg );
    if ( !path )
	return false;

    val = "s2s_";
    val += path->pth.name;
    val += "_max";
    return true;
}


static void
str_translate(string& str, double x, double y, double z)
{
    ostringstream oss;
    oss << "translate([" << x << ',' << y << ',' << z << "])";
    str = oss.str();
}

bool
S2T::handle_metavar_min2zero( const string& arg, string& val )
// $min2zero(path) --> translate([-x_min,-y_min,0])
{
    const S2T_Path* path = get_path( arg );
    if ( !path )
	return false;

    str_translate( val, -path->pth.P_min.x, -path->pth.P_min.y, 0);

    return true;
}

bool
S2T::handle_metavar_max2zero( const string& arg, string& val )
// $min2zero(path) --> translate([-x_min,-y_min,0])
{
    const S2T_Path* path = get_path( arg );
    if ( !path )
	return false;

    str_translate( val, -path->pth.P_max.x, -path->pth.P_max.y, 0);

    return true;
}


bool
S2T::handle_metavar_com2zero( const string& arg, string& val )
// $min2zero(path) --> translate([-x_min,-y_min,0])
{
    const S2T_Path* path = get_path( arg );
    if ( !path )
	return false;

    double x = (path->pth.P_max.x + path->pth.P_min.x)/2;
    double y = (path->pth.P_max.y + path->pth.P_min.y)/2;
    
    str_translate( val, -x, -y, 0);

    return true;
}



bool
S2T::handle_metavar( const string& var, const string& arg, string& val )
{
    if ( ! var.compare("path") )	return handle_metavar_path(arg,val);
    if ( ! var.compare("min") ) 	return handle_metavar_min(arg,val);
    if ( ! var.compare("max") ) 	return handle_metavar_max(arg,val);
    if ( ! var.compare("min2zero") ) 	return handle_metavar_min2zero(arg,val);
    if ( ! var.compare("max2zero") ) 	return handle_metavar_max2zero(arg,val);
    if ( ! var.compare("com2zero") ) 	return handle_metavar_com2zero(arg,val);
   return false;
}



void
S2T::handle_code( StrList& strlist )
{
    string_list_iter_t i = strlist.begin();
    string_list_iter_t i_max = strlist.end();
    while ( i != i_max ) {
	MetaVariableMarker mvm;
	bool b;
	do {
	    b = mvm.find( *i );
	    //cout << '"' << *i << '"';
	    if ( b ) {
		//cout << " : " << mvm;

		string var = i->substr( mvm.d_pos+1, (mvm.l_pos-mvm.d_pos-1) );
		string arg = i->substr( mvm.l_pos+1, (mvm.r_pos-mvm.l_pos-1) );

		//cout << " : \"" << var << "\" \"" << arg << '"';

		string val;
		bool valid = handle_metavar( var, arg, val );
		if ( !valid ) {
		    cout << "Warning: Invalid metavaraible in \""
			 << *i << '"' << endl;
		    //cout << " INVALID!";
		    break;
		}
		else {
		    //cout << "= \"" << val << '"' << endl;

		    // Replace the metavar with new value
		    i->replace( mvm.d_pos, mvm.r_pos+1-mvm.d_pos, val );
		    //cout << '"' << *i << '"';
		}
	    }
	//cout << endl;
	} while(b);
	++i;
    }

}



void
S2T::apply_origin_scad()
// change all y-values to y_new = page_height - y_old
{
    if ( opt_d )
	cout << "Apply origin scad..." << endl;
    S2T_Path_list_var_iter_t pi = pathl.begin();
    S2T_Path_list_var_iter_t pi_max = pathl.end();
    while ( pi != pi_max ) {
	XYPointListVarIter_t i     = pi->pth.seg.pl.begin();
	XYPointListVarIter_t i_end = pi->pth.seg.pl.end();
	while ( i != i_end ) {
	    i->y = svg_data.height - i->y;
	    ++i;
	}
	pi->pth.calc_bb();
	++pi;
    }
}



int
S2T::compile()
{
    S2T_Text_list_viter_t i;
    S2T_Text_list_viter_t i_max;

    i = textl.begin();
    i_max = textl.end();
    while ( i != i_max ) {
	if ( i->kind == S2T_Text::TK_Settings ) {
	    i->str.chomp();
	    handle_settings( i->str );
	}
	++i;
    }

    // 
    switch ( origin ) {
    case svg_val:
	// Do not do anything.
	break;
    case scad_val:
	apply_origin_scad();
	break;
    }

    i = textl.begin();
    i_max = textl.end();
    while ( i != i_max ) {
	if ( i->kind == S2T_Text::TK_Code ) {
	    handle_code( i->str );
	}
	++i;
    }

    return 0;
}



S2S*
S2T_Create()
{
    S2S* p = new S2T();
    return p;
}







// -----------------------------------------------------------------------------
// Visitor related classes




#if 0
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



void pN( const XMLNode* n )
{
    if ( !n )
	return;
    const XMLElement* e = n->ToElement();
    if ( e ) {
	const char* name = e->Name();

	cout << "E: " << name << endl;
    }
}



void
DoElement( const XMLElement* x )
{
    const char* name = x->Name();
    
}



void
s2t_func( const XMLDocument& doc, const string& ofnam )
{
    if( opt_d )
	cout << "=====================================================" << endl;

    const XMLElement* root = doc.RootElement();

    pN( root );

    
    if( opt_d )
	cout << "=====================================================" << endl;
}
#endif
