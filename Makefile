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
## Last Modified On: Sun Jan 11 22:39:13 2026
## Update Count    : 7
###############################################################################


LDLIBS = -ltinyxml2

CC=g++

all: svg2scad

svg2scad: svg2scad.o

svg2scad.o: svg2scad.cpp

