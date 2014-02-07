CREATE OR REPLACE FUNCTION striptags(xml) RETURNS xml
	LANGUAGE xslt
AS $$<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns="http://www.w3.org/1999/xhtml"
>

  <xsl:output format="text" omit-xml-declaration="yes"/>

  <xsl:template match="/">
    <xsl:apply-templates/>
  </xsl:template>

</xsl:stylesheet>
$$;


SELECT striptags('<?xml version="1.0"?><html xmlns="http://www.w3.org/1999/xhtml"><body>hello</body></html>'::xml);
SELECT striptags('<?xml version="1.0"?><html xmlns="http://www.w3.org/1999/xhtml"><body></body></html>'::xml);
