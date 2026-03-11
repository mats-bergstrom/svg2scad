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
// Last Modified On: Wed Mar 11 18:09:16 2026
// Update Count    : 561
// 


// -----------------------------------------------------------------------------
// Misc Globals

int opt_d = 0;				// Debug prints.
int opt_v = 0;				// Verbose mode.
int opt_fixme = 0;			// Print FIXMEs.


#include "s2s.hpp"






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
