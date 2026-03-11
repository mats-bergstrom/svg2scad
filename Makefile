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
## Last Modified On: Wed Mar 11 18:10:34 2026
## Update Count    : 18
###############################################################################


LDLIBS = -ltinyxml2
LDFLAGS = -g

CXXFLAGS = -g

CC=g++

.PHONY : clean

all: svg2scad

svg2scad: svg2scad.o s2s.o

svg2scad.o: svg2scad.cpp s2s.hpp

s2s.o: s2s.cpp s2s.hpp

clean:
	rm *.o *~ svg2scad
