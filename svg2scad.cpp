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
// Last Modified On: Mon Jan 12 20:12:02 2026
// Update Count    : 46
// 


#include <iostream>
#include <stdio.h>

#include<tinyxml2.h>

using namespace std;
using namespace tinyxml2;



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

    unsigned lvl;

 private:
    MyVisitor(const MyVisitor& );
    MyVisitor& operator=( const MyVisitor& );
};

MyVisitor::MyVisitor() : XMLVisitor(), lvl(0) {};

#define LL() do{cout<<__PRETTY_FUNCTION__<<endl;}while(0)
#define PP(X) do{for(unsigned i=(X);i;--i)cout << "  ";}while(0)

bool MyVisitor::VisitEnter(const XMLDocument& x)
{
    cout << "-------------------------------------------------------" << endl
	 << "BEGIN Document (" << lvl << ")" << endl;
    ++lvl;
    return true;
}

bool MyVisitor::VisitExit(const XMLDocument& x)  
{
    if(lvl) --lvl;
    cout << "END Document (" << lvl << ")" << endl
	 << "-------------------------------------------------------" << endl;
    return true;
}

bool MyVisitor::VisitEnter(const XMLElement& x, const XMLAttribute* a)
{
    PP(lvl);
    cout << x.Name() << " BEGIN" << endl;
    ++lvl;
    while (a) {
	PP(lvl);
	cout << a->Name() << "=" << '"' << a->Value() << '"' << endl;
	a = a->Next();
    }
    return true;
}

bool MyVisitor::VisitExit(const XMLElement& x)
{
    --lvl;
    PP(lvl);
    cout << x.Name() << " END" << endl;
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
    LL();
    return true;
}

bool MyVisitor::Visit(const XMLComment& x) 
{
    LL();
    return true;
}

bool MyVisitor::Visit(const XMLUnknown& x) 
{
    LL();
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
    doc.Accept( &printer );
    cout << "========================================================="<< endl;
    doc.Accept( &myvisitor );
    cout << "========================================================="<< endl;
    
    return 0;
}
