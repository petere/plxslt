PG_CONFIG = pg_config
PKG_CONFIG = pkg-config

ifneq ($(shell $(PKG_CONFIG) --exists --print-errors libxml-2.0 libxslt && echo yes),yes)
$(error libxml2 or libxslt is not installed properly)
endif

pg_version := $(word 2,$(shell $(PG_CONFIG) --version))
extensions_supported = $(filter-out 6.% 7.% 8.% 9.0%,$(pg_version))


MODULE_big = plxslt
OBJS = plxslt.o

EXTENSION = plxslt

DATA = $(if $(extensions_supported),,plxslt.sql)
DATA_built = $(if $(extensions_supported),plxslt--1.sql)

EXTRA_CLEAN = plxslt--1.sql


PG_CPPFLAGS += $(shell $(PKG_CONFIG) --cflags-only-I libxml-2.0 libxslt)
SHLIB_LINK += $(shell $(PKG_CONFIG) --libs libxml-2.0 libxslt)

REGRESS = init tohtml striptags
REGRESS_OPTS = --inputdir=test

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


plxslt--1.sql: plxslt.sql
	sed 's/^CREATE /CREATE OR REPLACE /' $< >$@
