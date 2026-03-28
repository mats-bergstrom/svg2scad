//                              -*- Mode: C++ -*- 
// Copyright (C) 2026, Mats
// $Id$
// 
// File name       : s2s.cpp
// Description     : svg2scad 2D oritented paths.
// 
// Author          : Mats
// Created On      : Wed Mar 11 17:51:08 2026
// 
// Last Modified By: Mats
// Last Modified On: Sat Mar 28 14:06:20 2026
// Update Count    : 18
// Status          : $State$
// 
// $Locker$
// $Log$
// 

#include "s2s.hpp"



// -----------------------------------------------------------------------------
// S2S baseclass.

S2S::~S2S()
{
    DLL();
}


int
S2S::visit( const XMLDocument& doc )
{
    DLL();
    return 0;
}


int
S2S::unparse( ostream& ost )
{
    DLL();
    return 0;
}

int
S2S::compile()
{
    DLL();
    return 0;
}

