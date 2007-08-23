MODULE = fileman
AM_CFLAGS = -Wunused
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess \
                       config.h.in config.sub configure install-sh \
		       ltconfig ltmain.sh missing mkinstalldirs \
		       stamp-h.in acconfig.h

EDJE_CC = @edje_cc@
EDJE_FLAGS = -v \
-id $(top_srcdir)/images \
-fd $(top_srcdir)/fonts

# data files for the module
filesdir =  $(datadir)
files_DATA = \
	e-module-$(MODULE).edj \
	module.desktop \
	fileman.edj

EXTRA_DIST = $(files_DATA) \
	fileman.edc \
	$(wildcard fonts/*.ttf) \
	$(wildcard images/*.png)

# the module .so file
INCLUDES = -I. \
			  -I$(top_srcdir) \
			  -I$(includedir) \
			   @e_cflags@

pkgdir                 =  $(datadir)/$(MODULE_ARCH)

pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			 e_mod_config.c \
			 e_mod_config.h \
			 e_fwin.c \
                         e_fwin.h \
			 e_toolbar.h \
			 e_toolbar.c
			 
module_la_LIBADD       = @e_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h $(top_builddir)/fileman.edj

%.edj:  %.edc
	$(EDJE_CC) $(EDJE_FLAGS) $< $@
clean-local:
	 rm -rf *~
uninstall:
	rm -rf $(datadir)
