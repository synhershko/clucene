## Makefile.am -- Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = 1.6

include_HEADERS = CLucene.h
lib_LTLIBRARIES        = libclucene.la
libclucene_la_LDFLAGS  = --version-info $(CLLIB_VERSION) --release $(PACKAGE_VERSION)

lsrcdir 	= $(top_srcdir)/src/CLucene
EXTRA_DIST 	= $(lsrcdir)/CLMonolithic.cpp \
              $(lsrcdir)/CMakeLists.txt \
              $(lsrcdir)/clucene-config.h.cmake

libclucene_la_SOURCES  =
include CLucene/Makefile.am
include CLucene/config/Makefile.am
include CLucene/util/Makefile.am
include CLucene/store/Makefile.am
include CLucene/debug/Makefile.am
include CLucene/analysis/Makefile.am
include CLucene/analysis/standard/Makefile.am
include CLucene/document/Makefile.am
include CLucene/index/Makefile.am
include CLucene/queryParser/Makefile.am
include CLucene/search/Makefile.am

SUBDIRS = .
DIST_SUBDIRS = . demo

monolithic: 
	@rm -f CLMonolithic.$(OBJEXT)
	$(MAKE) $(AM_MAKEFLAGS) CLMonolithic.o

CLMonolithic.o:
	$(CXXCOMPILE) -c $(lsrcdir)/CLMonolithic.cpp

MAINTAINERCLEANFILES    = Makefile.in
