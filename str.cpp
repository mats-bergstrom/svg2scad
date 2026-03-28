//                              -*- Mode: C++ -*- 
// Copyright (C) 2026, Mats
// 
// File name       : str.cpp
// Description     : General String Massaging
// 
// Author          : Mats
// Created On      : Mon Mar 23 18:28:00 2026
// 
// Last Modified By: Mats
// Last Modified On: Sat Mar 28 15:48:39 2026
// Update Count    : 30
// 


#include "str.hpp"



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
    while ( in_set( *s, " \t\n\r" ) )
	++s;
}





void
StrList::chomp( string& str )
{
    size_t l = str.length();
    if ( !l )
	return;

    // Quick return if neither initial or last char is WS.
    if ( !isspace(str[0]) &&
	 !isspace(str[l-1]) )
	return;

    size_t i = 0;

    // Let str[i] be first non-WS
    while ( (i < l) && isspace(str[i]) ) {
	++i;
    }

    if ( i == l ) {
	str.clear();
	return;
    }

    // Let str[j] be last non-WS
    size_t j = l;
    --j;
    while ( (j > i) && isspace( str[j] ) ) {
	--j;
    }

    // ___abcdef...ghi___	str[i] and str[j] are first and last non-ws
    //    i          j		character.

    size_t len = (j+1)-i;		// Length of the remaining string
    str = str.substr(i,len);
}

void
StrList::chomp()
{
    string_list_iter_t i = begin();
    string_list_iter_t i_max = end();
    while ( i != i_max ) {
	chomp( *i );
	++i;
    }
}



ostream&
MetaVariableMarker::print(ostream& os) const
{
    if ( d_pos == string::npos ) {
	os << "{-,-,-}";
    }
    else {
	os << '{' << d_pos << ',' << l_pos << ',' << r_pos << "}";
    }
    return os;
}

void
MetaVariableMarker::clear()
{
    d_pos = string::npos;
    l_pos = string::npos;
    r_pos = string::npos;
}


bool
MetaVariableMarker::find( const string& str )
{
    d_pos = str.find( '$' );
    if ( d_pos != string::npos ) {
	l_pos = str.find( '(', d_pos );
	if ( l_pos != string::npos ) {
	    r_pos = str.find( ')', l_pos );
	    if ( r_pos != string::npos ) {
		return true;
	    }
	}
    }
    clear();
    return false;
}



// -----------------------------------------------------------------------------
// Tag Value marker.
// <tag> ':' <value> (EOL | ';')
// abc : def; ghi:klm	t0=a, t1=c, v0=d,v1=f ; 

ostream&
TagValMarker::print(ostream& os) const
{
    if ( t0_pos == string::npos ) {
	os << "{-,-,-,-}";
    }
    else {
	os << '{' << t0_pos << ',' << t1_pos << ','
	   <<        v0_pos << ',' << v1_pos << "}";
    }
    return os;
}


void
TagValMarker::clear()
{
    t0_pos = string::npos;
    t1_pos = string::npos;
    v0_pos = string::npos;
    v1_pos = string::npos;
}


bool
TagValMarker::find( const string& str )
{
    while ( !str.empty() ) {

	// Ignore initial WS.
	t0_pos = str.find_first_not_of( " \t\n\r" );
	if ( t0_pos == string::npos ) break;
	while ( str[t0_pos] == ';' ) {
	    ++t0_pos;
	    t0_pos = str.find_first_not_of( " \t\n\r", t0_pos );
	    if ( t0_pos == string::npos ) break;
	}
	if ( t0_pos == string::npos ) break;

	// Find the ':'
	t1_pos = str.find( ':', t0_pos);
	if ( t1_pos == string::npos ) break;

	// Find first non-space after ':'
	v0_pos = str.find_first_not_of( " \t\n\r", t1_pos+1 );
	if ( v0_pos == string::npos ) break;
	
	// Find EOL or ';'
	v1_pos = str.find( ';', v0_pos);
	if ( v1_pos == string::npos ) {
	    v1_pos = str.length();
	}

	// Move t1 backwards to point at last non-space.
	do {
	    --t1_pos;
	} while ( in_set(str[t1_pos]," \t\n\r") && t1_pos >= t0_pos );
	if ( t1_pos < t0_pos ) break;

	// Move v1 backwards to point at last non-space.
	do {
	    --v1_pos;
	} while ( in_set(str[v1_pos]," \t\n\r") && v1_pos >= v0_pos );
	if ( v1_pos < v0_pos ) break;

	return true;
	
    };
    clear();
    return false;
}
