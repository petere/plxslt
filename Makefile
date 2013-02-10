PG_CONFIG = pg_config
PKG_CONFIG = pkg-config

MODULE_big = plxslt
OBJS = plxslt.o

DATA = plxslt.sql

PG_CPPFLAGS += $(shell $(PKG_CONFIG) --cflags-only-I libxml-2.0 libxslt)
SHLIB_LINK += $(shell $(PKG_CONFIG) --libs libxml-2.0 libxslt)

REGRESS = init tohtml
REGRESS_OPTS = --inputdir=test

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
