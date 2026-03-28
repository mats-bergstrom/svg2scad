######################### -*- Mode: Makefile-Gmake -*- ########################
## Copyright (C) 2026, Mats
## 
## File name       : Makefile
## Description     : for svg2scad
## 
## Author          : Mats
## Created On      : Sun Jan 11 21:03:31 2026
## 
## Last Modified By: Mats
## Last Modified On: Mon Mar 23 18:27:52 2026
## Update Count    : 21
###############################################################################


LDLIBS = -ltinyxml2
LDFLAGS = -g

CXXFLAGS = -g

CC=g++

.PHONY : clean

all: svg2scad

svg2scad: svg2scad.o s2t.o s2d.cpp s2s.o str.o

svg2scad.o: svg2scad.cpp s2s.hpp s2d.hpp s2t.hpp str.hpp

s2s.o: s2s.cpp s2s.hpp str.hpp

s2d.o: s2d.cpp s2d.hpp s2s.hpp str.hpp

s2t.o: s2t.cpp s2t.hpp s2s.hpp str.hpp

str.o: str.cpp str.hpp

clean:
	rm *.o *~ svg2scad
