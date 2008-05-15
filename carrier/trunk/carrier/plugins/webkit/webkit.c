/*
 * Adium Webkit views
 * Copyright (C) 2007 Sean Egan <seanegan@gmail.com>
 * Port to FunPidgin by Alex Sadleir <maxious@lambdacomplex.org>
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
// define PLUGIN_AUTHOR		"Sean Egan <seanegan@gmail.com>"
// Change plugin author so that support requests don't go to 
// someone who isn't responsible for all the dodgy hacks ;)
#define PLUGIN_AUTHOR          "Alex Sadleir <maxious@lambdacomplex.org>"
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

/* Pidgin headers */
#include <gtkconv.h>
#include <gtkimhtml.h>
#include <gtkplugin.h>


static PurpleConversationUiOps *uiops = NULL;

static void (*default_write_conv)(PurpleConversation *conv, const char *name, const char *alias,
						   const char *message, PurpleMessageFlags flags, time_t mtime);
static void (*default_create_conversation)(PurpleConversation *conv);

static void (*default_destroy_conversation)(PurpleConversation *conv);

static GtkWidget* hack_and_get_widget(PidginConversation *gtkconv);

typedef struct _PidginWebkit PidginWebkit;

struct _PidginWebkit
{
	GtkWidget *imhtml;
	GtkWidget *webkit;
};

static GHashTable *webkits = NULL;		/* Hashtable of webkits */

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

	while ((cur = strchr(cur, '%'))) {
		const char *replace = NULL;
		char *fin = NULL;
			
		if (!strncmp(cur, "%message%", strlen("%message%"))) {
			replace = message;
		} else if (!strncmp(cur, "%messageClasses%", strlen("%messageClasses%"))) {
			replace = flags & PURPLE_MESSAGE_SEND ? "outgoing" :
				  flags & PURPLE_MESSAGE_RECV ? "incoming" : "event";
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
			replace = purple_utf8_strftime(format ? format : "%X", NULL);
			g_free(format);
		} else if (!strncmp(cur, "%userIconPath%", strlen("%userIconPath%"))) {
			if (flags & PURPLE_MESSAGE_SEND) {
				if (purple_account_get_bool(conv->account, "use-global-buddyicon", TRUE)) {
					replace = purple_prefs_get_path(PIDGIN_PREFS_ROOT "/accounts/buddyicon");
				} else {
					PurpleStoredImage *img = purple_buddy_icons_find_account_icon(conv->account);
					replace = purple_imgstore_get_filename(img);
				}
				if (replace == NULL || !g_file_test(replace, G_FILE_TEST_EXISTS)) {
					replace = g_build_filename(resources_dir, 
								   "Outgoing", "buddy_icon.png", NULL);
				}
			} else if (flags & PURPLE_MESSAGE_RECV) {
				PurpleBuddyIcon *icon = purple_conv_im_get_icon(PURPLE_CONV_IM(conv));
				replace = purple_buddy_icon_get_full_path(icon);
				if (replace == NULL || !g_file_test(replace, G_FILE_TEST_EXISTS)) {
					replace = g_build_filename(resources_dir,
								   "Incoming", "buddy_icon.png", NULL);
				}
			}
		} else if (!strncmp(cur, "%senderScreenName%", strlen("%senderScreenName%"))) {
			replace = name;
		} else if (!strncmp(cur, "%sender%", strlen("%sender%"))) {
			replace = alias;
		} else if (!strncmp(cur, "%service%", strlen("%service%"))) {
			replace = purple_account_get_protocol_name(conv->account);
		} else {
			cur++;
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
		//	cur++;
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

static WebKitNavigationResponse
webkit_navigation_requested_cb(WebKitWebView *web_view, WebKitWebFrame *frame, WebKitNetworkRequest *request)
{
	/* TODO: Adium does not open file: URIs and allows WebKit to handle any navigation of type 'Other' */
	purple_notify_uri(NULL, webkit_network_request_get_uri(request));
	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

static GtkWidget 
*get_webkit(PurpleConversation *conv)
{
	PidginWebkit *gx;

	if ((gx = g_hash_table_lookup(webkits, conv)) == NULL)
	{
		PidginConversation *gtkconv;
		GtkWidget *webkit;
		GtkWidget *imhtml = NULL;
		char *header, *footer;
		char *template;
		
		gtkconv = PIDGIN_CONVERSATION(conv);
		if (!gtkconv)
			return NULL;
		imhtml = gtkconv->imhtml;

		gx = g_new0(PidginWebkit, 1);

		webkit = webkit_web_view_new();
		header = replace_header_tokens(header_html, header_html_len, conv);
		footer = replace_header_tokens(footer_html, footer_html_len, conv);
		template = replace_template_tokens(template_html, template_html_len + header_html_len, header, footer);
		webkit_web_view_load_string(WEBKIT_WEB_VIEW(webkit), template, "text/html", "UTF-8", base_href_path);

		g_signal_connect(G_OBJECT(webkit), "navigation-requested", G_CALLBACK(webkit_navigation_requested_cb), gx);
		
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
                                        t = t->children [GPOINTER_TO_INT(pos) - GPOINTER_TO_INT(t->values->str)];
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
                                     * char should only appear as the start of a tag.  Perhaps a safer (but costlier)
                                     *                                      * check would be to call gtk_imhtml_is_tag on it */
                        break;
                else {
                        alen = 1;
                        pos = strchr (t->values->str, *x);
                }

                if (pos)
                        t = t->children [GPOINTER_TO_INT(pos) - GPOINTER_TO_INT(t->values->str)];
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

static char 
*escape_message(char *text, PidginWebkit *gx)
{
	GString *str = g_string_new(NULL);
	char *cur = text;
	int smileylen = 0;
	GtkIMHtmlSmiley *imhtml_smiley;
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
			if (gx && gtk_imhtml_is_smiley(GTK_IMHTML(gx->imhtml), NULL, cur, &smileylen)) {
				char *smiley_markup, *unescaped;
				char *ws = g_malloc(smileylen * 5);
				g_snprintf (ws, smileylen + 1, "%s", cur);
			        unescaped = purple_unescape_html(ws);

				imhtml_smiley = gtk_imhtml_smiley_get(GTK_IMHTML(gx->imhtml), NULL, unescaped);
				smiley_markup = g_strdup_printf("<abbr title='%s'><img src='file://%s' alt='%s'/></abbr>",
								 imhtml_smiley->smile, imhtml_smiley->file, imhtml_smiley->smile);
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
static int
load_message_style() 
{

	char *file;

	style_dir = g_build_filename(purple_user_dir(), "styles", purple_prefs_get_string("/plugins/gtk/adium-ims/style"),  NULL);


        if (!g_file_test(style_dir, G_FILE_TEST_IS_DIR)) { // check constant
                purple_debug_error("adium-ims", "user's custom message style not detected @ %s, falling back to default\n", style_dir );
		style_dir = g_build_filename(DATADIR, "carrier", "webkit", "defaultstyle", NULL);
        }

	resources_dir = g_build_filename(style_dir, "Contents", "Resources", "", NULL);

	template_path = g_build_filename(DATADIR, "carrier", "webkit", "Template.html", NULL);
	// Although we're really loading the template from the pidgin DATADIR, 
	//	we need to tell WebKit to pretend that it's loading from the style dir
	//	so that relative links such as images are valid
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

	file = g_build_filename(resources_dir, purple_prefs_get_string("/plugins/gtk/adium-ims/css"), NULL);
	if (!g_file_get_contents(file, &basestyle_css, &basestyle_css_len, NULL)) {
                purple_debug_error("adium-ims", "Cascading Style Sheet not found at %s\n", file);
                return FALSE;
        }

	g_free(file);
	return TRUE;
}

static void 
purple_webkit_write_conv(PurpleConversation *conv, const char *name, const char *alias,
						   const char *message, PurpleMessageFlags flags, time_t mtime)
{
	PurpleConversationType type;
	GtkWidget *webkit;
	char *stripped;
	char *message_html;
	char *msg;
	char *escape;
	char *script;
	char *func = "appendMessage";
	PurpleMessageFlags old_flags = GPOINTER_TO_INT(purple_conversation_get_data(conv, "webkit-lastflags")); 

	/* Do the usual stuff first. */
	default_write_conv(conv, name, alias, message, flags, mtime);
	type = purple_conversation_get_type(conv);
	if (type != PURPLE_CONV_TYPE_IM)
	{
		/* If it's chat, we have nothing to do. */
		return;
	}

	/* So it's an IM. Let's play. */
	load_message_style();
	webkit = get_webkit(conv);
	stripped = g_strdup(message); //purple_markup_strip_html(message);

	if (flags & PURPLE_MESSAGE_SEND && old_flags & PURPLE_MESSAGE_SEND) {
		message_html = outgoing_next_content_html;
		func = "appendNextMessage";
	} else if (flags & PURPLE_MESSAGE_SEND) {
		message_html = outgoing_content_html;
	} else if (flags & PURPLE_MESSAGE_RECV && old_flags & PURPLE_MESSAGE_RECV) {
		message_html = incoming_next_content_html;
		func = "appendNextMessage";
	} else if (flags & PURPLE_MESSAGE_RECV) {
		message_html = incoming_content_html;
	} else {
		message_html = status_html;
	}
	purple_conversation_set_data(conv, "webkit-lastflags", GINT_TO_POINTER(flags));

	msg = replace_message_tokens(message_html, 0, conv, name, alias, stripped, flags, mtime);
	escape = escape_message(msg, g_hash_table_lookup(webkits, conv));
	script = g_strdup_printf("%s(\"%s\")", func, escape);
	webkit_web_view_execute_script(WEBKIT_WEB_VIEW(webkit), script);

	g_free(msg);
	g_free(stripped);
	g_free(escape);
	g_free(script);
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
	//return pane;
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

	load_message_style();
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
	default_create_conversation(conv);
	purple_conversation_use_webkit(conv);
}

static void
purple_webkit_destroy_conv(PurpleConversation *conv)
{
	PidginWebkit *gx;

	default_destroy_conversation(conv);

	gx = g_hash_table_lookup(webkits, conv);
	if (gx)
	{
		g_free(gx);
		g_hash_table_remove(webkits, conv);
	}
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

	list = purple_get_chats();
	while (list)
	{
		purple_conversation_use_webkit(list->data);
		list = list->next;
	}
	g_free(list);
	
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

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin) {
        PurplePluginPrefFrame *frame;
        PurplePluginPref *ppref;
	char *userstyles;
	gchar *file;
	GDir *dir;
	userstyles = g_build_filename(purple_user_dir(), "styles", NULL);
        frame = purple_plugin_pref_frame_new();

        ppref = purple_plugin_pref_new_with_label(g_strconcat("To install a Message Style, extract to: ", userstyles,"\n\
Default style based on tutorial.AdiumMessageStyle by Mark Fickett (Perez)", NULL));
        purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_INFO);
        purple_plugin_pref_frame_add(frame, ppref);

        ppref = purple_plugin_pref_new_with_name_and_label(
                                                        "/plugins/gtk/adium-ims/style",
							"Style:");
        purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
        purple_plugin_pref_add_choice(ppref, "Cloudborne (Default)", "default");
	dir = g_dir_open (userstyles, 0, NULL);
	while ( (file = (gchar*) g_dir_read_name(dir))) 
	{
		gchar *gfilepath =  g_build_filename(userstyles, file, NULL);
		if (g_file_test (gfilepath, G_FILE_TEST_IS_DIR) && g_ascii_strcasecmp(file,"__MACOSX") != 0) {
			purple_plugin_pref_add_choice(ppref, g_strsplit(file,".",2)[0], file);	
		}
	}
        purple_plugin_pref_frame_add(frame, ppref);

/*        ppref = purple_plugin_pref_new_with_name_and_label(
                                                        "/plugins/gtk/adium-ims/css",
                                                        "Variation:");
        purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
        purple_plugin_pref_add_choice(ppref, "main.css (Default)", "main.css");
        dir = g_dir_open ( g_build_filename(userstyles, "Variants", NULL), 0, NULL);
        while ( (file = (gchar*) g_dir_read_name(dir)))
        {
                gchar *gfilepath =  g_build_filename(userstyles, file, NULL);
                if (g_file_test (gfilepath, G_FILE_TEST_IS_DIR) && file !="__MACOSX") {
                        file = g_strsplit(file,".",2)[0];         
                        purple_plugin_pref_add_choice(ppref, file, file);
                }
        }
        purple_plugin_pref_frame_add(frame, ppref); */

        return frame;
} 


static PurplePluginUiInfo prefs_info = {
        get_plugin_pref_frame,
        0,   /* page_num (Reserved) */
        NULL, /* frame (Reserved) */
        /* Padding */
        NULL,
        NULL,
        NULL,
        NULL
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
	"http://pidgin.im",		/* website			*/

	plugin_load,			/* load				*/
	plugin_unload,			/* unload			*/
	NULL,				/* destroy			*/

	NULL,				/* ui_info			*/
	NULL,				/* extra_info			*/
	&prefs_info,			/* prefs_info			*/
	NULL,				/* actions			*/
	NULL,				/* reserved 1			*/
	NULL,				/* reserved 2			*/
	NULL,				/* reserved 3			*/
	NULL				/* reserved 4			*/
};

static void
init_plugin(PurplePlugin *plugin) {        
	purple_prefs_add_none("/plugins/gtk/adium-ims");
	purple_prefs_add_string("/plugins/gtk/adium-ims/style","default");
	purple_prefs_add_string("/plugins/gtk/adium-ims/css","main.css");	
	info.name = "Adium Message Styles (WebKit)";
	info.summary = "Adium-like styled conversation windows";
	info.description = "Chat using Adium's WebKit view.";
}

PURPLE_INIT_PLUGIN(webkit, init_plugin, info)
