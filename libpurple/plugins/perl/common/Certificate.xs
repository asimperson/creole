<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
<title>Pidgin ViewMTN: File libpurple/plugins/perl/common/Certificate.xs in revision [61b1e768..]</title>
<link rel="stylesheet" href="http://developer.pidgin.im/viewmtn/static/viewmtn.css" type="text/css" media="screen" />
<link rel="stylesheet" href="http://developer.pidgin.im/viewmtn/static/highlight.css" type="text/css" media="screen" />

<link rel="stylesheet" href="http://pidgin.im/shared/css/main.css" type="text/css" media="screen" />

  <!-- Correct rendering bugs from Microsoft Internet Explorer -->
  <!--[if lt IE 7]>
  <script type="text/javascript">
    IE7_PNG_SUFFIX = ".png";
  </script>
  <script src="http://pidgin.im/shared/js/ie7/ie7-standard-p.js" type="text/javascript"></script>
    <link rel="stylesheet" href="http://pidgin.im/shared/css/ie6.css" type="text/css" />
  <![endif]-->
  <!--[if gte IE 7]>
    <script defer src="http://pidgin.im/shared/js/ie7-hacks.js" type="text/javascript"></script>
    <link rel="stylesheet" href="http://pidgin.im/shared/css/ie7.css" type="text/css" />
  <![endif]-->

<script type="text/javascript" src="http://developer.pidgin.im/viewmtn/static/MochiKit/MochiKit.js"></script>
<script type="text/javascript" src="http://developer.pidgin.im/viewmtn/static/viewmtn.js"></script>
</head>
<body>
<div id="BodyWrapper">
<div id="Container">
<div id="header">
<img src="http://pidgin.im/shared/img/logo.text.jpg" alt="Pidgin" />
<ul>
<li><a href="http://pidgin.im/">Home</a></li>
<li><a href="http://pidgin.im/download">Download</a></li>
<li><a href="http://pidgin.im/about">About</a></li>
<li><a href="http://pidgin.im/news">News</a></li>
<li class="active"><a href="http://developer.pidgin.im/">Support &amp; Development</a></li>
</ul>
</div><!-- .TopNavigation -->

<div id="popupBox" class="invisible"></div>
<div id="menuBar">
  <strong>ViewMTN
:</strong>
  <a href="http://developer.pidgin.im/viewmtn/">Branches</a> |
  <a href="http://developer.pidgin.im/viewmtn/tags">Tags</a> |
  <a href="http://developer.pidgin.im/viewmtn/help">Help</a> |
  <a href="http://developer.pidgin.im/viewmtn/about">About</a>
  <br/>

<strong>Revision [61b1e768..]</strong>:
<a href="http://developer.pidgin.im/viewmtn/revision/info/61b1e7687aede70c580182fd1dc5f2d797e70092">Info</a> |
<a href="http://developer.pidgin.im/viewmtn/revision/browse/61b1e7687aede70c580182fd1dc5f2d797e70092">Browse Files</a> <!-- |
<a href="http://developer.pidgin.im/viewmtn/revision/tar/61b1e7687aede70c580182fd1dc5f2d797e70092">Download (tar)</a> --><br />
<strong>File libpurple/plugins/perl/common/Certificate.xs</strong>:
<a href="http://developer.pidgin.im/viewmtn/revision/filechanges/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Certificate.xs">Changes</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/file/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Certificate.xs">View</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/downloadfile/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Certificate.xs">Download</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/filechanges/rss/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Certificate.xs">RSS</a>
</div>


<p>
Below is the file 'libpurple/plugins/perl/common/Certificate.xs' from this revision. You can also 
<a href="http://developer.pidgin.im/viewmtn/revision/downloadfile/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Certificate.xs">download the file</a>.
</p>

<pre class="code">
#include &quot;module.h&quot;

struct cb_data {
	SV *cb;
	SV *user_data;
};

static void cb_cert_verify(PurpleCertificateVerificationStatus st, struct cb_data *d) {
	dSP;

	ENTER;
	SAVETMPS;

	PUSHMARK(SP);

	XPUSHs(sv_2mortal(newSViv(st)));
	XPUSHs(d-&gt;user_data);

	PUTBACK;

	call_sv(d-&gt;cb, G_VOID | G_EVAL);

	if(SvTRUE(ERRSV)) {
		STRLEN l_a;
		purple_debug_warning(&quot;perl&quot;, &quot;Failed to run 'certificate verify' callback: %s\n&quot;, SvPV(ERRSV, l_a));
	}

	FREETMPS;
	LEAVE;

	SvREFCNT_dec(d-&gt;cb);
	SvREFCNT_dec(d-&gt;user_data);

	g_free(d);
}

MODULE = Purple::Certificate  PACKAGE = Purple::Certificate  PREFIX = purple_certificate_
PROTOTYPES: ENABLE

BOOT:
{
	HV *stash = gv_stashpv(&quot;Purple::Certificate&quot;, 1);

	static const constiv *civ, const_iv[] = {
#define const_iv(name) {#name, (IV)PURPLE_CERTIFICATE_##name}
		const_iv(INVALID),
		const_iv(VALID),
	};

	for (civ = const_iv + sizeof(const_iv) / sizeof(const_iv[0]); civ-- &gt; const_iv; )
		newCONSTSUB(stash, (char *)civ-&gt;name, newSViv(civ-&gt;iv));
}

void
purple_certificate_add_ca_search_path(path)
	const char* path

gboolean
purple_certificate_check_subject_name(crt, name)
	Purple::Certificate crt
	const gchar* name

Purple::Certificate
purple_certificate_copy(crt)
	Purple::Certificate crt

void
purple_certificate_destroy(crt)
	Purple::Certificate crt

void
purple_certificate_display_x509(crt)
	Purple::Certificate crt

## changed order of arguments, so that $cert-&gt;export($file) could be used
gboolean
purple_certificate_export(crt, filename)
	const gchar* filename
	Purple::Certificate crt
	C_ARGS:
		filename, crt

Purple::Certificate::Pool
purple_certificate_find_pool(scheme_name, pool_name)
	const gchar* scheme_name
	const gchar* pool_name

Purple::Certificate::Scheme
purple_certificate_find_scheme(name)
	const gchar* name

Purple::Certificate::Verifier
purple_certificate_find_verifier(scheme_name, ver_name)
	const gchar* scheme_name
	const gchar* ver_name

Purple::Handle
purple_certificate_get_handle()

gchar_own*
purple_certificate_get_issuer_unique_id(crt)
	Purple::Certificate crt

gchar_own*
purple_certificate_get_subject_name(crt)
	Purple::Certificate crt

gchar_own*
purple_certificate_get_unique_id(crt)
	Purple::Certificate crt

Purple::Certificate
purple_certificate_import(scheme, filename)
	Purple::Certificate::Scheme scheme
	const gchar* filename

gboolean
purple_certificate_register_pool(pool)
	Purple::Certificate::Pool pool

gboolean
purple_certificate_register_scheme(scheme)
	Purple::Certificate::Scheme scheme

gboolean
purple_certificate_register_verifier(vr)
	Purple::Certificate::Verifier vr

gboolean
purple_certificate_signed_by(crt, issuer)
	Purple::Certificate crt
	Purple::Certificate issuer

gboolean
purple_certificate_unregister_pool(pool)
	Purple::Certificate::Pool pool

gboolean
purple_certificate_unregister_scheme(scheme)
	Purple::Certificate::Scheme scheme

gboolean
purple_certificate_unregister_verifier(vr)
	Purple::Certificate::Verifier vr

void
purple_certificate_verify_complete(vrq, st)
	Purple::Certificate::VerificationRequest vrq
	Purple::Certificate::VerificationStatus st

gboolean
purple_certificate_get_times(crt, OUTLIST time_t activation, OUTLIST time_t expiration)
	Purple::Certificate crt
	PROTOTYPE: $

void
purple_certificate_destroy_list(...)
	PREINIT:
	GList* l = NULL;
	int i = 0;
	CODE:
		for(i = 0; i &lt; items; i++) { /* PurpleCertificate */
			l = g_list_prepend(l, purple_perl_ref_object(ST(i)));
		}
		purple_certificate_destroy_list(l);

void
purple_certificate_get_pools()
	PREINIT:
		GList *l;
	PPCODE:
		for(l = purple_certificate_get_pools(); l; l = l-&gt;next) {
			XPUSHs(sv_2mortal(purple_perl_bless_object(l-&gt;data, &quot;Purple::Certificate::Pool&quot;)));
		}

void
purple_certificate_get_schemes()
	PREINIT:
		GList *l;
	PPCODE:
		for(l = purple_certificate_get_schemes(); l; l = l-&gt;next) {
			XPUSHs(sv_2mortal(purple_perl_bless_object(l-&gt;data, &quot;Purple::Certificate::Scheme&quot;)));
		}

void
purple_certificate_get_verifiers()
	PREINIT:
		GList *l;
	PPCODE:
		for(l = purple_certificate_get_verifiers(); l; l = l-&gt;next) {
			XPUSHs(sv_2mortal(purple_perl_bless_object(l-&gt;data, &quot;Purple::Certificate::Verifier&quot;)));
		}

void
purple_certificate_check_signature_chain(...)
	PREINIT:
		GList *l = NULL;
		gboolean ret;
		int i;
	PPCODE:
		for(i = 0; i &lt; items; i++) { /* PurpleCertificate */
			l = g_list_prepend(l, purple_perl_ref_object(ST(i)));
		}
		l = g_list_reverse(l);
		ret = purple_certificate_check_signature_chain(l);
		g_list_free(l);
		if(ret) XSRETURN_YES;
		XSRETURN_NO;

SV*
purple_certificate_get_fingerprint_sha1(crt)
	Purple::Certificate crt
	PREINIT:
		GByteArray *gba = NULL;
	CODE:
		gba = purple_certificate_get_fingerprint_sha1(crt);
		RETVAL = newSVpv(gba-&gt;data, gba-&gt;len);
		g_byte_array_free(gba, TRUE);
	OUTPUT:
		RETVAL

void
purple_certificate_verify(verifier, subject_name, cert_chain, cb, cb_data)
	Purple::Certificate::Verifier verifier
	const gchar* subject_name
	AV* cert_chain
	CV *cb
	SV *cb_data
	PREINIT:
		GList *l = NULL;
		int len = 0, i = 0;
		struct cb_data *d = NULL;
	PPCODE:
		len = av_len(cert_chain) + 1;
		for(i = 0; i &lt; len; i++) {
			SV **sv = av_fetch(cert_chain, i, 0);
			if(!sv || !purple_perl_is_ref_object(*sv)) {
				g_list_free(l);
				warn(&quot;Purple::Certificate::verify: cert_chain: non-purple object in array...&quot;);
				XSRETURN_UNDEF;
			}
			l = g_list_prepend(l, purple_perl_ref_object(*sv));
		}
		l = g_list_reverse(l);

		d = g_new0(struct cb_data, 1);
		d-&gt;cb = newSVsv(ST(3));
		d-&gt;user_data = newSVsv(cb_data);

		purple_certificate_verify(verifier, subject_name, l, (PurpleCertificateVerifiedCallback) cb_cert_verify, d);

		g_list_free(l);

MODULE = Purple::Certificate  PACKAGE = Purple::Certificate::Pool  PREFIX = purple_certificate_pool_
PROTOTYPES: ENABLE

void
purple_certificate_pool_get_idlist(pool)
	Purple::Certificate::Pool pool
	PREINIT:
		GList *l, *b;
	PPCODE:
		b = purple_certificate_pool_get_idlist(pool);
		for(l = b; l; l = l-&gt;next) {
			XPUSHs(sv_2mortal(newSVpv(l-&gt;data, 0)));
		}
		purple_certificate_pool_destroy_idlist(b);

gboolean
purple_certificate_pool_contains(pool, id)
	Purple::Certificate::Pool pool
	const gchar* id

gboolean
purple_certificate_pool_delete(pool, id)
	Purple::Certificate::Pool pool
	const gchar* id

Purple::Certificate::Scheme
purple_certificate_pool_get_scheme(pool)
	Purple::Certificate::Pool pool

gchar_own*
purple_certificate_pool_mkpath(pool, id)
	Purple::Certificate::Pool pool
	const gchar* id

Purple::Certificate
purple_certificate_pool_retrieve(pool, id)
	Purple::Certificate::Pool pool
	const gchar* id

gboolean
purple_certificate_pool_store(pool, id, crt)
	Purple::Certificate::Pool pool
	const gchar* id
	Purple::Certificate crt

gboolean
purple_certificate_pool_usable(pool)
	Purple::Certificate::Pool pool

</pre>


<script type="text/javascript">
installCallbacks("http://developer.pidgin.im/viewmtn/");
</script>

<div id="footer">
  ViewMTN 0.10<br />
</div>

</div> <!-- #Container -->
</div> <!-- #BodyWrapper -->
</body>
</html>
