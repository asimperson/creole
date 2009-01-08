/*
 * WebKit message styles
 * Copyright (C) 2008 Simo Mattila
 * Copyright (C) 2007 Sean Egan (original version) 
 *
 * Port to Carrier by Alex Sadleir <maxious@lambdacomplex.org>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* This plugins is basically Sadrul's x-chat plugin, but with Webkit
 * instead of xtext.
 */

#define PLUGIN_ID		"gtk-adium-ims"
#define PLUGIN_NAME		"adium-ims"
/* define PLUGIN_AUTHOR		"Sean Egan <seanegan@gmail.com>"
   Change plugin author so that support requests don't go to 
   someone who isn't responsible for all the dodgy hacks ;) */
#define PLUGIN_AUTHOR          "Simo Mattila <simo.h.mattila@gmail.com> / Alex Sadleir <maxious@lambdacomplex.org>"
#define PURPLE_PLUGINS          "Hell yeah"

/* System headers */
#include <string.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

/* Webkit headers */
#include <webkit/webkitwebview.h>
#include <webkit/webkitnetworkrequest.h>

/* Purple headers */
#include <conversation.h>
#include <notify.h>
#include <util.h>
#include <debug.h>
#include <version.h>
#include <gtkplugin.h>

/* Pidgin headers */
#include <gtkconv.h>
#include <gtkimhtml.h>

/* Own includes */
#include "webkit.h"
#include "prefs.h"


static PurpleConversationUiOps *uiops = NULL;

static void (*default_write_conv)(PurpleConversation *conv, const char *name, const char *alias,
						   const char *message, PurpleMessageFlags flags, time_t mtime);
static void (*default_create_conversation)(PurpleConversation *conv);

static void (*default_destroy_conversation)(PurpleConversation *conv);

static void pidgin_webkit_prefs_variant_cb(const char *name, PurplePrefType type,
						gconstpointer val, gpointer data);

static GtkWidget* hack_and_get_widget(PidginConversation *gtkconv);

void pidgin_webkit_write_conv_cb(GtkWidget *webkit, char *script);

gboolean pidgin_webkit_load_message_style(void);

void pidgin_webkit_unload_message_style(void);


typedef struct _PidginWebkit PidginWebkit;
typedef struct _pending_script pending_script;

struct _PidginWebkit
{
	GtkWidget *imhtml;
	GtkWidget *webkit;
};

struct _pending_script
{
	char *script;
	struct _pending_script *next;
};


static GHashTable *webkits = NULL;		/* Hashtable of webkits */
static GHashTable *pending_scripts = NULL;	/* Hashtable of pending messages */

/* Cache the contents of the HTML files */
char *template_html = NULL;                 /* This is the skeleton: some basic javascript mostly */
char *header_html = NULL;                   /* This is the first thing to be appended to any conversation */
char *footer_html = NULL;		    /* This is the last thing appended to the conversation */
char *incoming_content_html = NULL;         /* This is a received message */
char *outgoing_content_html = NULL;         /* And a sent one */
char *incoming_next_content_html = NULL;    /* The same things, but used when someone sends multiple subsequent */
char *outgoing_next_content_html = NULL;    /* messages in a row */
char *status_html = NULL;                   /* Non-IM status messages */
char *basestyle_css = NULL;		    /* Shared CSS attributes */

/* Cache their lenghts too, to pass into g_string_new_len, avoiding crazy allocation */
gsize template_html_len = 0;
gsize header_html_len = 0;
gsize footer_html_len = 0;
gsize incoming_content_html_len = 0;
gsize outgoing_content_html_len = 0;
gsize incoming_next_content_html_len = 0;
gsize outgoing_next_content_html_len = 0;
gsize status_html_len = 0;
gsize basestyle_css_len = 0;

/* And their paths */
char *style_dir = NULL;
char *resources_dir = NULL;
char *template_path = NULL;
char *base_href_path = NULL;
char *css_path = NULL;


static char 
*replace_message_tokens(char *text, gsize len, PurpleConversation *conv, const char *name, const char *alias, 
			     const char *message, PurpleMessageFlags flags, time_t mtime)
{
	GString *str = g_string_new_len(NULL, len);
	char *cur = text;
	char *prev = cur;
	struct tm *tm;

	purple_debug_info("WebKit","replace_message_tokens\n");
	
	tm = localtime(&mtime);

	if(text == NULL) {
		purple_debug_info("WebKit", "text == NULL!\n");
		return NULL;
	}

	while ((cur = strchr(cur, '%'))) {
		char *replace = NULL;
		char *fin = NULL;
			
		if (!strncmp(cur, "%message%", strlen("%message%"))) {
			replace = g_strdup(message);
		} else if (!strncmp(cur, "%messageClasses%",
				    strlen("%messageClasses%"))) {
			replace = g_strdup(flags & PURPLE_MESSAGE_SEND ? "outgoing" :
				  flags & PURPLE_MESSAGE_RECV ? "incoming" :
				  "event");
		} else if (!strncmp(cur, "%shortTime%", strlen("%shortTime%"))) {
			replace = g_strdup(purple_utf8_strftime("%H:%M", tm));
		} else if (!strncmp(cur, "%time", strlen("%time"))) {
			char *format = NULL;
			if (*(cur + strlen("%time")) == '{') {
				char *start = cur + strlen("%time") + 1;
				char *end = strstr(start, "}%");
				if (!end) /* Invalid string */
					continue;
				format = g_strndup(start, end - start);
				fin = end + 1;
			}
			if(format) {
				replace = g_strdup(purple_utf8_strftime(format, tm));
			} else {
				replace = g_strdup(purple_time_format(tm));
			}
			g_free(format);
		} else if (!strncmp(cur, "%userIconPath%",
					strlen("%userIconPath%"))) {
			if (flags & PURPLE_MESSAGE_SEND) {
				if (purple_account_get_bool(conv->account,
						"use-global-buddyicon", TRUE)) {
					replace = g_strdup(purple_prefs_get_path(PIDGIN_PREFS_ROOT "/accounts/buddyicon"));
				} else {
					PurpleStoredImage *img =
						purple_buddy_icons_find_account_icon(conv->account);
					const char *file = purple_imgstore_get_filename(img);
					replace = g_build_filename(purple_user_dir(), "icons", file, NULL);
					purple_debug_info("WebKit", "img filename: %s\n", replace);
				}
				if (replace == NULL || !g_file_test(replace,
							G_FILE_TEST_EXISTS)) {
					replace = g_build_filename(style_dir,
								"Contents",
								"Resources",
								"Outgoing",
								"buddy_icon.png",
								NULL);
				}
				purple_debug_info("WebKit", "userIconPath, outgoing = \"%s\"\n", replace);
			} else if (flags & PURPLE_MESSAGE_RECV) {
				PurpleBuddy *buddy = purple_find_buddy(purple_conversation_get_account(conv), purple_conversation_get_name(conv));
				if (buddy) {
					PurpleContact *contact = purple_buddy_get_contact(buddy);
					if (contact) {
						PurpleStoredImage *custom_img = purple_buddy_icons_node_find_custom_icon((PurpleBlistNode*)contact);
						if (custom_img) {
							/* There is a custom icon for this user */
							const char* file = purple_imgstore_get_filename(custom_img);
							replace = g_build_filename(purple_user_dir(), "icons", file, NULL);
							purple_debug_info("WebKit", "img filename: %s\n", replace);
						}
					}
				}
				if(replace == NULL || !g_file_test(replace, G_FILE_TEST_EXISTS)) {
					PurpleBuddyIcon *icon = purple_conv_im_get_icon(PURPLE_CONV_IM(conv));
						replace = purple_buddy_icon_get_full_path(icon);
				}
				if (replace == NULL || !g_file_test(replace, G_FILE_TEST_EXISTS)) {
					replace = g_build_filename(style_dir,
								"Contents",
								"Resources",
								"Incoming",
								"buddy_icon.png",
								NULL);
				}
				purple_debug_info("WebKit", "userIconPath, incoming = \"%s\"\n", replace);
			}
		} else if (!strncmp(cur, "%senderScreenName%",
					strlen("%senderScreenName%"))) {
			replace = g_strdup(name);
		} else if (!strncmp(cur, "%sender%", strlen("%sender%"))) {
			replace = g_strdup(alias);
		} else if (!strncmp(cur, "%service%", strlen("%service%"))) {
			replace = g_strdup(purple_account_get_protocol_name(conv->account));
		} else {
			cur++;
			continue;
		}

		/* Here we have a replacement to make */
		g_string_append_len(str, prev, cur - prev);
		g_string_append(str, replace);

		g_free(replace);

		/* And update the pointers */
		if (fin) {
			prev = cur = fin + 1;	
		} else {
			prev = cur = strchr(cur + 1, '%') + 1;
		}

	}
	
	/* And wrap it up */
	g_string_append(str, prev);
	return g_string_free(str, FALSE);
}

static char 
*replace_header_tokens(char *text, gsize len, PurpleConversation *conv)
{
	GString *str = g_string_new_len(NULL, len);
	char *cur = text;
	char *prev = cur;

	if (text == NULL)
		return NULL;

	while ((cur = strchr(cur, '%'))) {
		const char *replace = NULL;
		char *fin = NULL;

  		if (!strncmp(cur, "%chatName%", strlen("%chatName%"))) {
			replace = conv->name;
  		} else if (!strncmp(cur, "%sourceName%", strlen("%sourceName%"))) {
			replace = purple_account_get_alias(conv->account);
			if (replace == NULL)
				replace = purple_account_get_username(conv->account);
  		} else if (!strncmp(cur, "%destinationName%", strlen("%destinationName%"))) {
			PurpleBuddy *buddy = purple_find_buddy(conv->account, conv->name);
			if (buddy) {
				replace = purple_buddy_get_alias(buddy);
			} else {
				replace = conv->name;
			}
  		} else if (!strncmp(cur, "%incomingIconPath%", strlen("%incomingIconPath%"))) {
			PurpleBuddyIcon *icon = purple_conv_im_get_icon(PURPLE_CONV_IM(conv));
			replace = purple_buddy_icon_get_full_path(icon);
  		} else if (!strncmp(cur, "%outgoingIconPath%", strlen("%outgoingIconPath%"))) {
  		} else if (!strncmp(cur, "%timeOpened", strlen("%timeOpened"))) {
			char *format = NULL;
			if (*(cur + strlen("%timeOpened")) == '{') {
				char *start = cur + strlen("%timeOpened") + 1;
				char *end = strstr(start, "}%");
				if (!end) /* Invalid string */
					continue;
				format = g_strndup(start, end - start);
				fin = end + 1;
			} 
			replace = purple_utf8_strftime(format ? format : "%X", NULL);
			g_free(format);
		} else {
			continue;
		}

		/* Here we have a replacement to make */
		g_string_append_len(str, prev, cur - prev);
		g_string_append(str, replace);

		/* And update the pointers */
		if (fin) {
			prev = cur = fin + 1;	
		} else {
			prev = cur = strchr(cur + 1, '%') + 1;
		}
	}
	
	/* And wrap it up */
	g_string_append(str, prev);
	return g_string_free(str, FALSE);
}

static char 
*replace_template_tokens(char *text, int len, char *header, char *footer) {
	GString *str = g_string_new_len(NULL, len);

	char **ms = g_strsplit(text, "%@", 6);
	
	if (ms[0] == NULL || ms[1] == NULL || ms[2] == NULL || ms[3] == NULL || ms[4] == NULL || ms[5] == NULL) {
		g_strfreev(ms);
		g_string_free(str, TRUE);
		return NULL;
	}

	g_string_append(str, ms[0]);
	g_string_append(str, base_href_path);
	g_string_append(str, ms[1]);
	if (basestyle_css)
		g_string_append(str, basestyle_css);
	g_string_append(str, ms[2]);
	if (css_path)
		g_string_append(str, css_path);
	g_string_append(str, ms[3]);
	if (header)
		g_string_append(str, header);
	g_string_append(str, ms[4]);
	if (footer)
		g_string_append(str, footer);
	g_string_append(str, ms[5]);
	
	g_strfreev(ms);
	return g_string_free(str, FALSE);
}

void pidgin_webkit_write_conv_cb(GtkWidget *webkit, char *script)
{
	purple_debug_info("WebKit","pidgin_webkit_write_conv_cb\n");
	webkit_web_view_execute_script(WEBKIT_WEB_VIEW(webkit), script);
}

void
webkit_load_started_cb(WebKitWebView *web_view, WebKitWebFrame *frame)
{
	pending_script *pendscript;

	purple_debug_info("WebKit","webkit_load_started_cb\n");

	pendscript = g_hash_table_lookup(pending_scripts, web_view);
	if(!pendscript) {
		purple_debug_info("WebKit","Making new queue for scripts\n");
		pendscript = g_new0(pending_script, 1);
		g_hash_table_insert(pending_scripts, web_view, pendscript);
	}
}

void
webkit_load_finished_cb(WebKitWebView *web_view, WebKitWebFrame *frame)
{
	pending_script *pendscript, *parent;

	purple_debug_info("WebKit","webkit_load_finished_cb\n");
	pendscript = g_hash_table_lookup(pending_scripts, web_view);

	if(!pendscript) {
		purple_debug_info("WebKit","Can't find webkit from hash table!\n");
		return;
	}

	if(!pendscript->script) {
		purple_debug_info("WebKit","No cueued scripts\n");
		g_hash_table_remove(pending_scripts, web_view);
		g_free(pendscript);
		return;
	}

	purple_debug_info("WebKit","Executing cueued scripts\n");
	while(pendscript && pendscript->script) {
		pidgin_webkit_write_conv_cb(GTK_WIDGET(web_view), pendscript->script);
		g_free(pendscript->script);
		parent = pendscript;
		pendscript = pendscript->next;
		g_free(parent);
	}

	purple_debug_info("WebKit","Deleting script queue\n");
	g_hash_table_remove(pending_scripts, web_view);
}


static WebKitNavigationResponse
webkit_navigation_requested_cb(WebKitWebView *web_view, WebKitWebFrame *frame,
				WebKitNetworkRequest *request)
{

	purple_debug_info("WebKit","webkit_navigation_requested_cb\n");

	/* TODO: Adium does not open file: URIs and allows WebKit to handle any
	   navigation of type 'Other' */
	purple_notify_uri(NULL, webkit_network_request_get_uri(request));
	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}
static void pidgin_webkit_prefs_style_cb(const char *name, PurplePrefType type,
					gconstpointer val, gpointer data)
{
	const char *message_style = purple_prefs_get_string(
				"/plugins/gtk/gtk-simom-webkit/message-style");
	char *path, *script;
	GList *list;
	PidginWebkit *gx = NULL;

	path = g_build_filename(purple_user_dir(), "message_styles", message_style, NULL);

	/* Does new style exist? */
	if(!g_file_test(path, G_FILE_TEST_EXISTS)) {
		purple_debug_error("WebKit","Message style doesn't exist\n");
		g_free(path);
		return;
	}

	g_free(path);

	purple_debug_info("WebKit", "Changing to message style \"%s\"\n", message_style);

	pidgin_webkit_unload_message_style();
	pidgin_webkit_load_message_style();

	path = g_build_filename(style_dir, "Contents", "Resources", "main.css", NULL);
	script = g_strdup_printf("setStylesheet(\"baseStyle\", \"%s\")", path);
	g_free(path);

	list = purple_get_conversations();
	while (list)
	{
		gx = g_hash_table_lookup(webkits, list->data);
		if(gx) {
			webkit_web_view_execute_script(WEBKIT_WEB_VIEW(gx->webkit), script);
		} else {
			purple_debug_info("WebKit", "No webkit matching conv!\n");
		}
		list = list->next;
	}

	g_free(script);
}

static void pidgin_webkit_prefs_variant_cb(const char *name, PurplePrefType type,
					gconstpointer val, gpointer data)
{
	const char *variant = purple_prefs_get_string(
				"/plugins/gtk/gtk-simom-webkit/variant");
	char *path, *script;
	GList *list;
	PidginWebkit *gx = NULL;

	if(!strcmp(variant, "")) {
		/* Using main.css */
		purple_debug_info("WebKit", "Changing to default variant\n");
		path = g_build_filename(style_dir, "Contents", "Resources",
						"main.css", NULL);
	} else {
		path = g_build_filename(style_dir, "Contents", "Resources",
						"Variants", variant, NULL);

		/* Does new variant exist? */
		if(!g_file_test(path, G_FILE_TEST_EXISTS)) {
			purple_debug_error("WebKit","Variant doesn't exist\n");
			g_free(path);
			return;
		}

		purple_debug_info("WebKit", "Changing to variant \"%s\"\n", variant);
	}

	g_free(css_path);
	css_path = path;

	script = g_strdup_printf("setStylesheet(\"mainStyle\", \"%s\")", css_path);

	list = purple_get_conversations();
	while (list)
	{
		gx = g_hash_table_lookup(webkits, list->data);
		if(gx) {
			webkit_web_view_execute_script(WEBKIT_WEB_VIEW(gx->webkit), script);
		} else {
			purple_debug_info("WebKit", "No webkit matching conv!\n");
		}
		list = list->next;
	}

	g_free(script);
}

static int
load_message_style() 
{

	char *file;

	style_dir = g_build_filename(purple_user_dir(), "message_styles", purple_prefs_get_string("/plugins/gtk/adium-ims/style"),  NULL);

	purple_debug_info("WebKit", "Message style dir: %s\n", style_dir);

        if (!g_file_test(style_dir, G_FILE_TEST_IS_DIR)) { /* check constant */
                purple_debug_error("adium-ims", "user's custom message style not detected @ %s, falling back to default\n", style_dir );
		style_dir = g_build_filename(DATADIR, "carrier", "webkit", "Cloudbourne.AdiumMessageStyle", NULL);
        }

	resources_dir = g_build_filename(style_dir, "Contents", "Resources", "", NULL);

	template_path = g_build_filename(resources_dir, "Template.html", NULL);
	if (!g_file_get_contents(template_path, &template_html,
					&template_html_len, NULL)) {
		purple_debug_info("WebKit", "No custom Template.html, falling back"
						" to default Template.html\n");
		g_free(template_path);
		purple_debug_info("WebKit", "Using default Template.html\n");
		template_path = g_build_filename(DATADIR, "carrier", "webkit", "Cloudbourne.AdiumMessageStyle", 
				"Contents", "Resources", "Template.html", NULL);
		if (!g_file_get_contents(template_path, &template_html,
						&template_html_len, NULL)) {
			purple_debug_error("WebKit","Can't open default "
							"Template.html!\n");
			g_free(style_dir); style_dir = NULL;
			g_free(template_path); template_path = NULL;
			return FALSE;
		}
	} else {
		purple_debug_info("WebKit", "Using custom Template.html\n");
	}

	/* Although we're really loading the template from the pidgin DATADIR, 
	 *	we need to tell WebKit to pretend that it's loading from the style dir
	 *	so that relative links such as images are valid				*/
	base_href_path = g_build_filename(resources_dir, "Template.html", NULL);


	if (!g_file_get_contents(template_path, &template_html, &template_html_len, NULL)) {
                purple_debug_error("adium-ims", "Template.html not found at %s\n", template_path);
		return FALSE;
	}
	
	file = g_build_filename(resources_dir, "Header.html", NULL);
	g_file_get_contents(file, &header_html, &header_html_len, NULL);

	file = g_build_filename(resources_dir, "Footer.html", NULL);
	g_file_get_contents(file, &footer_html, &footer_html_len, NULL);

	file = g_build_filename(resources_dir, "Outgoing", "Content.html", NULL);
	if (!g_file_get_contents(file, &outgoing_content_html, &outgoing_content_html_len, NULL)) {
                purple_debug_error("adium-ims", "Outgoing/Content.html not found at %s\n", file);
		return FALSE;
	}

	file = g_build_filename(resources_dir, "Outgoing", "NextContent.html", NULL);
	if (!g_file_get_contents(file, &outgoing_next_content_html, &outgoing_next_content_html_len, NULL)) {
		outgoing_next_content_html = outgoing_content_html;
		outgoing_next_content_html_len = outgoing_content_html_len;
	}

	file = g_build_filename(resources_dir, "Incoming", "Content.html", NULL);
	if (!g_file_get_contents(file, &incoming_content_html, &incoming_content_html_len, NULL)) {
                purple_debug_error("adium-ims", "Incoming/Content.html not found at %s\n", file);
		return FALSE;
	}

	file = g_build_filename(resources_dir, "Incoming", "NextContent.html", NULL);
	if (!g_file_get_contents(file, &incoming_next_content_html, &incoming_next_content_html_len, NULL)) {
		incoming_next_content_html = incoming_content_html;
		incoming_next_content_html_len = incoming_content_html_len;
	}

	file = g_build_filename(resources_dir, "Status.html", NULL);
	if (!g_file_get_contents(file, &status_html, &status_html_len, NULL)) {
		purple_debug_error("adium-ims", "Status.html not found at %s\n", file);
		return FALSE;
	}

	if(!strcmp(purple_prefs_get_string("/plugins/gtk/adium-ims/variant"), "")) {
                /* No variant defined, using main.css */
                file = g_build_filename(resources_dir, "main.css", NULL);
        } else {
		file = g_build_filename(resources_dir, "Variants", purple_prefs_get_string("/plugins/gtk/adium-ims/variant"), NULL);
        }


	if (!g_file_get_contents(file, &basestyle_css, &basestyle_css_len, NULL)) {
                purple_debug_error("adium-ims", "Cascading Style Sheet not found at %s\n", file);
                return FALSE;
        }

	g_free(file);
	return TRUE;
}

static void pidgin_webkit_conv_switched_cb(PurpleConversation *active_conv, gpointer data)
{
	GList *list;
	PidginWebkit *gx, *new_gx;
	
	purple_debug_info("WebKit", "conv_switched_cb\n");
		
	/* Let's find out if we already have webkit for earlier conversation
		within this contact */
	list = PIDGIN_CONVERSATION(active_conv)->convs;
	while (list) {
		PurpleConversation *conv = list->data;
		
		if(conv == active_conv) {
			list = list->next;
			continue;
		}
		
		gx = g_hash_table_lookup(webkits, conv);
		if(gx) {
			/* Found earlier conversation, let's update it to active one */
			purple_debug_info("WebKit", "conv_switched_cb: found"
								" earlier conversation with contact\n");
			new_gx = g_new0(PidginWebkit, 1);
			new_gx->webkit = gx->webkit;
			new_gx->imhtml = gx->imhtml;
			g_hash_table_insert(webkits, active_conv, new_gx);
			
			g_free(gx);
			g_hash_table_remove(webkits, conv);
			return;
		}
		list = list->next;
	}
	purple_debug_info("WebKit", "conv_switched_cb: no earlier conversation"
						" with this contact\n");
}

static GtkWidget 
*get_webkit(PurpleConversation *conv)
{
	PidginWebkit *gx;
	/* if a WebKit view doesn't exist yet, we'll have to make one */
	if ((gx = g_hash_table_lookup(webkits, conv)) == NULL)
	{
		PidginConversation *gtkconv;
		GtkWidget *webkit;
		GtkWidget *imhtml = NULL;
		char *header, *footer;
		char *template;
		pending_script *pendscript;
		
		gtkconv = PIDGIN_CONVERSATION(conv);
		if (!gtkconv)
			return NULL;
		imhtml = gtkconv->imhtml;

		gx = g_new0(PidginWebkit, 1);
		load_message_style();
		webkit = webkit_web_view_new();
		header = replace_header_tokens(header_html, header_html_len, conv);
		footer = replace_header_tokens(footer_html, footer_html_len, conv);
		template = replace_template_tokens(template_html, template_html_len + header_html_len, header, footer);
		g_signal_connect(G_OBJECT(webkit), "navigation-requested",
				G_CALLBACK(webkit_navigation_requested_cb), gx);
		g_signal_connect(G_OBJECT(webkit), "load-started",
				G_CALLBACK(webkit_load_started_cb), gx);
		g_signal_connect(G_OBJECT(webkit), "load-finished",
				G_CALLBACK(webkit_load_finished_cb), gx);

		/* To prevent skipping of early messages */
		purple_debug_info("WebKit","Making new queue for scripts\n");
		pendscript = g_new0(pending_script, 1);
		g_hash_table_insert(pending_scripts, webkit, pendscript);

		webkit_web_view_load_string(WEBKIT_WEB_VIEW(webkit), template,
					"text/html", "UTF-8", template_path);

		gx->webkit = webkit;
		gx->imhtml = hack_and_get_widget(gtkconv);
		g_hash_table_insert(webkits, conv, gx);

		g_free(header);
		g_free(template);
	}
	return gx->webkit;
}

static gint
gtk_smiley_tree_lookup (GtkSmileyTree *tree,
                        const gchar   *text)
{
        GtkSmileyTree *t = tree;
        const gchar *x = text;
        gint len = 0;
        const gchar *amp;
        gint alen;

        while (*x) {
                gchar *pos;

                if (!t->values)
                        break;

                if(*x == '&' && (amp = purple_markup_unescape_entity(x, &alen))) {
                        gboolean matched = TRUE;
                        /* Make sure all chars of the unescaped value match */
                        while (*(amp + 1)) {
                                pos = strchr (t->values->str, *amp);
                                if (pos)
                                        t = t->children [GPOINTER_TO_INT(pos) -
					    GPOINTER_TO_INT(t->values->str)];
                                else {
                                        matched = FALSE;
                                        break;
                                }
                                amp++;
                        }
                        if (!matched)
                                break;

                        pos = strchr (t->values->str, *amp);
                }
                else if (*x == '<') /* Because we're all WYSIWYG now, a '<'
                                     * char should only appear as the start of
				     * a tag.  Perhaps a safer (but costlier)
				     * check would be to call gtk_imhtml_is_tag
				     * on it */
                        break;
                else {
                        alen = 1;
                        pos = strchr (t->values->str, *x);
                }

                if (pos)
                        t = t->children [GPOINTER_TO_INT(pos) -
				GPOINTER_TO_INT(t->values->str)];
                else
                        break;

                x += alen;
                len += alen;
        }

        if (t->image)
                return len;

        return 0;
}

static gboolean
gtk_imhtml_is_smiley (GtkIMHtml   *imhtml,
                      GSList      *fonts,
                      const gchar *text,
                      gint        *len)
{
        GtkSmileyTree *tree;
        GtkIMHtmlFontDetail *font;
        char *sml = NULL;

        if (fonts) {
                font = fonts->data;
                sml = font->sml;
        }

        if (!sml)
                sml = imhtml->protocol_name;

        if (!sml || !(tree = g_hash_table_lookup(imhtml->smiley_data, sml)))
                tree = imhtml->default_smilies;

        if (tree == NULL)
                return FALSE;

        *len = gtk_smiley_tree_lookup (tree, text);
        return (*len > 0);
}

char *escape_message(char *text, PidginWebkit *gx)
{
	GString *str = g_string_new(NULL);
	char *cur = text;
	int smileylen = 0;

	purple_debug_info("WebKit","escape_message\n");

	while (cur && *cur) {
		switch (*cur) {
		case '\\':
			g_string_append(str, "\\\\");	
			break;
		case '\"':
			g_string_append(str, "\\\"");
			break;
		case '\r':
			g_string_append(str, "<br/>");
			break;
		case '\n':
			break;
		default:
			if (gx && gtk_imhtml_is_smiley(GTK_IMHTML(gx->imhtml),
			    NULL, cur, &smileylen)) {
				char *smiley_markup, *unescaped;
				char *ws = g_malloc(smileylen * 5);

				purple_debug_info("WebKit", "Smiley found, length %i\n", smileylen);
				g_snprintf (ws, smileylen + 1, "%s", cur);
			        unescaped = purple_unescape_html(ws);
				purple_debug_info("WebKit", "Smiley: \"%s\"\n", unescaped);

				GtkIMHtmlSmiley *imhtml_smiley =
					gtk_imhtml_smiley_get(
						GTK_IMHTML(gx->imhtml),
						GTK_IMHTML(gx->imhtml)->protocol_name, unescaped);
				if(!imhtml_smiley) {
					purple_debug_info("WebKit","imhtml_smiley == NULL!\n");
					g_string_append_c(str, *cur);
					break;
				}
				smiley_markup = g_strdup_printf("<abbr title='%s'><img src='file://%s' alt='%s'/></abbr>",
							imhtml_smiley->smile,
							imhtml_smiley->file,
							imhtml_smiley->smile);
				purple_debug_info("WebKit", "Smiley_markup: \"%s\"\n", smiley_markup);
				g_string_append(str, smiley_markup);
				cur += smileylen - 1;
			} else {
				g_string_append_c(str, *cur);
			}
		}
		cur++;
	}
	return g_string_free(str, FALSE);
}

static void 
purple_webkit_write_conv(PurpleConversation *conv, const char *name, const char *alias,
						   const char *message, PurpleMessageFlags flags, time_t mtime)
{
	purple_debug_info("WebKit","pidgin_webkit_write_conv\n");

	PurpleConversationType type;
	GtkWidget *webkit;
	pending_script *pendscript, *pendscript_new;
	char *stripped;
	char *message_html;
	char *msg;
	char *escape;
	char *script;
	char *linkified;
	char *func = "appendMessage";
	const char *old_name = purple_conversation_get_data(conv, "webkit-prevname");
	PurpleMessageFlags old_flags =
			GPOINTER_TO_INT(purple_conversation_get_data(conv,
							"webkit-lastflags"));

	purple_debug_info("WebKit", "name: \"%s\", old_name: \"%s\"\n", name, old_name);
	purple_debug_info("WebKit", "name: \"%s\", alias: \"%s\", flags 0x%x\n", name, alias,flags);
	purple_debug_info("WebKit", "message: \"%s\"\n", message);

	/* Do the usual stuff first. */
	default_write_conv(conv, name, alias, message, flags, mtime);

	webkit = get_webkit(conv);
	if (webkit==NULL) {
		purple_debug_info("WebKit","write_conv: webkit==NULL!\n");
	}

	if(purple_prefs_get_bool(PIDGIN_PREFS_ROOT "/conversations/show_incoming_formatting")) {
		stripped = g_strdup(message);
	} else {
		stripped = purple_markup_strip_html(message);
	}

	if(flags & PURPLE_MESSAGE_NO_LINKIFY) {
		linkified = stripped;
	} else {
		linkified = purple_markup_linkify(stripped);
		g_free(stripped);
	}

	type = purple_conversation_get_type(conv);
	if (type == PURPLE_CONV_TYPE_CHAT) {
		purple_debug_info("WebKit","write_conv: It's a chat\n");

		if (flags & PURPLE_MESSAGE_SYSTEM) {
			/* System message, let's use Status.html */
			message_html = status_html;
			purple_debug_info("WebKit","write_conv: system message\n");
		} else if (flags & PURPLE_MESSAGE_SEND && old_flags & PURPLE_MESSAGE_SEND) {
			message_html = outgoing_next_content_html;
			func = "appendNextMessage";
			purple_debug_info("WebKit","write_conv: subsequent message out\n");
		} else if (flags & PURPLE_MESSAGE_SEND) {
			message_html = outgoing_content_html;
			purple_debug_info("WebKit","write_conv: first message out\n");
		} else if (flags & PURPLE_MESSAGE_RECV &&
				old_name && !strcmp(name, old_name)) {
			message_html = incoming_next_content_html;
			func = "appendNextMessage";
			purple_debug_info("WebKit","write_conv: subsequent message in\n");
		} else if (flags & PURPLE_MESSAGE_RECV) {
			message_html = incoming_content_html;
			purple_debug_info("WebKit","write_conv: first message in\n");
		} else {
			message_html = status_html;
			purple_debug_info("WebKit","write_conv: status message\n");
		}
	} else {
		switch(type) {
			case PURPLE_CONV_TYPE_IM:
				purple_debug_info("WebKit", "write_conv: It's an IM\n");
				break;
			case PURPLE_CONV_TYPE_MISC:
				purple_debug_info("Webkit", "write_conv: It's MISC, handling as IM...\n");
				break;
			case PURPLE_CONV_TYPE_UNKNOWN:
				purple_debug_info("Webkit", "write_conv: It's UNKNOWN, handling as IM...\n");
				break;
			default:
				purple_debug_info("WebKit", "write_conv: Unknown conversation type, so what is this?!?\n");
				purple_debug_info("WebKit", "write_conv: Handling as IM...\n");
		}

		if (flags & PURPLE_MESSAGE_SYSTEM) {
			/* System message, let's use Status.html */
			message_html = status_html;
			purple_debug_info("WebKit","write_conv: system message\n");
		} else if (flags & PURPLE_MESSAGE_SEND && old_flags & PURPLE_MESSAGE_SEND) {
			message_html = outgoing_next_content_html;
			func = "appendNextMessage";
			purple_debug_info("WebKit","write_conv: subsequent message out\n");
		} else if (flags & PURPLE_MESSAGE_SEND) {
			message_html = outgoing_content_html;
			purple_debug_info("WebKit","write_conv: first message out\n");
		} else if (flags & PURPLE_MESSAGE_RECV &&
			   old_flags & PURPLE_MESSAGE_RECV) {
			message_html = incoming_next_content_html;
			func = "appendNextMessage";
			purple_debug_info("WebKit","write_conv: subsequent message in\n");
		} else if (flags & PURPLE_MESSAGE_RECV) {
			message_html = incoming_content_html;
			purple_debug_info("WebKit","write_conv: first message in\n");
		} else {
			message_html = status_html;
			purple_debug_info("WebKit","write_conv: status message\n");
		}
	}

	purple_conversation_set_data(conv, "webkit-lastflags",
					GINT_TO_POINTER(flags));
	purple_conversation_set_data(conv, "webkit-prevname", strdup(name));

	msg = replace_message_tokens(message_html, 0, conv, name, alias,
					linkified, flags, mtime);
	escape = escape_message(msg, g_hash_table_lookup(webkits, conv));
	script = g_strdup_printf("%s(\"%s\")", func, escape);

	if(!WEBKIT_IS_WEB_VIEW(webkit)) {
		purple_debug_info("WebKit","Don't have web view!\n");
	}

	pendscript = g_hash_table_lookup(pending_scripts, webkit);
	if(pendscript) {
		purple_debug_info("WebKit","WebKit still loading, queuing script...\n");
		if(!pendscript->script) {
		pendscript->script = script;
		} else {
			while(pendscript->next) {
				pendscript = pendscript->next;
			}
			pendscript_new = g_new0(pending_script, 1);
			pendscript_new->script = script;

			pendscript->next = pendscript_new;
		}
	} else {
		pidgin_webkit_write_conv_cb(webkit, script);
		g_free(script);
	}

	g_free(msg);
	g_free(linkified);
	g_free(escape);
}

static GtkWidget*
hack_and_get_widget(PidginConversation *gtkconv)
{
	GtkWidget *tab_cont, *pane, *vbox, *vpaned, *hpaned, *imhtml;
	GList *list;
	
	tab_cont = gtkconv->tab_cont;
	purple_debug_info("adium-ims","Tab container %s hooked\n", G_OBJECT_TYPE_NAME(tab_cont));

	list = gtk_container_get_children(GTK_CONTAINER(tab_cont));
	
	if (!(purple_prefs_get_bool(PIDGIN_PREFS_ROOT "/conversations/funpidgin_auto_size"))) {
		vpaned = list->data;
		vbox = gtk_paned_get_child1(GTK_PANED(vpaned));
	} else {	
		vbox = list->data;
	}
	
	g_list_free(list);
        purple_debug_info("adium-ims","Vbox %s hooked\n", G_OBJECT_TYPE_NAME(vbox));

	list = GTK_BOX(vbox)->children;
	hpaned = ((GtkBoxChild*)list->next->data)->widget;
	vbox = GTK_BIN(hpaned)->child;
        purple_debug_info("adium-ims","Vbox %s hooked\n", G_OBJECT_TYPE_NAME(vbox));
	
	list = GTK_BOX(vbox)->children;
	pane = ((GtkBoxChild*)list->data)->widget;
        purple_debug_info("adium-ims","Pane %s hooked\n", G_OBJECT_TYPE_NAME(pane));

	imhtml = gtk_container_get_children(GTK_CONTAINER(pane))->data;
	purple_debug_info("adium-ims","HTML Container %s hooked\n", G_OBJECT_TYPE_NAME(imhtml));
	return imhtml;
}

static void
purple_conversation_use_webkit(PurpleConversation *conv)
{
	PidginConversation *gtkconv;
	GtkWidget *parent, *webkit, *frame;
	if (purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_IM)
		return;

	gtkconv = PIDGIN_CONVERSATION(conv);
	if (!gtkconv)
		return;

	frame = hack_and_get_widget(gtkconv);
	g_object_ref(frame);
	webkit = get_webkit(conv);
	
	parent = frame->parent;
	gtk_container_remove(GTK_CONTAINER(parent), frame);
	gtk_widget_hide_all(frame);

	gtk_container_add(GTK_CONTAINER(parent), webkit);


	gtk_widget_show(webkit);
}

static void
purple_webkit_create_conv(PurpleConversation *conv)
{

	purple_debug_info("WebKit","pidgin_webkit_create_conv\n");

	default_create_conversation(conv);
	purple_conversation_use_webkit(conv);
}

static void
purple_webkit_destroy_conv(PurpleConversation *conv)
{
	PidginWebkit *gx;

	purple_debug_info("WebKit","pidgin_webkit_destroy_conv\n");

	default_destroy_conversation(conv);

	gx = g_hash_table_lookup(webkits, conv);
	if (gx)
	{
		g_free(gx);
		g_hash_table_remove(webkits, conv);
	}
}

gboolean purple_webkit_load_message_style(void)
{
	char *file;
	const char *message_style = purple_prefs_get_string(
				"/plugins/gtk/gtk-simom-webkit/message-style");
	const char *variant = purple_prefs_get_string(
				"/plugins/gtk/gtk-simom-webkit/variant");
	
	/* Message style root directory */
	style_dir = g_build_filename(purple_user_dir(), "message_styles", message_style, NULL);

	purple_debug_info("WebKit", "Message style dir: %s\n", style_dir);

	template_path = g_build_filename(style_dir, "Contents", "Resources",
						"Template.html", NULL);
	if (!g_file_get_contents(template_path, &template_html,
					&template_html_len, NULL)) {
		purple_debug_info("WebKit", "No custom Template.html, falling back"
						" to default Template.html\n");
		g_free(template_path);
		purple_debug_info("WebKit", "Using default Template.html\n");
		template_path = g_build_filename(purple_user_dir(), "message_styles", "Template.html", NULL);
		if (!g_file_get_contents(template_path, &template_html,
						&template_html_len, NULL)) {
			purple_debug_error("WebKit","Can't open default "
							"Template.html!\n");
			g_free(style_dir); style_dir = NULL;
			g_free(template_path); template_path = NULL;
			return FALSE;
		}
	} else {
		purple_debug_info("WebKit", "Using custom Template.html\n");
	}
	
	file = g_build_filename(style_dir, "Contents", "Resources",
				"Header.html", NULL);
	if(!g_file_get_contents(file, &header_html, &header_html_len, NULL)) {
		purple_debug_info("WebKit","No Header.html\n");
	}
	g_free(file);

	file = g_build_filename(style_dir, "Contents", "Resources",
				"Footer.html", NULL);
	if(!g_file_get_contents(file, &footer_html, &footer_html_len, NULL)) {
		purple_debug_info("WebKit","No Footer.html\n");
	}
	g_free(file);

	file = g_build_filename(style_dir, "Contents", "Resources", "Incoming",
				"Content.html", NULL);
	if (!g_file_get_contents(file, &incoming_content_html,
					&incoming_content_html_len, NULL)) {
		purple_debug_error("WebKit", "Can't open Incoming/Content.html\n");
		g_free(style_dir); style_dir = NULL;
		g_free(template_path); template_path = NULL;
		g_free(header_html); header_html = NULL;
		g_free(footer_html); footer_html = NULL;
		g_free(file);
		return FALSE;
	}
	g_free(file);

	file = g_build_filename(style_dir, "Contents", "Resources", "Incoming",
				"NextContent.html", NULL);
	if (!g_file_get_contents(file, &incoming_next_content_html,
					&incoming_next_content_html_len, NULL)) {
		incoming_next_content_html = g_strdup(incoming_content_html);
		incoming_next_content_html_len = incoming_content_html_len;
		purple_debug_info("WebKit", "No Incoming/NextContent.html\n");
	}
	g_free(file);

	file = g_build_filename(style_dir, "Contents", "Resources", "Outgoing",
				"Content.html", NULL);
	if (!g_file_get_contents(file, &outgoing_content_html,
					&outgoing_content_html_len, NULL)) {
		outgoing_content_html = g_strdup(incoming_content_html);
		outgoing_content_html_len = incoming_content_html_len;
		purple_debug_info("WebKit", "No Outgoing/Content.html\n");
	}
	g_free(file);

	file = g_build_filename(style_dir, "Contents", "Resources", "Outgoing",
				"NextContent.html", NULL);
	if (!g_file_get_contents(file, &outgoing_next_content_html,
				  &outgoing_next_content_html_len, NULL)) {
		outgoing_next_content_html = g_strdup(outgoing_content_html);
		outgoing_next_content_html_len = outgoing_content_html_len;
		purple_debug_info("WebKit", "No Outgoing/NextContent.html\n");
	}
	g_free(file);

	file = g_build_filename(style_dir, "Contents", "Resources",
				"Status.html", NULL);
	if (!g_file_get_contents(file, &status_html, &status_html_len, NULL)) {
		purple_debug_error("WebKit", "Can't open Status.html\n");
		g_free(style_dir); style_dir = NULL;
		g_free(template_path); template_path = NULL;
		g_free(header_html); header_html = NULL;
		g_free(footer_html); footer_html = NULL;
		g_free(incoming_content_html); incoming_content_html = NULL;
		g_free(incoming_next_content_html); incoming_next_content_html = NULL;
		g_free(outgoing_content_html); outgoing_content_html = NULL;
		g_free(outgoing_next_content_html); outgoing_next_content_html = NULL;
		g_free(file);
		return FALSE;
	}
	g_free(file);

	file = g_build_filename(style_dir, "Contents", "Resources",
					"main.css", NULL);
	if(!g_file_get_contents(file, &basestyle_css, &basestyle_css_len, NULL)) {
		purple_debug_error("WebKit", "Can't open main.css!\n");
	}

	if(!strcmp(variant, "")) {
		/* No variant defined, using main.css */
		css_path = g_build_filename(style_dir, "Contents", "Resources",
						"main.css", NULL);
	} else {
		css_path = g_build_filename(style_dir, "Contents", "Resources",
						"Variants", variant, NULL);
	}

	purple_debug_info("WebKit", "css_path: %s\n", css_path);

	return TRUE;
}

void
purple_webkit_unload_message_style(void)
{
	g_free(template_html);
	g_free(header_html);
	g_free(footer_html);
	g_free(incoming_content_html);
	g_free(outgoing_content_html);
	g_free(incoming_next_content_html);
	g_free(outgoing_next_content_html);
	g_free(status_html);
	g_free(style_dir);
	g_free(template_path);
	g_free(css_path);
	g_free(basestyle_css);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	GList *list;
	if (load_message_style() == FALSE) return FALSE;
	uiops = pidgin_conversations_get_conv_ui_ops();

	if (uiops == NULL) {
		purple_debug_error("adium-ims", "UI Ops not defined\n");
		return FALSE;
	}

	/* Use the oh-so-useful uiops. Signals? bleh. */
	default_write_conv = uiops->write_conv;
	uiops->write_conv = purple_webkit_write_conv;

	default_create_conversation = uiops->create_conversation;
	uiops->create_conversation = purple_webkit_create_conv;

	default_destroy_conversation = uiops->destroy_conversation;
	uiops->destroy_conversation = purple_webkit_destroy_conv;

	webkits = g_hash_table_new(g_direct_hash, g_direct_equal);
	pending_scripts = g_hash_table_new(g_direct_hash, g_direct_equal);

	list = purple_get_chats();
	while (list)
	{
		purple_conversation_use_webkit(list->data);
		list = list->next;
	}
	g_free(list);
	
	purple_signal_connect(pidgin_conversations_get_handle(),
	                      "conversation-switched", plugin,
	                      PURPLE_CALLBACK(pidgin_webkit_conv_switched_cb), NULL);
	return TRUE;

}

static void 
remove_webkit(PurpleConversation *conv, PidginWebkit *gx, gpointer null)
{
	GtkWidget *frame, *parent;

	frame = gx->webkit->parent;
	parent = frame->parent;

	GTK_PANED(parent)->child1 = NULL;
	gx->imhtml->parent = NULL;
	gtk_container_add(GTK_CONTAINER(parent), gx->imhtml);
	g_object_unref(gx->imhtml);

	gtk_widget_show_all(gx->imhtml);

	gtk_widget_destroy(frame);
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	/* Restore the default ui-ops */
	uiops->write_conv = default_write_conv;
	uiops->create_conversation = default_create_conversation;
	uiops->destroy_conversation = default_destroy_conversation;
	
	/* Clear up everything */
	g_hash_table_foreach(webkits, (GHFunc)remove_webkit, NULL);
	g_hash_table_destroy(webkits);

	return TRUE;
}


static PidginPluginUiInfo ui_info = {
        plugin_config_frame,
        0 /* page_num (Reserved) */
};


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,		/* Magic			*/
	PURPLE_MAJOR_VERSION,		/* Purple Major Version		*/
	PURPLE_MINOR_VERSION,		/* Purple Minor Version		*/
	PURPLE_PLUGIN_STANDARD,		/* plugin type			*/
	PIDGIN_PLUGIN_TYPE,		/* ui requirement		*/
	0,				/* flags			*/
	NULL,				/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,	/* priority			*/

	PLUGIN_ID,			/* plugin id			*/
	NULL,				/* name				*/
	"0.1",				/* version			*/
	NULL,				/* summary			*/
	NULL,				/* description			*/
	PLUGIN_AUTHOR,			/* author			*/
	"http://funpidgin.sf.net",	/* website			*/

	plugin_load,			/* load				*/
	plugin_unload,			/* unload			*/
	NULL,				/* destroy			*/

	&ui_info,			/* ui_info			*/
	NULL,				/* extra_info			*/
	NULL,				/* prefs_info			*/
	NULL,				/* actions			*/
	NULL,				/* reserved 1			*/
	NULL,				/* reserved 2			*/
	NULL,				/* reserved 3			*/
	NULL				/* reserved 4			*/
};

static void
init_plugin(PurplePlugin *plugin) {        
	purple_prefs_add_none("/plugins/gtk/adium-ims");
	purple_prefs_add_string("/plugins/gtk/adium-ims/style","Cloudbourne.AdiumMessageStyle");
	purple_prefs_add_string("/plugins/gtk/adium-ims/variant","");	
	info.name = "Adium Message Styles (WebKit)";
	info.summary = "Adium-like styled conversation windows";
	info.description = "Chat using Adium's WebKit view.";
}

PURPLE_INIT_PLUGIN(webkit, init_plugin, info)
