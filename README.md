PL/XSLT Procedural Language Handler for PostgreSQL
==================================================

PL/XSLT is a procedural language handler for PostgreSQL that allows you to write stored procedures in XSLT.  A function definition looks like:

    CREATE FUNCTION foo(xml) RETURNS xml AS $$
    <?xml version="1.0"?>
    <xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
      ...
    </xsl:stylesheet>
    $$ LANGUAGE xslt;

Conditions on the function definition:

- The first parameter must be of type `xml`.  It will receive the input document.  Every function must have at least this one parameter.

- The following function parameters will be supplied to the stylesheet as XSL parameters.

- The return type must match the output method specified by the stylesheet.  If it is `xml`, then the return type must be `xml`; if it is `text` or `html`, the return type must be `text` or `varchar`.

- Triggers are not supported.

Refer to the `INSTALL` file for installation instructions and `COPYING` for the license.  The distribution also contains a test suite which contains a simplistic demonstration of the functionality.

I'm interested if anyone is using this.

Peter Eisentraut <peter@eisentraut.org>

[![Build Status](https://secure.travis-ci.org/petere/plxslt.png)](http://travis-ci.org/petere/plxslt)

Installation
------------

Build:

    make
    make install

Set the environment variable `PG_CONFIG` to the location of the `pg_config` program belonging to the desired PostgreSQL installation, if it is not found automatically.

Load into database:

    CREATE EXTENSION plxslt;

For versions before PostgreSQL 9.1:

    psql -f plxslt.sql
