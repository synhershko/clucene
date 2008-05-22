## Makefile.am -- Process this file with automake to produce Makefile.in

INCLUDES   =  -I$(top_srcdir)

SUBDIRS = src test
DIST_SUBDIRS = src test doc win32 .

ACLOCAL_AMFLAGS = -I $(top_srcdir)/m4

EXTRA_DIST = $(top_srcdir)/APACHE.license \
           $(top_srcdir)/LGPL.license \
           $(top_srcdir)/project-root.jam \
           $(top_srcdir)/Jamfile.v2 \
           $(top_srcdir)/Jamfile \
           $(top_srcdir)/autogen.sh \
           $(top_srcdir)/HACKING \
           $(top_srcdir)/REQUESTS \
           $(top_srcdir)/CMakeLists.txt

distall: distdir
	$(AMTAR) chof - $(distdir) >$(distdir).tar
	GZIP=$(GZIP_ENV) gzip -c $(distdir).tar >$(distdir).tar.gz
	bzip2 -9 -c $(distdir).tar >$(distdir).tar.bz2
	-rm -f $(distdir).zip
	zip -rq $(distdir).zip $(distdir)
	-rm -f $(distdir).tar
	$(am__remove_distdir)

dist-hook:
	rm -rfd `find $(distdir)/ -name CVS`
	rm -rfd `find $(distdir)/ -name .svn`
	rm -rfd `find $(distdir)/ -name .dep`

MAINTAINERCLEANFILES    = Makefile.in \
						  aclocal.m4 \
						  configure \
						  config.h.tmp.in

DISTCLEANFILES = 	*.tar.gz \
					*.tar.bz2 \
					*.zip \
					_configs.sed \
					src/CLucene/clucene-config.h \
					config.h.tmp

upload:
	make distcheck
	ncftpput -v upload.sourceforge.net /incoming @PACKAGE@-@VERSION@.tar.gz

doxygen:
	rm -fdr ./doc/html
	doxygen Doxyfile
	cp ./doc/clucene.jpg ./doc/html/clucene.jpg

test: test-all

#test-compiles using different compile options
test-all: test-pedantic test-ascii test-singlethreading test-namespace test-mmap test-mtracking test-refcnt test-minqp

test-do:
	@rm -f src/CLMonolithic.$(OBJEXT)
	@rm -f test/cl_test_monolithic$(EXEEXT)
	@rm -f test/CLMonolithic_Test.$(OBJEXT)
	cd src && $(MAKE) $(AM_MAKEFLAGS) CLMonolithic.$(OBJEXT) AM_CPPFLAGS="-pedantic $(AM_CPPFLAGS)"
	cd test && $(MAKE) $(AM_MAKEFLAGS) CLMonolithic_Test.$(OBJEXT) AM_CPPFLAGS="-pedantic $(AM_CPPFLAGS)"
	cd test && $(MAKE) $(AM_MAKEFLAGS) cl_test_monolithic$(EXEEXT) AM_CPPFLAGS="-pedantic $(AM_CPPFLAGS)"
	test/cl_test_monolithic$(EXEEXT)

test-pedantic:
	$(MAKE) test-do AM_CPPFLAGS=""

test-ascii:
	$(MAKE) test-do AM_CPPFLAGS="-D_ASCII"

test-namespace:
	$(MAKE) test-do AM_CPPFLAGS="-DDISABLE_NAMESPACE"
	
test-sparsemap:
	$(MAKE) test-do AM_CPPFLAGS="-D_CL_HAVE_GOOGLE_DENSE_HASH_MAP"

test-mmap:
	$(MAKE) test-do AM_CPPFLAGS="-DLUCENE_FS_MMAP"

test-singlethreading:
	$(MAKE) test-do AM_CPPFLAGS="-D_CL_DISABLE_MULTITHREADING"

test-mtracking:
	$(MAKE) test-do AM_CPPFLAGS="-DLUCENE_ENABLE_MEMLEAKTRACKING"

test-refcnt:
	$(MAKE) test-do AM_CPPFLAGS="-DLUCENE_ENABLE_REFCOUNT"

test-minqp:
	$(MAKE) test-do AM_CPPFLAGS="-DNO_FUZZY_QUERY -DNO_WILDCARD_QUERY -DNO_PREFIX_QUERY -DNO_RANGE_QUERY"

test-debug:
	$(MAKE) test-do AM_CPPFLAGS="-D_DEBUG"
