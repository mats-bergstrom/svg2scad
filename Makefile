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
## Last Modified On: Sun Jan 18 13:58:44 2026
## Update Count    : 15
###############################################################################


LDLIBS = -ltinyxml2
LDFLAGS = -g

CXXFLAGS = -g

CC=g++

.PHONY : clean

all: svg2scad

svg2scad: svg2scad.o

svg2scad.o: svg2scad.cpp


clean:
	rm *.o *~ svg2scad
