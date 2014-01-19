/*
 * $Id: $
 *
 * Module:  pipetypesetter -- XSL-FO to SVG/PS/PDF pipe-converter
 * Created: 11-MAY-2010 16:15
 * Author:  tmr
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libfo/fo-libfo.h>
#include <libfo/libfo-compat.h>
#include <libfo/libfo-version.h>

#if _HAS_CAIRO_
#include <libfo/fo-doc-cairo.h>
#endif

#if _HAS_GNOMEPRINT_
#include <libfo/fo-doc-gp.h>
#endif

/* Max 2MB input and output buffers */
/* XXX: original value was 5MB, but Linux SIGSEGVed on it even if all the
 * limits and quotas were configured to allow it */
#define BUFSIZE     2*1024*1024

/*
static const gchar * testing_data = "<?xml version=\"1.0\" encoding=\"ISO8859-2\"?>"
"<fo:root xmlns:fo=\"http://www.w3.org/1999/XSL/Format\">"
"  <fo:layout-master-set>"
"    <fo:simple-page-master page-height=\"297mm\" page-width=\"210mm\""
"           margin=\"5mm 25mm 5mm 25mm\" master-name=\"PageMaster\">"
"      <fo:region-body margin=\"20mm 0mm 20mm 0mm\" />"
"    </fo:simple-page-master>"
"  </fo:layout-master-set>"
"  <fo:page-sequence master-reference=\"PageMaster\">"
"    <fo:flow flow-name=\"xsl-region-body\">"
"      <fo:block>"
"        <fo:block>Hello World!</fo:block>"
"        <fo:block>This is the first +ì¹èø¾ýáíé+Ì©ÈØ®ÝÁÍÉ"
"           <fo:inline font-weight=\"bold\">SimpleDoc</fo:inline>"
"         </fo:block>"
"         <fo:block space-after=\"10pt\">"
"           <fo:external-graphic"
"             src=\"url('/home/tmr/src/tests/xmlroff-lib/powered_1ovms.gif')\""
"             width=\"340px\" height=\"238px\" />"
"         </fo:block>"
"      </fo:block>"
"    </fo:flow>"
"  </fo:page-sequence>"
"</fo:root>\0";
*/

typedef enum {
#if _HAS_CAIRO_
  Cairo,
#endif
#if _HAS_GNOMEPRINT_
  GP
#endif
} EBackendType;

typedef enum { EOutputTypePDF, EOutputTypeEPS, EOutputTypeSVG } EOutputType;

int load_file (FILE * f, char * out) {

  char buf [512]; int i, res = 0;
  while ((i = fread (buf, 1, sizeof (buf), f)) > 0 && res < BUFSIZE) {
    memcpy (out + res, buf, BUFSIZE -res -1);
    res += i;
  }

  return res;
}

static void exit_if_error (GError *error) {

  if (error != NULL) {
    g_warning ("%s:: %s", error->domain, error->message);
    g_error_free (error);
    exit (1);
  }
}

static FoDoc * init_fo_doc (EBackendType b,
    const gchar * out_file, FoLibfoContext * libfo_context) {

  FoDoc *fo_doc = NULL;
  GError *error = NULL;

  fo_doc = (b == Cairo)?
#if _HAS_CAIRO_
    fo_doc_cairo_new () :
#else
    NULL :
#endif

#if _HAS_GNOMEPRINT_
    fo_doc_gp_new ();
#else
    NULL;
#endif

  fo_doc_open_file (fo_doc, out_file, libfo_context, &error);

  exit_if_error (error);

  return fo_doc;
}

/*
 *  @input  -> a zero terminated XML string
 *  @output -> binary data of the resulting document
 *             (no zero termination!)
 *
 *  return value -> lenght of the @output
 */
int process_document (char * input, char * output, EOutputType type) {

  gboolean validation = FALSE;
  gboolean compat     = TRUE;
  gboolean continue_after_error = TRUE;
  FoFlagsFormat format_mode = FO_FLAG_FORMAT_AUTO;

  FoWarningFlag warning_mode = FO_WARNING_FO | FO_WARNING_PROPERTY;
  FoDebugFlag debug_mode = FO_DEBUG_NONE;
  gchar out_file [256] = "/tmp/pipetypesetter.XXXXXXXX\0";
  int fd = mkstemp ((char *) &out_file); close (fd);
  const gchar    * xml_data = input; //testing_data;
  const gchar    * xml_url = NULL;

#if _HAS_CAIRO_
  EBackendType backend = Cairo;
#else
  EBackendType backend = GP;
#endif

  switch (type) {
    case EOutputTypeSVG: format_mode = FO_FLAG_FORMAT_SVG; break;
    case EOutputTypeEPS: format_mode = FO_FLAG_FORMAT_POSTSCRIPT; break;
    case EOutputTypePDF:
    default:             format_mode = FO_FLAG_FORMAT_PDF; break;
  }

  FoDoc * fo_doc = NULL;
  GError *error = NULL;

  fo_libfo_init ();
  FoLibfoContext * libfo_context = fo_libfo_context_new ();
  fo_libfo_context_set_validation (libfo_context, validation);
  fo_libfo_context_set_continue_after_error (libfo_context, continue_after_error);

  fo_libfo_context_set_format (libfo_context, format_mode);

  fo_doc = init_fo_doc (backend, out_file, libfo_context);

  fo_libfo_context_set_debug_mode (libfo_context, debug_mode);
  fo_libfo_context_set_warning_mode (libfo_context, warning_mode);

  FoXmlDoc * result_tree = fo_xml_doc_new_from_string (xml_data, xml_url, NULL, libfo_context, &error);

  if (compat == TRUE) {
    FoXmlDoc * old_result_tree = result_tree;
    result_tree = libfo_compat_make_compatible (old_result_tree, libfo_context, &error);
    fo_xml_doc_unref (old_result_tree);
    exit_if_error (error);
  }

  FoXslFormatter * fo_xsl_formatter = fo_xsl_formatter_new ();
  fo_xsl_formatter_set_result_tree (fo_xsl_formatter, result_tree);
  exit_if_error (error);

  fo_xsl_formatter_set_fo_doc (fo_xsl_formatter, fo_doc);
  fo_xsl_formatter_format (fo_xsl_formatter, libfo_context, &error);
  exit_if_error (error);

  fo_xsl_formatter_draw (fo_xsl_formatter, libfo_context, &error);
  exit_if_error (error);

  g_object_unref (fo_xsl_formatter);
  g_object_unref (fo_doc);
  fo_libfo_shutdown ();

  FILE * f = fopen (out_file, "r");
  int len = load_file (f, (char *) output);
  fclose (f);
  unlink (out_file);

  return len;
}

char * mybasename (char * path) {

  int len = strlen (path);

  while (len-- > 0)
    if (*(path+len) == '/') return (path+len+1);

  return path;
}

int main (int argc, char * argv []) {

  char buf [BUFSIZE];
  char out [BUFSIZE];
  int inlen = load_file (stdin, (char *) &buf);
  buf [inlen] = '\0';
  EOutputType type = EOutputTypePDF;

  /* This does not work on Linux, nobody knows why :-( */
  //const char * progname = basename (argv [0]);
  const char * progname = mybasename (argv [0]);

  if (!strncmp (progname, "xslfo2ps", 8)) type = EOutputTypeEPS;
  else if (!strncmp (progname, "xslfo2svg", 9)) type = EOutputTypeSVG;
  else type = EOutputTypePDF; /* for xslfo2pdf and all the friends */

  int outlen = process_document ((char *) &buf, (char *) &out, type);
  fwrite (out, sizeof (char), outlen, stdout);

  return (0);
}

// vim: fdm=syntax:fdn=0:tw=74:ts=2:syn=c
