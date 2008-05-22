INCLUDES                = -I$(top_srcdir)/src -I$(top_builddir)/src

check_PROGRAMS            = cl_test
cl_test_LDFLAGS           = $(top_builddir)/src/libclucene.la
DIST_SUBDIRS = data

tstsrcdir = $(top_srcdir)/test
cl_test_SOURCES            = CuTest.cpp testall.cpp tests.cpp
include analysis/Makefile.am
include debug/Makefile.am
include document/Makefile.am
include index/Makefile.am
include queryParser/Makefile.am
include search/Makefile.am
include store/Makefile.am
include util/Makefile.am

EXTRA_DIST = $(tstsrcdir)/README \
			 $(tstsrcdir)/CLMonolithic_Test.cpp \
			 $(tstsrcdir)/Jamfile.v2 \
			 $(tstsrcdir)/*.h \
			 $(tstsrcdir)/CMakeLists.txt
MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = cl_test_monolithic$(EXEEXT) cl_test_monolithic *.stackdump
TESTS = cl_test$(EXEXT)

#monolithic building
monolithic: 
	@rm -f CLMonolithic_Test.$(OBJEXT)
	$(MAKE) $(AM_MAKEFLAGS) cl_test_monolithic$(EXEEXT)

CLMonolithic_Test.$(OBJEXT):
	$(CXXCOMPILE) -c $(srcdir)/CLMonolithic_Test.cpp

cl_test_monolithic$(EXEEXT): CLMonolithic_Test.o
	@rm -f cl_test_monolithic$(EXEEXT)
	$(CXXLINK) CLMonolithic_Test.$(OBJEXT) ../src/CLMonolithic.$(OBJEXT) $(LIBS)
