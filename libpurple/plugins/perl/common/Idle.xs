<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
<title>Pidgin ViewMTN: File libpurple/plugins/perl/common/Idle.xs in revision [61b1e768..]</title>
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
<strong>File libpurple/plugins/perl/common/Idle.xs</strong>:
<a href="http://developer.pidgin.im/viewmtn/revision/filechanges/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Idle.xs">Changes</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/file/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Idle.xs">View</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/downloadfile/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Idle.xs">Download</a> | 
<a href="http://developer.pidgin.im/viewmtn/revision/filechanges/rss/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Idle.xs">RSS</a>
</div>


<p>
Below is the file 'libpurple/plugins/perl/common/Idle.xs' from this revision. You can also 
<a href="http://developer.pidgin.im/viewmtn/revision/downloadfile/61b1e7687aede70c580182fd1dc5f2d797e70092/libpurple/plugins/perl/common/Idle.xs">download the file</a>.
</p>

<pre class="code">
#include &quot;module.h&quot;

MODULE = Purple::Idle  PACKAGE = Purple::Idle  PREFIX = purple_idle_
PROTOTYPES: ENABLE

void
purple_idle_touch()

void
purple_idle_set(time)
	time_t time

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
