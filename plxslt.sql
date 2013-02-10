CREATE FUNCTION plxslt_handler() RETURNS language_handler
    AS '$libdir/plxslt'
    LANGUAGE C;

CREATE FUNCTION plxslt_validator(oid) RETURNS void
    AS '$libdir/plxslt'
    LANGUAGE C;

CREATE LANGUAGE xslt
    HANDLER plxslt_handler
    VALIDATOR plxslt_validator;
