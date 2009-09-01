AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = createlang_xslt.sql.in test.sql

pkglib_LTLIBRARIES = plxslt.la
plxslt_la_SOURCES = plxslt.c

plxslt_la_CFLAGS = $(Libxml_CFLAGS) $(Libxslt_CFLAGS)
plxslt_la_LDFLAGS = -module -avoid-version $(Libxslt_LIBS)

pkgdata_SCRIPTS = createlang_xslt.sql

createlang_xslt.sql: createlang_xslt.sql.in plxslt.la
	. plxslt.la && sed "s,%filename%,$(pkglibdir)/$$dlname,g" $(srcdir)/createlang_xslt.sql.in >createlang_xslt.sql

CLEANFILES = createlang_xslt.sql
MAINTAINERCLEANFILES = aclocal.m4 config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing
