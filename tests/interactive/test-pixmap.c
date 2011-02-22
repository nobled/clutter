#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <gmodule.h>

#undef CLUTTER_DISABLE_DEPRECATED
#include <clutter/clutter.h>

#ifdef CLUTTER_WINDOWING_X11

# include <clutter/x11/clutter-x11.h>
# include <clutter/x11/clutter-x11-texture-pixmap.h>

# include <X11/Xlib.h>
# include <X11/extensions/Xcomposite.h>

#define IMAGE   TESTS_DATADIR G_DIR_SEPARATOR_S "redhand.png"

# ifdef USE_GDKPIXBUF
# include <gdk-pixbuf/gdk-pixbuf.h>

static gboolean disable_x11 = FALSE;
static gboolean disable_animation = FALSE;

static GOptionEntry g_options[] =
{
  { "disable-x11",
    0, 0,
    G_OPTION_ARG_NONE,
    &disable_x11,
    "Disable redirection through X11 pixmap",
    NULL },
  { "disable-animation",
    0, 0,
    G_OPTION_ARG_NONE,
    &disable_animation,
    "Disable the animations",
    NULL },

  { NULL }
};

static void
toggle_texture_quality (ClutterActor *actor)
{
  if (CLUTTER_IS_CONTAINER (actor))
    clutter_container_foreach (CLUTTER_CONTAINER (actor),
                               (ClutterCallback) toggle_texture_quality,
                               NULL);

  if (CLUTTER_IS_TEXTURE (actor))
    {
      ClutterTextureQuality quality;

      quality = clutter_texture_get_filter_quality (CLUTTER_TEXTURE (actor));

      if (quality == CLUTTER_TEXTURE_QUALITY_HIGH)
        quality = CLUTTER_TEXTURE_QUALITY_MEDIUM;
      else
        quality = CLUTTER_TEXTURE_QUALITY_HIGH;

      g_print ("switching to quality %s for %p\n",
               quality == CLUTTER_TEXTURE_QUALITY_HIGH
               ? "high" : "medium",
               actor);

      clutter_texture_set_filter_quality (CLUTTER_TEXTURE (actor), quality);
    }
}

static gboolean
stage_key_release_cb (ClutterActor *actor,
		      ClutterEvent *event,
		      gpointer      data)
{
  switch (clutter_event_get_key_symbol (event))
    {
    case CLUTTER_KEY_q:
    case CLUTTER_KEY_Q:
      clutter_main_quit ();
      break;

    case CLUTTER_KEY_m:
      toggle_texture_quality (actor);
      break;
    }
  return FALSE;
}

static gboolean
draw_arc (Pixmap pixmap)
{
  Display *dpy = clutter_x11_get_default_display ();
  static GC gc = None;
  static int x = 100, y = 100;

  if (gc == None)
    {
      XGCValues gc_values = { 0 };

      gc_values.line_width = 12;
      /* This is an attempt to get a black pixel will full
         opacity. Seemingly the BlackPixel macro and the default GC
         value are a fully transparent color */
      gc_values.foreground = 0xff000000;

      gc = XCreateGC (dpy,
                      pixmap,
                      GCLineWidth | GCForeground,
                      &gc_values);
    }

  XDrawArc (dpy, pixmap, gc, x, y, 100, 100, 0, 360 * 64);

  x -= 5;
  y -= 5;

  return TRUE;
}

static gboolean
stage_button_press_cb (ClutterActor    *actor,
		       ClutterEvent    *event,
		       gpointer         data)
{
  draw_arc ((Pixmap)data);
  return FALSE;
}

Pixmap
create_pixmap (guint *width, guint *height, guint *depth)
{
  Display      *dpy = clutter_x11_get_default_display ();
  Pixmap        pixmap;
  GdkPixbuf    *pixbuf;
  GError       *error = NULL;
  XImage       *image;
  char         *data, *d;
  guchar       *p, *line, *endofline, *end;
  guint         w, h, rowstride;
  GC            gc;
  XGCValues     gc_values = {0};

  pixbuf = gdk_pixbuf_new_from_file (IMAGE, &error);
  if (error)
    g_error ("%s", error->message);

  /* We assume that the image had an alpha channel */
  g_assert (gdk_pixbuf_get_has_alpha (pixbuf));

  w = gdk_pixbuf_get_width  (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  data = g_malloc (w * h * 4);
  image = XCreateImage (dpy,
                        NULL,
                        32,
                        ZPixmap,
                        0,
                        data,
                        w, h,
                        8,
                        w * 4);

  p = gdk_pixbuf_get_pixels (pixbuf);
  d  = data;
  end = p + rowstride*h;

  /* Convert from RGBA as contained in the pixmap to ARGB as used in X */
  for (line = p; line < end ; line += rowstride)
  {
    p = line;
    endofline = p + 4 * w;

    for (p = line; p < endofline; p += 4, d+=4)
      {

#  define r ((guint32)(*(p)))
#  define g ((guint32)(*(p+1)))
#  define b ((guint32)(*(p+2)))
#  define a ((guint32)(*(p+3)))
        guint32 pixel =
            ((a << 24) & 0xFF000000  ) |
            ((r << 16) & 0x00FF0000  ) |
            ((g <<  8) & 0x0000FF00) |
            ((b)       & 0x000000FF );

        *((guint32 *)d) = pixel;

      }
#  undef r
#  undef g
#  undef b
#  undef a

  }

  g_object_unref (pixbuf);

  pixmap = XCreatePixmap (dpy,
                          DefaultRootWindow (dpy),
                          w, h,
                          32);

  gc = XCreateGC (dpy,
                  pixmap,
                  0,
                  &gc_values);

  XPutImage (dpy,
             pixmap,
             gc,
             image,
             0, 0,
             0, 0,
             w, h);

  XFreeGC (dpy, gc);
  XDestroyImage (image);

  if (width) *width = w;
  if (height) *height = h;
  if (depth) *depth = 32;

  return pixmap;
}
# endif /* USE_GDKPIXBUF */

/* each time the timeline animating the label completes, swap the direction */
static void
timeline_completed (ClutterTimeline *timeline,
                    gpointer         user_data)
{
  clutter_timeline_set_direction (timeline,
                                  !clutter_timeline_get_direction (timeline));
  clutter_timeline_start (timeline);
}

G_MODULE_EXPORT int
test_pixmap_main (int argc, char **argv)
{
#ifdef USE_GDKPIXBUF
  GOptionContext      *context;
  Display	      *xdpy;
  int		       screen;
  ClutterActor        *group = NULL, *label, *stage, *tex;
  Pixmap               pixmap;
  const ClutterColor   gry = { 0x99, 0x99, 0x99, 0xFF };
  Window               win_remote;
  guint		       w, h, d;
  GC		       gc;
  ClutterTimeline     *timeline;
  ClutterAlpha	      *alpha;
  ClutterBehaviour    *depth_behavior;
  int		       i;
  int                  row_height;

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  xdpy = clutter_x11_get_default_display ();
  XSynchronize (xdpy, True);

  context = g_option_context_new (" - test-pixmap options");
  g_option_context_add_main_entries (context, g_options, NULL);
  g_option_context_parse (context, &argc, &argv, NULL);

  pixmap = create_pixmap (&w, &h, &d);

  screen = DefaultScreen(xdpy);
  win_remote = XCreateSimpleWindow (xdpy, DefaultRootWindow(xdpy),
				    0, 0, 200, 200,
				    0,
				    WhitePixel(xdpy, screen),
				    WhitePixel(xdpy, screen));

  XMapWindow (xdpy, win_remote);

  stage = clutter_stage_get_default ();
  clutter_actor_set_position (stage, 0, 150);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &gry);

  timeline = clutter_timeline_new (5000);
  g_signal_connect (timeline,
                    "completed", G_CALLBACK (timeline_completed),
                    NULL);

  alpha = clutter_alpha_new_full (timeline, CLUTTER_LINEAR);
  depth_behavior = clutter_behaviour_depth_new (alpha, -2500, 400);

  if (!disable_x11)
    {
      group = clutter_group_new ();
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), group);
      label = clutter_text_new_with_text ("fixed",
                                          "ClutterX11Texture (Window)");
      clutter_container_add_actor (CLUTTER_CONTAINER (group), label);
      tex = clutter_x11_texture_pixmap_new_with_window (win_remote);
      clutter_container_add_actor (CLUTTER_CONTAINER (group), tex);
      clutter_actor_set_position (tex, 0, 20);
      clutter_x11_texture_pixmap_set_automatic (CLUTTER_X11_TEXTURE_PIXMAP (tex),
                                                TRUE);
      clutter_texture_set_filter_quality (CLUTTER_TEXTURE (tex),
                                          CLUTTER_TEXTURE_QUALITY_HIGH);
      clutter_actor_set_position (group, 0, 0);
      if (!disable_animation)
        clutter_behaviour_apply (depth_behavior, group);
    }

  if (group)
    row_height = clutter_actor_get_height (group);
  else
    row_height = 0;

  /* NB: We only draw on the window after being redirected, so we dont
   * have to worry about handling expose events... */
  gc = XCreateGC (xdpy, win_remote, 0, NULL);
  XSetForeground (xdpy, gc, BlackPixel (xdpy, screen));
  XSetLineAttributes(xdpy, gc, 5, LineSolid, CapButt, JoinMiter);

  for (i = 0; i < 10; i++)
    XDrawLine (xdpy, win_remote, gc, 0+i*20, 0, 10+i*20+i, 200);


  group = clutter_group_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), group);
  label = clutter_text_new_with_text ("fixed", "ClutterX11Texture (Pixmap)");
  clutter_container_add_actor (CLUTTER_CONTAINER (group), label);
  tex = clutter_x11_texture_pixmap_new_with_pixmap (pixmap);
  clutter_x11_texture_pixmap_set_automatic (CLUTTER_X11_TEXTURE_PIXMAP (tex),
                                            TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), tex);
  clutter_actor_set_position (tex, 0, 20);
  clutter_texture_set_filter_quality (CLUTTER_TEXTURE (tex),
				      CLUTTER_TEXTURE_QUALITY_HIGH);
  /* oddly, the actor's size is 0 until it is realized, even though
     pixmap-height is set */
  clutter_actor_set_position (group, 0, row_height);
  if (!disable_animation)
    clutter_behaviour_apply (depth_behavior, group);


  g_signal_connect (stage, "key-release-event",
                    G_CALLBACK (stage_key_release_cb), (gpointer)pixmap);
  g_signal_connect (stage, "button-press-event",
                    G_CALLBACK (stage_button_press_cb), (gpointer)pixmap);

  clutter_actor_show (stage);

  if (!disable_animation)
    clutter_timeline_start (timeline);

  g_timeout_add_seconds (1, (GSourceFunc)draw_arc, GUINT_TO_POINTER (pixmap));

  clutter_main ();
# endif /* USE_GDKPIXBUF */

  return EXIT_SUCCESS;
}

#else

int
test_pixmap_main (int argc, char **argv)
{
  return EXIT_SUCCESS;
};

#endif /* CLUTTER_WINDOWING_X11 */
