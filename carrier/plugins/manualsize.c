/*
 *
 * Pidgin 2.4 manual entry area height sizing plugin
 * License: GPLv2, see http://www.fsf.org/ for a full text
 *
 * Copyright (C) 2008, Artemy Kapitula <dalt74@gmail.com>
 *
 * Version 0.5a
 *
 */

#include "internal.h"
#include "pidgin.h"
#include "gtkprefs.h"

#include "conversation.h"
#include "prefs.h"
#include "signals.h"
#include "version.h"
#include "debug.h"

#include "gtkplugin.h"
#include "gtkutils.h"
#include "gtkimhtml.h"

#ifndef _WIN32
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#define NOTIFY_PLUGIN_ID "pidgin-entry-manual-height"
#define PLUGIN_VERSION "0.5a"

static PurplePlugin * myself = NULL;
static PurplePlugin * my_plugin = NULL;
    // Static plugin registration info

static gboolean page_added = FALSE;
    // The flag of page has been added.
    // It's used to track a case when we add a second page and should to do some
    // additional work to track a page resize issues

static GList * books_connected = NULL;
    // List of notebooks we connected to. When plugin is unloaded,
    // we will disconnect our handler for a "page-added" and "page-removed" signals

static GList * connected_convs = NULL;
    // List of lower_hbox'es. We connect it size-allocated signal
    // so when it's size changed, we store new size immediately,
    // not when conversation closed

static void save_im_size( gint sz ) {
	purple_prefs_set_int( "/plugins/manualsize/im_entry_height", sz );
}

static void save_chat_size( gint sz ) {
	purple_prefs_set_int( "/plugins/manualsize/chat_entry_height", sz );
}


//
// Find a first "placed" objects (the object that has allocation with a height > 1)
// and it's internal height.
//
// It's required because when creating a non-first page in the notebook,
// the widget of the added page has allocation->heigth = 1, and we cannot
// use it as a base for evaluating position of separator in a GtkVPaned
//
static GtkWidget * find_placed_object(GtkWidget * w, int * client_height) {
        GtkWidget * ret;
        int border_width;
        border_width = gtk_container_get_border_width( GTK_CONTAINER(w) );
        if ((w->allocation.height > 1)||(gtk_widget_get_parent(w)==NULL)) {
                *client_height = w->allocation.height;
                return w;
        } else {
                ret = find_placed_object( gtk_widget_get_parent(w), client_height );
                *client_height = *client_height - border_width + 2;
                return ret;
        }
}

//
// Find a widget of parent_class in the widget's parents
//
static GtkWidget * get_parent_of_class(GtkWidget * w, char * parent_class) {
	if (strcmp(GTK_OBJECT_TYPE_NAME(w),parent_class)==0) return w;
	if (gtk_widget_get_parent(w)==NULL) return NULL;
	return get_parent_of_class(gtk_widget_get_parent(w),parent_class);
}

//
// Find a GtkNotebook in the widget's parents
// It's used to find a GtkNotebook in a conversation window
// to attach a "page-added" signal handler
//
static GtkWidget * get_notebook(GtkWidget * w) {
	return get_parent_of_class( w, "GtkNotebook" );
}

//
// Find a GtkWindows in the widget's parents
//
static GtkWidget * get_window(GtkWidget * w) {
	return get_parent_of_class( w, "GtkWindow" );
}

//
// Signal handler. Triggers a page_added flag.
//
static void
on_page_add( GtkNotebook * book, GtkWidget * widget, guint page_num, gpointer user_data ) {
	page_added = TRUE;
	return;
}

//
// When removing last page, forget this notebook
//
static void
on_page_remove( GtkNotebook * book, GtkWidget * widget, guint page_num, gpointer user_data ) {
	if (gtk_notebook_get_n_pages(book) == 0) {
		books_connected = g_list_remove( books_connected, book );
	}
}

//
// Attach a handlers on a notebook if it is not already attached
// Adds a notebook into a tracked objects list
//
static void
connect_notebook_handler(GtkNotebook * notebook) {
	GList * item = g_list_find( books_connected, notebook );
	if (!item) {
		g_signal_connect_after( notebook, "page-added", (GCallback)on_page_add, NULL );
		g_signal_connect_after( notebook, "page-removed", (GCallback)on_page_remove, NULL );
		books_connected = g_list_append( books_connected, notebook );
	}
}

//
// Callback - called when lower_hbox resized.
// Stores entry area height into a preferences.
//
static void lower_hbox_resized( GtkWidget * w, GtkAllocation * a, gpointer conv ) {
	PidginConversation * gtk_conv = (PidginConversation*)conv;
	gboolean chat = (gtk_conv->active_conv->type == PURPLE_CONV_TYPE_CHAT);
	if (chat) {
		save_chat_size( a->height );
	} else {
		save_im_size( a->height );
	}
}

//
// Register lower_hbox and connect a handler on it
//
static void
connect_resize_handler(GtkWidget * lower_hbox, PidginConversation * conv) {
	connected_convs = g_list_append( connected_convs, conv );
	g_signal_connect_after( lower_hbox, "size-allocate", (GCallback)lower_hbox_resized, conv );
}

//
// Rebuild conversation pane.
// Find a conversation pane ("pane")
// Find a parent for a pane ("top")
// Create GtkVPaned ("vpaned")
// Move "pane" from a "top" to the up of "vpaned"
// Move "lower_hbox" of conversation to the bottom "vpaned"
// Insert "vpaned" into a "top"
// Change "vpaned" divider position
// Set focus where it had before we start
//
static void
rebuild_container(PidginConversation * conv) {

	GtkWidget * window;		// Conversation window
	GtkWidget * now_focused;	// Currently focused widget
	GtkWidget * pane;		// Conversation pane
	GtkWidget * top;		// Parent of conversation pane
	GtkWidget * vpaned;		// Our GtkVPaned
	GtkNotebook * notebook;		// Notebook in a conversation window
	gboolean chat;
	int handle_size = 0;
	int parent_area = 0;
	int border_size = 0;
	int new_pos;
	int stored_height;
	GtkPositionType tabpos = -1;
	GValue v;

	pane = gtk_widget_get_parent(GTK_WIDGET(conv->lower_hbox));
	top = gtk_widget_get_parent( pane );
	vpaned= gtk_vpaned_new();
	notebook = GTK_NOTEBOOK(get_notebook(top));
	chat = (conv->active_conv->type == PURPLE_CONV_TYPE_CHAT);
	window = get_window(conv->lower_hbox);
	if (window) {
		now_focused = gtk_window_get_focus(GTK_WINDOW(window));
	} else {
		now_focused = NULL;
	}
	
	stored_height = (chat)?
		purple_prefs_get_int( "/plugins/manualsize/chat_entry_height" )
		:
		purple_prefs_get_int( "/plugins/manualsize/im_entry_height" );

	if (stored_height < 0) stored_height = 128;

	if (notebook) {
		tabpos = gtk_notebook_get_tab_pos( notebook );
		connect_notebook_handler( notebook );
	}

	memset( &v, 0, sizeof(v) );
	g_value_init( &v, G_TYPE_BOOLEAN );
	
	gtk_widget_show( vpaned );

	g_value_set_boolean( &v, TRUE );
	gtk_widget_reparent( pane, vpaned );
	gtk_container_child_set_property( GTK_CONTAINER(vpaned), pane, "resize", &v );

	g_value_set_boolean( &v, FALSE );
	gtk_widget_reparent( conv->lower_hbox, vpaned );
	gtk_container_child_set_property( GTK_CONTAINER(vpaned), conv->lower_hbox, "resize", &v );

	g_value_unset( &v );

	gtk_container_add( GTK_CONTAINER(top), vpaned );

	gtk_widget_style_get( vpaned, "handle-size", &handle_size, NULL );

	find_placed_object( top, &parent_area );
	border_size = gtk_container_get_border_width(GTK_CONTAINER(top));

	new_pos =
		parent_area -
		stored_height -
		handle_size - 
		border_size * 2 - 
		(((page_added==TRUE)&&((tabpos==GTK_POS_TOP)||(tabpos==GTK_POS_BOTTOM)))?24:0);

	gtk_paned_set_position( GTK_PANED(vpaned), new_pos );

	page_added = FALSE;

	//
	// After rebuilding pane we lost focus
	// Take it back to the widget that has been focused when we start
	//
	if (now_focused) {
		gtk_widget_grab_focus( now_focused );
	} else {
		gtk_widget_grab_focus( conv->entry );
	}

	connect_resize_handler( conv->lower_hbox, conv );

}

//
// Signal handler. Called when conversation created, and rebuilds a conversation pane
//
static void
on_display(void* data) {
	PidginConversation * gtkconv = (PidginConversation*)data;
	rebuild_container( gtkconv );
}

//
// Signal handler. Called when conversation destroyed, to store an input area size
//
static void
on_destroy(void * data) {
	PurpleConversation * conv = (PurpleConversation*)data;
	PidginConversation * gtkconv;
	if (conv) {
    	        gtkconv = PIDGIN_CONVERSATION( conv );
	        if (gtkconv) {
	    		g_signal_handlers_disconnect_by_func( gtkconv->lower_hbox, lower_hbox_resized, gtkconv );
	    		connected_convs = g_list_remove( connected_convs, gtkconv );
		}
	}
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	void * gtk_conv_handle = pidgin_conversations_get_handle();
	void * conv_handle = purple_conversations_get_handle();

	myself = plugin;
	purple_prefs_add_none( "/plugins" );
	purple_prefs_add_none( "/plugins/manualsize" );
	purple_prefs_add_int( "/plugins/manualsize/chat_entry_height", 128 );
	purple_prefs_add_int( "/plugins/manualsize/im_entry_height", 128 );

	my_plugin = plugin;

	purple_signal_connect(gtk_conv_handle, "conversation-displayed", plugin,
	                    PURPLE_CALLBACK(on_display), NULL);
	purple_signal_connect(conv_handle, "deleting-conversation", plugin,
	                    PURPLE_CALLBACK(on_destroy), NULL);

	return TRUE;
}

//
// Traverse connected notebooks and remove our signal handler
//
static void
cleanup_pages_callback(gpointer data, gpointer user_data) {
	g_signal_handlers_disconnect_by_func( data, on_page_add, NULL );
	g_signal_handlers_disconnect_by_func( data, on_page_remove, NULL );
}

//
// Traverse connected conversations and remove lower_hbox_resized handler
//
static void
cleanup_convs_callback(gpointer data, gpointer user_data) {
	PidginConversation * conv = (PidginConversation*)data;
	g_signal_handlers_disconnect_by_func( conv->lower_hbox, lower_hbox_resized, conv );
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	myself = NULL;
	g_list_foreach( books_connected, cleanup_pages_callback, NULL );
	g_list_foreach(connected_convs, cleanup_convs_callback, NULL );
	g_list_free( books_connected );
	g_list_free( connected_convs );
	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                           /**< type           */
	PIDGIN_PLUGIN_TYPE,                               /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                          /**< priority       */

	NOTIFY_PLUGIN_ID,                                 /**< id             */
	N_("Entry area manual sizing"),                   /**< name           */
	PLUGIN_VERSION,                                   /**< version        */
	                                                  
	N_("Allows you to change entry area height"),     /**  summary        */
	N_("Allows you to change entry area height"),     /**  description    */
	                                                  
	"Artemy Kapitula <dalt74@gmail.com>",             /**< author         */
	"http://myfotomx.com/builds_en.html",             /**< homepage       */

	plugin_load,                                      /**< load           */
	plugin_unload,                                    /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	NULL,                                             /**< extra_info     */
	NULL,
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
    return;
}

PURPLE_INIT_PLUGIN(manualsize, init_plugin, info)
