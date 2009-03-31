## $Id$

## Makefile.am -- Process this file with automake to produce Makefile.in

## compile library

lib_LTLIBRARIES = libmddlib3.la

revision.h:
	./BuildRevNo

BUILT_SOURCES = revision.h

libmddlib3_la_SOURCES = \
	../include/mddexpert.h \
	../include/mddlib.h \
	dd_edge.cc \
	domain.cc \
	forest.cc \
	operation_ext.cc \
	operation.cc \
	compute_manager.cc \
	compute_cache.cc \
	mdds.cc \
	mdds_ext.cc \
	operation_ext.h \
	compute_cache.h \
	mdds.h \
	mdds_ext.h \
	defines.h \
	hash.h \
	fixed_size_hash.h \
	mdd_hash.h \
	timer.h

## compile apps

CXXFLAGS = 
CXXFLAGS += -Wall
## CXXFLAGS += -g
## CXXFLAGS += -ggdb
## CXXFLAGS += -pg
## CXXFLAGS += -DDEVELOPMENT_CODE
## CXXFLAGS += -DDIFF_SAT
## CXXFLAGS += -O2
## CXXFLAGS += -O3
## CXXFLAGS += -m64
