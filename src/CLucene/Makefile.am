## Makefile.am -- Process this file with automake to produce Makefile.in
stdhdir = $(includedir)/CLucene

libclucene_la_SOURCES    += $(lsrcdir)/StdHeader.cpp
stdh_HEADERS = $(lsrcdir)/CLBackwards.h
stdh_HEADERS += $(lsrcdir)/CLConfig.h
stdh_HEADERS += $(lsrcdir)/LuceneThreads.h
stdh_HEADERS += $(lsrcdir)/StdHeader.h

clucene_configdir = $(libdir)/CLucene
clucene_config_DATA = $(top_builddir)/src/CLucene/clucene-config.h
