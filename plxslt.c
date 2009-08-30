/*
 * PL/XSLT language handler
 *
 * Copyright © 2007 by Peter Eisentraut
 * See the COPYING file for details.
 *
 */

#include <postgres.h>
#include <fmgr.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <access/heapam.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_type.h>
#include <commands/trigger.h>
#include <utils/syscache.h>
#include <utils/builtins.h>
#include <utils/rel.h>
#include <utils/xml.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>


PG_MODULE_MAGIC;


#define _textout(x) (DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(&x))))



/*
 * Convert the C string "input" to a Datum of type "typeoid".
 */
static Datum
cstring_to_type(char * input, Oid typeoid)
{
	HeapTuple	typetuple;
	Form_pg_type pg_type_entry;
	Datum		ret;

	typetuple = SearchSysCache(TYPEOID, ObjectIdGetDatum(typeoid), 0, 0, 0);
	if (!HeapTupleIsValid(typetuple))
		elog(ERROR, "cache lookup failed for type %u", typeoid);

	pg_type_entry = (Form_pg_type) GETSTRUCT(typetuple);

	ret = OidFunctionCall3(pg_type_entry->typinput,
						   CStringGetDatum(input),
						   0, -1);

	ReleaseSysCache(typetuple);

	PG_RETURN_DATUM(ret);
}



/*
 * Convert the Datum "input" that is of type "typeoid" to a C string.
 */
static char *
type_to_cstring(Datum input, Oid typeoid)
{
	HeapTuple	typetuple;
	Form_pg_type pg_type_entry;
	Datum		ret;

	typetuple = SearchSysCache(TYPEOID, ObjectIdGetDatum(typeoid), 0, 0, 0);
	if (!HeapTupleIsValid(typetuple))
		elog(ERROR, "cache lookup failed for type %u", typeoid);

	pg_type_entry = (Form_pg_type) GETSTRUCT(typetuple);

	ret = OidFunctionCall3(pg_type_entry->typoutput,
						   input,
						   0, -1);

	ReleaseSysCache(typetuple);

	return DatumGetCString(ret);
}



/*
 * Internal handler function
 */
Datum
handler_internal(Oid function_oid, FunctionCallInfo fcinfo, bool execute)
{
	HeapTuple	proctuple;
	Form_pg_proc pg_proc_entry;
	char	   *sourcecode;
	Datum		prosrcdatum;
	bool		isnull;
	const char **xslt_params;
	int			i;
	Oid		   *argtypes;
	char	  **argnames;
	char	   *argmodes;
	int			numargs;
	xmlDocPtr	ssdoc;
	xmlDocPtr	xmldoc;
	xmlDocPtr	resdoc;
	xsltStylesheetPtr stylesheet;
	int			reslen;
	xmlChar	   *resstr;
	Datum		resdatum;

	if (CALLED_AS_TRIGGER(fcinfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("trigger functions not supported")));

	proctuple = SearchSysCache(PROCOID, ObjectIdGetDatum(function_oid), 0, 0, 0);
	if (!HeapTupleIsValid(proctuple))
		elog(ERROR, "cache lookup failed for function %u", function_oid);

	prosrcdatum = SysCacheGetAttr(PROCOID, proctuple, Anum_pg_proc_prosrc, &isnull);
	if (isnull)
		elog(ERROR, "null prosrc");

	sourcecode = pstrdup(DatumGetCString(DirectFunctionCall1(textout,
															 prosrcdatum)));
	/* allow one blank line at the start */
	if (sourcecode[0] == '\n')
		sourcecode++;

	numargs = get_func_arg_info(proctuple,
								&argtypes, &argnames, &argmodes);

	if (numargs < 1)
		ereport(ERROR,
				(errmsg("XSLT function must have at least one argument")));

				if (argtypes[0] != XMLOID)
		ereport(ERROR,
				(errmsg("first argument of XSLT function must have type XML")));

#if 0
	xsltSetGenericErrorFunc(NULL, xmlGenericError);
#endif

	ssdoc = xmlParseDoc((xmlChar *) sourcecode); /* XXX use backend's xml_parse here() */
	stylesheet = xsltParseStylesheetDoc(ssdoc); /* XXX check error handling */
	if (!stylesheet)
		ereport(ERROR,
				(errmsg("could not parse stylesheet")));

	pg_proc_entry = (Form_pg_proc) GETSTRUCT(proctuple);

	{
		char	   *method;

		method = (char *) stylesheet->method;
		/*
		 * TODO: This is strictly speaking not correct because the
		 * default output method may be "html", but that can only
		 * detected at run time, so punt for now.
		 */
		if (!method)
			method = "xml";

		if (strcmp(method, "xml") == 0 && pg_proc_entry->prorettype != XMLOID)
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("XSLT stylesheet has output method \"xml\" but return type of function is not xml")));
		else if ((strcmp(method, "html") == 0 || strcmp(method, "text") == 0)
				 && pg_proc_entry->prorettype != TEXTOID && pg_proc_entry->prorettype != VARCHAROID)
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("XSLT stylesheet has output method \"%s\" but return type of function is not text or varchar", method)));
	}

	/* validation stops here */
	if (!execute)
	{
		ReleaseSysCache(proctuple);
		PG_RETURN_VOID();
	}

	/* execution begins here */

	xslt_params = palloc(((numargs - 1) * 2 + 1) * sizeof(*xslt_params));
	for (i = 0; i < numargs-1; i++)
	{
		xslt_params[i*2] = argnames[i+1];
		xslt_params[i*2+1] = type_to_cstring(PG_GETARG_DATUM(i+1),
											 argtypes[i+1]);
	}
	xslt_params[i*2] = NULL;

	{
		xmltype *arg0 = PG_GETARG_XML_P(0);
		// XXX this ought to use xml_parse()
		xmldoc = xmlParseMemory((char *) VARDATA(arg0), VARSIZE(arg0) - VARHDRSZ);
	}

	resdoc = xsltApplyStylesheet(stylesheet, xmldoc, xslt_params);
	if (!resdoc)
		elog(ERROR, "xsltApplyStylesheet() failed");

	xmlFreeDoc(xmldoc);

	if (xsltSaveResultToString(&resstr, &reslen, resdoc, stylesheet) != 0)
		elog(ERROR, "result serialization failed");

	xsltFreeStylesheet(stylesheet);
	xmlFreeDoc(resdoc);

	xsltCleanupGlobals();
	xmlCleanupParser();

	resdatum = cstring_to_type((char *) resstr, pg_proc_entry->prorettype);
	ReleaseSysCache(proctuple);
	PG_RETURN_DATUM(resdatum);
}



/*
 * The PL handler
 */
PG_FUNCTION_INFO_V1(plxslt_handler);

Datum
plxslt_handler(PG_FUNCTION_ARGS)
{
	return handler_internal(fcinfo->flinfo->fn_oid, fcinfo, true);
}



/*
 * Validator function
 */
PG_FUNCTION_INFO_V1(plxslt_validator);

Datum
plxslt_validator(PG_FUNCTION_ARGS)
{
	return handler_internal(PG_GETARG_OID(0), fcinfo, false);
}
