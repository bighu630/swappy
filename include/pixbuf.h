#pragma once

#include "swappy.h"

GdkPixbuf *pixbuf_init_from_file(struct swappy_state *state);
GdkPixbuf *pixbuf_get_from_state(struct swappy_state *state);
void pixbuf_save_state_to_folder(GdkPixbuf *pixbuf, char *folder,
                                 char *filename_format);
void pixbuf_save_to_file(GdkPixbuf *pixbuf, char *file);
void pixbuf_save_to_stdout(GdkPixbuf *pixbuf);
void pixbuf_scale_surface_from_widget(struct swappy_state *state,
                                      GtkWidget *widget);

int pixbuf_upload_to_net(GdkPixbuf *pixbuf,char *picgo_path);
char* upload_file_to_net(char *path,char *picgo_path);
void pixbuf_free(struct swappy_state *state);
