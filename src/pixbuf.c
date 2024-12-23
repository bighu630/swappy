#include "pixbuf.h"

#include <cairo/cairo.h>
#include <gio/gunixoutputstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

GdkPixbuf *pixbuf_get_from_state(struct swappy_state *state) {
  guint width = cairo_image_surface_get_width(state->rendering_surface);
  guint height = cairo_image_surface_get_height(state->rendering_surface);
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(state->rendering_surface, 0,
                                                  0, width, height);

  return pixbuf;
}

static void write_file(GdkPixbuf *pixbuf, char *path) {
  GError *error = NULL;
  gdk_pixbuf_savev(pixbuf, path, "png", NULL, NULL, &error);

  if (error != NULL) {
    g_critical("unable to save drawing area to pixbuf: %s", error->message);
    g_error_free(error);
  }
}

void pixbuf_save_state_to_folder(GdkPixbuf *pixbuf, char *folder,
                                 char *filename_format) {
  time_t current_time = time(NULL);
  char *c_time_string;
  char filename[255];
  char path[MAX_PATH];
  size_t bytes_formated;

  c_time_string = ctime(&current_time);
  c_time_string[strlen(c_time_string) - 1] = '\0';
  bytes_formated = strftime(filename, sizeof(filename), filename_format,
                            localtime(&current_time));
  if (!bytes_formated) {
    g_warning(
        "filename_format: %s overflows filename limit - file cannot be saved",
        filename_format);
    return;
  }

  g_snprintf(path, MAX_PATH, "%s/%s", folder, filename);
  g_info("saving surface to path: %s", path);
  write_file(pixbuf, path);
}

void pixbuf_save_to_stdout(GdkPixbuf *pixbuf) {
  GOutputStream *out;
  GError *error = NULL;

  out = g_unix_output_stream_new(STDOUT_FILENO, TRUE);

  gdk_pixbuf_save_to_stream(pixbuf, out, "png", NULL, &error, NULL);

  if (error != NULL) {
    g_warning("unable to save surface to stdout: %s", error->message);
    g_error_free(error);
    return;
  }

  g_object_unref(out);
}

GdkPixbuf *pixbuf_init_from_file(struct swappy_state *state) {
  GError *error = NULL;
  char *file =
      state->temp_file_str != NULL ? state->temp_file_str : state->file_str;
  GdkPixbuf *image = gdk_pixbuf_new_from_file(file, &error);

  if (error != NULL) {
    g_printerr("unable to load file: %s - reason: %s\n", file, error->message);
    g_error_free(error);
    return NULL;
  }

  state->original_image = image;
  return image;
}

void pixbuf_save_to_file(GdkPixbuf *pixbuf, char *file) {
  if (g_strcmp0(file, "-") == 0) {
    pixbuf_save_to_stdout(pixbuf);
  } else {
    write_file(pixbuf, file);
  }
}

int pixbuf_upload_to_net(GdkPixbuf *pixbuf,char *picgo_path) {
    char* tempPath = "/tmp/swappy_upload_temp.png";
    pixbuf_save_to_file(pixbuf, tempPath);
    char* url = upload_file_to_net(tempPath,picgo_path);
    char* exec = malloc(strlen("wl-copy ")+strlen(url)+1);
    strcpy(exec,"wl-copy ");
    strcat(exec,url);
    int ret = system(exec);
    free(exec);
    return ret;
}

char* upload_file_to_net(char *path,char *picgo_path){
    FILE *fp;
    char buffer[64];
    char *last_line = NULL;

    if (strlen(picgo_path) == 0) {
        picgo_path = "picgo";
    }

    // 打开一个管道执行命令
    fp = popen(strcat(strcat(picgo_path," u "), path), "r");
    if (fp == NULL) {
        perror("popen failed");
        return "";
    }

    // 读取命令输出
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        last_line = buffer;
    }

    // 关闭管道
    pclose(fp);
    size_t len = strcspn(last_line, "\n");

    // 如果找到了换行符，就把它替换为 '\0' 来去除
    if (last_line[len] == '\n') {
        last_line[len] = '\0';
    }

    return last_line;
}

void pixbuf_scale_surface_from_widget(struct swappy_state *state,
                                      GtkWidget *widget) {
  GtkAllocation *alloc = g_new(GtkAllocation, 1);
  GdkPixbuf *image = state->original_image;
  gtk_widget_get_allocation(widget, alloc);

  gboolean has_alpha = gdk_pixbuf_get_has_alpha(image);
  cairo_format_t format = has_alpha ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24;
  gint image_width = gdk_pixbuf_get_width(image);
  gint image_height = gdk_pixbuf_get_height(image);

  cairo_surface_t *original_image_surface =
      cairo_image_surface_create(format, image_width, image_height);

  if (!original_image_surface) {
    g_error("unable to create cairo original surface from pixbuf");
    goto finish;
  } else {
    cairo_t *cr;
    cr = cairo_create(original_image_surface);
    gdk_cairo_set_source_pixbuf(cr, image, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
  }

  cairo_surface_t *rendering_surface =
      cairo_image_surface_create(format, image_width, image_height);

  if (!rendering_surface) {
    g_error("unable to create rendering surface");
    goto finish;
  }

  g_info("size of area to render: %ux%u", alloc->width, alloc->height);

finish:
  if (state->original_image_surface) {
    cairo_surface_destroy(state->original_image_surface);
    state->original_image_surface = NULL;
  }
  state->original_image_surface = original_image_surface;

  if (state->rendering_surface) {
    cairo_surface_destroy(state->rendering_surface);
    state->rendering_surface = NULL;
  }
  state->rendering_surface = rendering_surface;

  g_free(alloc);
}

void pixbuf_free(struct swappy_state *state) {
  if (G_IS_OBJECT(state->original_image)) {
    g_object_unref(state->original_image);
  }
}
