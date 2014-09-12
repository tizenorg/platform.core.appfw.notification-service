#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include <dirent.h>
#include <notification.h>
#include <dlfcn.h>
#include <dlog.h>


void load_plugins (char *path, int (**fct)(notification_h))
{
	DIR *dir;
	struct dirent *plugins_dir;
	char *plugin_path;
	void *plugin;
	int (*plugin_fct)(notification_h);

	dir = opendir (path);
	if (!dir)
		return;

	while ((plugins_dir = readdir(dir)) != NULL) {
		if (g_str_has_suffix (plugins_dir->d_name, ".so")) {
			plugin_path = g_strconcat (path, G_DIR_SEPARATOR_S, plugins_dir->d_name, NULL);
			plugin = dlopen (plugin_path, RTLD_NOW | RTLD_LOCAL);
			plugin_fct = dlsym (plugin, "display_notification");
			g_free (plugin_path);
			if (!plugin) {
				LOGD("\"%s\" is not a plugin, continuing", plugins_dir->d_name);
				continue;
			} else if (!plugin_fct) {
				LOGD("Plugin \"%s\" incompatible, continuing", plugins_dir->d_name);
				dlclose (plugins_dir->d_name);
				continue;
			} else {
				 /* use the first working plugin, if not configured otherwise */
				LOGD("Plugin \"%s\" compatible, loading...", plugins_dir->d_name);
				*fct = plugin_fct;
				break;
			}
		}
	}

	closedir (dir);
}

int display_notification_text (notification_h noti)
{
	char *pkgname = NULL;
	char *title = NULL;
	char *content = NULL;
	char *image_path = NULL;

	notification_get_pkgname (noti, &pkgname);
	if (pkgname == NULL)
		notification_get_application (noti, &pkgname);
	notification_get_title (noti, &title, NULL);
	notification_get_text (noti, NOTIFICATION_TEXT_TYPE_CONTENT, &content);
	notification_get_image (noti, NOTIFICATION_IMAGE_TYPE_ICON, &image_path);

	LOGD("\nNew Notification : %s", title);
	LOGD("Icon : %s", image_path);
	LOGD("Message : %s", content);

	return 1;
}

void display_notifications_cb (void *data, notification_type_e notif_type)
{
	int (*fct)(notification_h) = data;
	notification_h noti = NULL;
	notification_list_h notification_list = NULL;
	notification_list_h get_list = NULL;

	notification_get_list (NOTIFICATION_TYPE_NOTI, -1, &notification_list);
	if (notification_list) {
		get_list = notification_list_get_head (notification_list);
		while (get_list) {
			noti = notification_list_get_data (get_list);

			 /* if the display function was successful, delete the notification */
			if  ( (*fct)(noti) ) {
				get_list = notification_list_remove(get_list, noti);
				notification_delete(noti);
			}
		}
	}
}


int main (int argc, char **argv)
{
	GMainLoop *mainloop = NULL;
	gboolean error_s;
	notification_error_e error_n;
	int (*disp_fct)(notification_h);

	 /* fall back to pure text notification if no plugin works */
	disp_fct = &display_notification_text;
	LOGD("Checking for display plugins...");
	if (g_file_test (PLUGINSDIR, G_FILE_TEST_IS_DIR))
		load_plugins (PLUGINSDIR, &disp_fct);

retry_socket:
	LOGD("Checking if the notifications server socket exists...");
	error_s = g_file_test ("/tmp/.notification.service", G_FILE_TEST_EXISTS);
	if (!error_s) {
		LOGD("Could not find the notifications server socket");
		sleep (5);
		goto retry_socket;
	}

retry_service:
	LOGD("Checking if the notifications server is available...");
	error_n = notification_resister_changed_cb (display_notifications_cb, (*disp_fct));
	if (error_n != NOTIFICATION_ERROR_NONE) {
		LOGD("Could not register with notifications server");
		sleep (5);
		goto retry_service;
	}

	mainloop = g_main_loop_new (NULL, FALSE);
	if (!mainloop) {
		printf ("Failed to create the GLib main loop\n");
		return -1;
	}

	g_main_loop_run (mainloop);

	return 0;
}
