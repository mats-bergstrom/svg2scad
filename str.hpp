//                              -*- Mode: C++ -*- 
// Copyright (C) 2026, Mats
// 
// File name       : str.hpp
// Description     : General String Massaging
// 
// Author          : Mats
// Created On      : Mon Mar 23 18:13:50 2026
// 
// Last Modified By: Mats
// Last Modified On: Sat Mar 28 14:05:52 2026
// Update Count    : 14
// 

#ifndef __STR_HPP__
#define __STR_HPP__

#include <string>
#include <deque>
#include <iostream>

using namespace std;


int in_set(char c, const char* s);
void skip_ws( const char* &s );


typedef deque<string> string_list_t;
typedef string_list_t::iterator string_list_iter_t;
typedef string_list_t::const_iterator string_list_citer_t;

class MetaVariableMarker {
    // A metavariable is '$' <name> '(' <arg-lis> ')'
public:
    MetaVariableMarker() :
	d_pos(string::npos), l_pos(string::npos), r_pos(string::npos) {};
    
    size_t d_pos;			// position of '$'
    size_t l_pos;			// position of '('
    size_t r_pos;			// position of ')'

    void clear();
    bool find( const string& str );	// Find a metavariable in string

    ostream& print(ostream& os) const;
};

inline ostream& operator<<(ostream& os, const MetaVariableMarker& mvm) {
    return mvm.print(os);
}

class TagValMarker {
    // A Tag Val is <tag> ':' <val> [';']
public:
    TagValMarker() :
	t0_pos(string::npos), t1_pos(string::npos),
	v0_pos(string::npos), v1_pos(string::npos) {};

    size_t t0_pos;			// Start of tag
    size_t t1_pos;			// 1 past end of tag
    size_t v0_pos;			// Star of val
    size_t v1_pos;			// 1 past end of val (or npos);

    void clear();
    bool find( const string& str );	// Find a metavariable in string

    ostream& print(ostream& os) const;
};

inline ostream& operator<<(ostream& os, const TagValMarker& mvm) {
    return mvm.print(os);
}


class StrList : public string_list_t {
public:
    static void chomp(string& );     // Remove initial and trailing WS
    void chomp();		     // comp all strings in the list.
    ostream& print(ostream& os) const {
	string_list_citer_t i = begin();
	string_list_citer_t i_max = end();
	while ( i != i_max ) {
	    os << *i;
	    ++i;
	    if ( i != i_max )
		os << endl;
	}
	return os;
    };
};
inline ostream& operator<<(ostream& os, const StrList& x ) {return x.print(os);}


#endif
