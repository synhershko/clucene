## Makefile.am -- Process this file with automake to produce Makefile.in

INCLUDES                = -I$(top_srcdir)/src -I$(top_builddir)/src
demosrcdir 				= $(top_srcdir)/src/demo

bin_PROGRAMS            = cl_demo

cl_demo_SOURCES         = $(demosrcdir)/DeleteFiles.cpp \
						  $(demosrcdir)/IndexFiles.cpp \
						  $(demosrcdir)/SearchFiles.cpp \
						  $(demosrcdir)/Statistics.cpp \
						  $(demosrcdir)/Main.cpp \
						  $(demosrcdir)/stdafx.cpp

cl_demo_LDADD           = $(top_builddir)/src/libclucene.la

EXTRA_DIST = $(demosrcdir)/Jamfile.v2 \
			 $(demosrcdir)/stdafx.h \
			 $(demosrcdir)/CMakeLists.txt
MAINTAINERCLEANFILES    = Makefile.in
