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
// Last Modified On: Wed Mar 11 18:11:18 2026
// Update Count    : 13
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
#define PP(X) do{for(unsigned i=(X);i;--i)cout << "  ";}while(0)
#define DD(X) do{if(opt_d)cout<<__PRETTY_FUNCTION__<<" "<<#X<<"="<<X<<endl;}while(0)
#define PPD(L,X) do{if(opt_d){PP(L);DD(X);}}while(0)
#define PLE(L,X) do{if(opt_d){PP(L);LLE(X);}}while(0)


#if 0
int in_set(char c, const char* s);
void skip_ws( const char* &s );
#endif




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



#endif
