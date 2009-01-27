/*
 * WebKit message styles
 * Copyright (C) 2008 Simo Mattila
 *
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

#include <gtk/gtk.h>
#include <dirent.h>
#include <string.h>

#include <purple.h>

#include "prefs.h"
#include "webkit.h"

enum {
	TITLE_COLUMN,
	FILE_COLUMN,
	N_COLUMNS
};

static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
	GtkTreePath *path;
        gchar *vtitle, *vfile, *stitle, *sfile;

	purple_debug_info("WebKit", "Selection changed\n");

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
		path = gtk_tree_model_get_path(model, &iter);
		if(gtk_tree_path_get_depth(path) > 1) {
	                gtk_tree_model_get (model, &iter, TITLE_COLUMN, &vtitle, FILE_COLUMN, &vfile, -1);
			gtk_tree_path_up(path);
			gtk_tree_model_get_iter(model, &iter, path);
	                gtk_tree_model_get (model, &iter, TITLE_COLUMN, &stitle, FILE_COLUMN, &sfile, -1);
		} else {
	                gtk_tree_model_get (model, &iter, TITLE_COLUMN, &stitle, FILE_COLUMN, &sfile, -1);
			vtitle = g_strdup("Default variant");
			vfile = g_strdup("");
		}

		purple_debug_info("WebKit", "Message style \"%s\", directory \"%s\"\n", stitle, sfile);
		purple_debug_info("WebKit", "Variant \"%s\", file \"%s\"\n", vtitle, vfile);
		purple_prefs_set_string("/plugins/gtk/adium-ims/style", sfile);
		purple_prefs_set_string("/plugins/gtk/adium-ims/variant", vfile);
                g_free (stitle);
		g_free(sfile);
		g_free(vtitle);
		g_free(vfile);
        }

}

gboolean
fill_tree(GtkTreeStore *store, GtkTreeView *treeview)
{
	GtkTreeIter iter1, iter2;
	GtkTreeSelection *select;
	GtkTreeModel *model;
	GtkTreePath *path;
	gchar *cur, *file=NULL, *vfile=NULL, *title=NULL, *sdirectory, *vdirectory;
	DIR *dir, *vdir;
	struct dirent *dir_ent;
	gint length;
	const char *message_style = purple_prefs_get_string(
				"/plugins/gtk/adium-ims/style");
	const char *variant = purple_prefs_get_string(
				"/plugins/gtk/adium-ims/variant");


	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	sdirectory = g_build_filename(purple_user_dir(), "message_styles", NULL);
	dir = opendir(sdirectory);
	g_free(sdirectory);
	if(!dir) {
		return FALSE;
	}

	while((dir_ent = readdir(dir)) != NULL) {
		if(!strcmp(dir_ent->d_name, ".") || !strcmp(dir_ent->d_name, "..")) {
			continue;
		}
		file = strdup(dir_ent->d_name);
		cur = strrchr(file, '.');
		if(!cur) {
			continue;
		}
		if(strcmp(cur, ".AdiumMessageStyle")) {
			/* Not a style */
			continue;
		}

		length = strlen(file) - strlen("AdiumMessageStyle");
		title = g_malloc(length);
		strncpy(title, file, length);
		cur = strrchr(title, '.');
		*cur = '\0';

		gtk_tree_store_append (store, &iter1, NULL);
		gtk_tree_store_set(store, &iter1, TITLE_COLUMN, title, FILE_COLUMN, file, -1);
		g_free(title);
		if(!strcmp(file, message_style) && !strcmp(variant, "")) {
			purple_debug_info("WebKit", "Current style found\n");
			gtk_tree_selection_select_iter(select, &iter1);
		}

		vdirectory = g_build_filename(purple_user_dir(), "message_styles", file, "Contents",
						"Resources", "Variants", NULL);

		vdir = opendir(vdirectory);
		g_free(vdirectory);
		if(!vdir) {
			printf("Error opening variant dir\n");
			continue;
		}

		while((dir_ent = readdir(vdir)) != NULL) {
			if(!strcmp(dir_ent->d_name, ".") || !strcmp(dir_ent->d_name, "..")) {
				continue;
			}
			vfile = strdup(dir_ent->d_name);
			cur = strrchr(vfile, '.');
			if(!cur) {
				continue;
			}
			if(strcmp(cur, ".css")) {
				/* Not a variant */
				continue;
			}
			length = strlen(vfile) - strlen("css");
			title = g_malloc(length);
			strncpy(title, vfile, length);
			cur = strrchr(title, '.');
			*cur = '\0';

			gtk_tree_store_append (store, &iter2, &iter1);
			gtk_tree_store_set(store, &iter2, TITLE_COLUMN, title, FILE_COLUMN, vfile, -1);

			if(!strcmp(vfile, variant) && !strcmp(file, message_style)) {
				purple_debug_info("WebKit", "Current style and variant found\n");
				path = gtk_tree_model_get_path(model, &iter2);
				gtk_tree_view_expand_to_path(treeview, path);
				gtk_tree_selection_select_path(select, path);
				gtk_tree_path_free(path);
			}

			g_free(title);
			g_free(vfile);
		}
		g_free(file);
	}

	closedir(dir);

	return TRUE;
}

GtkWidget *
plugin_config_frame(PurplePlugin *plugin)
{
	GtkWidget *treeview, *scroll;
	GtkTreeStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkTreeModel *model;

	store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	scroll = gtk_scrolled_window_new(NULL, NULL);

	gtk_widget_set_size_request(scroll, 200, 300);

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_container_add(GTK_CONTAINER(scroll), treeview);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Message styles and variants", renderer, "text", TITLE_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	fill_tree(store, GTK_TREE_VIEW(treeview));

	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed", G_CALLBACK (tree_selection_changed_cb), NULL);

	gtk_widget_show_all(scroll);

	return scroll;
}

