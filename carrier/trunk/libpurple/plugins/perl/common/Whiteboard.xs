<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
<title>Pidgin ViewMTN: File libpurple/plugins/perl/common/Whiteboard.xs in revision [61b1e768..]</title>
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
<strong>File libpurple/plugins/perl/common/Whiteboard.xs</strong>:
<a href="http://developer.pidgin.im/viewmtn/revision/filechanges/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Whiteboard.xs">Changes</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/file/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Whiteboard.xs">View</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/downloadfile/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Whiteboard.xs">Download</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/filechanges/rss/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Whiteboard.xs">RSS</a>
</div>


<p>
Below is the file 'libpurple/plugins/perl/common/Whiteboard.xs' from this revision. You can also 
<a href="http://developer.pidgin.im/viewmtn/revision/downloadfile/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Whiteboard.xs">download the file</a>.
</p>

<pre class="code">
#include &quot;module.h&quot;

MODULE = Purple::Whiteboard  PACKAGE = Purple::Whiteboard  PREFIX = purple_whiteboard_
PROTOTYPES: ENABLE

void
purple_whiteboard_clear(wb)
	Purple::Whiteboard wb

Purple::Whiteboard
purple_whiteboard_create(account, who, state)
	Purple::Account account
	const char* who
	int state

void
purple_whiteboard_destroy(wb)
	Purple::Whiteboard wb

void
purple_whiteboard_draw_line(wb, x1, y1, x2, y2, color, size)
	Purple::Whiteboard wb
	int x1
	int y1
	int x2
	int y2
	int color
	int size

void
purple_whiteboard_draw_point(wb, x, y, color, size)
	Purple::Whiteboard wb
	int x
	int y
	int color
	int size

Purple::Whiteboard
purple_whiteboard_get_session(account, who)
	Purple::Account account
	const char* who

void
purple_whiteboard_send_brush(wb, size, color)
	Purple::Whiteboard wb
	int size
	int color

void
purple_whiteboard_send_clear(wb)
	Purple::Whiteboard wb

void
purple_whiteboard_set_brush(wb, size, color)
	Purple::Whiteboard wb
	int size
	int color

void
purple_whiteboard_set_dimensions(wb, width, height)
	Purple::Whiteboard wb
	int width
	int height

gboolean
purple_whiteboard_get_brush(wb, OUTLIST int size, OUTLIST int color)
	Purple::Whiteboard wb
	PROTOTYPE: $

gboolean
purple_whiteboard_get_dimensions(wb, OUTLIST int width, OUTLIST int height)
	Purple::Whiteboard wb
	PROTOTYPE: $

void
purple_whiteboard_start(wb)
	Purple::Whiteboard wb

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
