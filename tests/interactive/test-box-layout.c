/*
 * Copyright 2009 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 */
#include <stdio.h>
#include <stdlib.h>

#include <gmodule.h>

#include <clutter/clutter.h>
#include <cogl/cogl.h>
#include "pango/cogl-pango.h"

#define INSTRUCTIONS \
        "Press v\t\342\236\236\tSwitch horizontal/vertical\n"           \
        "Press h\t\342\236\236\tSwitch homogeneous\n"			\
        "Press p\t\342\236\236\tSwitch pack start/end\n"                \
        "Press s\t\342\236\236\tIncrement spacing (up to 12px)\n"       \
        "Press a\t\342\236\236\tSwitch animations on/off\n"             \
        "Press q\t\342\236\236\tQuit"

static ClutterActor *hover_actor = NULL;
static ClutterActor *box = NULL;
static ClutterActor *label = NULL;
static guint         last_index  = 0;

static void
on_paint (ClutterActor *actor,
          gpointer      user_data)
{
  guint index_ = GPOINTER_TO_UINT (user_data);
  gchar *text = g_strdup_printf ("%u", index_);
  ClutterActorBox alloc = { 0, };
  CoglColor color;
  gint layout_width, layout_height;
  gfloat width, height;
  PangoLayout *layout;

  clutter_actor_get_allocation_box (actor, &alloc);
  clutter_actor_box_get_size (&alloc, &width, &height);

  layout = clutter_actor_create_pango_layout (actor, text);
  pango_layout_get_size (layout, &layout_width, &layout_height);

  cogl_color_init_from_4ub (&color, 0, 0, 0, 255);
  cogl_pango_render_layout (layout,
                            (width - (layout_width / 1024)) / 2,
                            (height - (layout_height / 1024)) / 2,
                            &color, 0);

  g_object_unref (layout);
  g_free (text);
}

static void
enter_event (ClutterActor *actor,
             ClutterEvent *event,
             gpointer data)
{
  clutter_rectangle_set_border_width (CLUTTER_RECTANGLE (actor), 2);
  clutter_rectangle_set_border_color (CLUTTER_RECTANGLE (actor),
                                      CLUTTER_COLOR_Black);

  hover_actor = actor;
}

static void
leave_event (ClutterActor *actor,
             ClutterEvent *event,
             gpointer data)
{
  clutter_rectangle_set_border_width (CLUTTER_RECTANGLE (actor), 0);

  hover_actor = NULL;
}

static gboolean
button_release_event (ClutterActor     *actor,
                      ClutterEvent     *event,
                      ClutterBoxLayout *layout)
{
  gboolean xfill, yfill;
  ClutterBoxAlignment xalign, yalign;
  gint button;

  button = clutter_event_get_button (event);

  if (button == 1)
    {
      clutter_box_layout_get_fill (layout, actor, &xfill, &yfill);
      clutter_box_layout_set_fill (layout, actor,
                                   xfill ? FALSE : TRUE,
                                   yfill ? FALSE : TRUE);
    }
  else
    {
      clutter_box_layout_get_alignment (layout, actor, &xalign, &yalign);

      if (xalign < 2)
        xalign += 1;
      else
        xalign = 0;

      if (yalign < 2)
        yalign += 1;
      else
        yalign = 0;

      clutter_box_layout_set_alignment (layout, actor, xalign, yalign);
    }

  return TRUE;
}

static void
add_actor (ClutterBoxLayout *layout,
           guint             index_)
{
  ClutterActor *rect;
  static gboolean expand = TRUE;
  ClutterColor color;

  clutter_color_from_hls (&color,
                          g_random_double_range (0.0, 360.0),
                          0.5,
                          0.5);
  color.alpha = 255;

  rect = clutter_rectangle_new_with_color (&color);
  clutter_actor_set_size (rect, 32, 64);
  clutter_box_layout_pack (layout, rect, expand,
                           FALSE, /* x-fill */
                           FALSE, /* y-fill */
                           CLUTTER_BOX_ALIGNMENT_CENTER,
                           CLUTTER_BOX_ALIGNMENT_CENTER);

  clutter_actor_set_reactive (rect, TRUE);
  g_signal_connect_after (rect, "paint",
                    G_CALLBACK (on_paint),
                    GUINT_TO_POINTER (index_));
  g_signal_connect (rect, "enter-event", G_CALLBACK (enter_event), NULL);
  g_signal_connect (rect, "leave-event", G_CALLBACK (leave_event), NULL);
  g_signal_connect (rect, "button-release-event",
                    G_CALLBACK (button_release_event),
                    layout);

  expand = !expand;
}

static gboolean
key_release_cb (ClutterActor     *actor,
                ClutterEvent     *event,
                ClutterBoxLayout *layout)
{
  gboolean toggle;
  guint spacing;

  switch (clutter_event_get_key_symbol (event))
    {
    case CLUTTER_KEY_a:
      toggle = clutter_box_layout_get_use_animations (layout);
      clutter_box_layout_set_use_animations (layout, !toggle);
      break;

    case CLUTTER_KEY_v:
      toggle = clutter_box_layout_get_vertical (layout);
      clutter_box_layout_set_vertical (layout, !toggle);
      break;

    case CLUTTER_KEY_h:
      toggle = clutter_box_layout_get_homogeneous (layout);
      clutter_box_layout_set_homogeneous (layout, !toggle);
      break;

    case CLUTTER_KEY_p:
      toggle = clutter_box_layout_get_pack_start (layout);
      clutter_box_layout_set_pack_start (layout, !toggle);
      break;

    case CLUTTER_KEY_s:
      spacing = clutter_box_layout_get_spacing (layout);

      if (spacing > 12)
        spacing = 0;
      else
        spacing++;

      clutter_box_layout_set_spacing (layout, spacing);
      break;

    case CLUTTER_KEY_plus:
      add_actor (layout, last_index++);
      break;

    case CLUTTER_KEY_q:
      clutter_main_quit ();
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static void
stage_size_changed_cb (ClutterActor *stage,
                       const ClutterActorBox *allocation,
                       ClutterAllocationFlags flags,
                       gpointer dummy G_GNUC_UNUSED)
{
  gfloat width, height;

  clutter_actor_box_get_size (allocation, &width, &height);
  clutter_actor_set_size (box, width - 100, height - 100);

  clutter_actor_set_y (label,
                       height - clutter_actor_get_height (label) - 8);
}

G_MODULE_EXPORT int
test_box_layout_main (int argc, char *argv[])
{
  ClutterActor *stage;
  ClutterLayoutManager *layout;
  gint i;

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  stage = clutter_stage_get_default ();
  clutter_stage_set_title (CLUTTER_STAGE (stage), "Box Layout");
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);
  clutter_actor_set_size (stage, 640, 480);

  layout = clutter_box_layout_new ();

  box = clutter_box_new (layout);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), box);

  for (i = 0; i < 5; i++)
    add_actor (CLUTTER_BOX_LAYOUT (layout), last_index++);

  g_signal_connect (stage, "key-release-event",
                    G_CALLBACK (key_release_cb),
                    layout);
  g_signal_connect (stage, "allocation-changed",
                    G_CALLBACK (stage_size_changed_cb),
                    NULL);

  label = clutter_text_new_with_text ("Sans 12px", INSTRUCTIONS);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), label);
  clutter_actor_set_y (label,
                       480 - clutter_actor_get_height (label) - 8);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
