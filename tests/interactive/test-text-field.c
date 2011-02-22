#include <stdlib.h>
#include <gmodule.h>
#include <clutter/clutter.h>

static void
on_entry_paint (ClutterActor *actor,
                gpointer      data)
{
  ClutterActorBox allocation = { 0, };
  gfloat width, height;

  clutter_actor_get_allocation_box (actor, &allocation);
  clutter_actor_box_clamp_to_pixel (&allocation);
  clutter_actor_box_get_size (&allocation, &width, &height);

  cogl_set_source_color4ub (255, 255, 255, 24);
#if 0
  /* this spills over to the next actor in the paint cycle, and retains
   * the same source color
   */
  cogl_path_round_rectangle (0, 0, width, height, 4.0, 1.0);
  cogl_path_stroke ();
#else
  cogl_rectangle (0, 0, width, height);
#endif
}

static void
on_entry_activate (ClutterText *text,
                   gpointer     data)
{
  g_print ("Text activated: %s (cursor: %d, selection at: %d)\n",
           clutter_text_get_text (text),
           clutter_text_get_cursor_position (text),
           clutter_text_get_selection_bound (text));
}

#define is_hex_digit(c)         (((c) >= '0' && (c) <= '9') || \
                                 ((c) >= 'a' && (c) <= 'f') || \
                                 ((c) >= 'A' && (c) <= 'F'))
#define to_hex_digit(c)         (((c) <= '9') ? (c) - '0' : ((c) & 7) + 9)

static gboolean
on_captured_event (ClutterText *text,
                   ClutterEvent *event,
                   gpointer      dummy G_GNUC_UNUSED)
{
  gboolean is_unicode_mode = FALSE;
  gunichar c;
  guint keyval;

  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;

  is_unicode_mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (text),
                                                        "unicode-mode"));

  c = clutter_event_get_key_unicode (event);
  keyval = clutter_event_get_key_symbol (event);
  if (keyval == CLUTTER_KEY_u)
    {
      ClutterModifierType mods = clutter_event_get_state (event);

      if (is_unicode_mode)
        {
          GString *str = g_object_get_data (G_OBJECT (text), "unicode-str");

          clutter_text_set_preedit_string (text, NULL, NULL, 0);

          g_object_set_data (G_OBJECT (text), "unicode-mode",
                             GINT_TO_POINTER (FALSE));
          g_object_set_data (G_OBJECT (text), "unicode-str",
                             NULL);

          g_string_free (str, TRUE);

          return FALSE;
        }

      if ((mods & CLUTTER_CONTROL_MASK) &&
          (mods & CLUTTER_SHIFT_MASK))
        {
          PangoAttrList *attrs;
          PangoAttribute *a;
          GString *str = g_string_sized_new (5);

          g_string_append (str, "u");

          g_object_set_data (G_OBJECT (text),
                             "unicode-mode",
                             GINT_TO_POINTER (TRUE));
          g_object_set_data (G_OBJECT (text),
                             "unicode-str",
                             str);

          attrs = pango_attr_list_new ();

          a = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
          a->start_index = 0;
          a->end_index = str->len;
          pango_attr_list_insert (attrs, a);

          clutter_text_set_preedit_string (text, str->str, attrs, str->len);

          pango_attr_list_unref (attrs);

          return TRUE;
        }

      return FALSE;
    }
  else if (is_unicode_mode && is_hex_digit (c))
    {
      GString *str = g_object_get_data (G_OBJECT (text), "unicode-str");
      PangoAttrList *attrs;
      PangoAttribute *a;
      gchar buf[8];
      gsize len;

      len = g_unichar_to_utf8 (c, buf);
      buf[len] = '\0';

      g_string_append (str, buf);

      g_print ("added '%s' to '%s' (len:%d)",
               buf,
               str->str,
               (int) str->len);

      attrs = pango_attr_list_new ();

      a = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
      a->start_index = 0;
      a->end_index = str->len;
      pango_attr_list_insert (attrs, a);

      clutter_text_set_preedit_string (text, str->str, attrs, str->len);

      pango_attr_list_unref (attrs);

      return TRUE;
    }
  else if (is_unicode_mode && (keyval == CLUTTER_KEY_BackSpace))
    {
      GString *str = g_object_get_data (G_OBJECT (text), "unicode-str");
      PangoAttrList *attrs;
      PangoAttribute *a;

      g_string_truncate (str, str->len - 1);

      attrs = pango_attr_list_new ();

      a = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
      a->start_index = 0;
      a->end_index = str->len;
      pango_attr_list_insert (attrs, a);

      clutter_text_set_preedit_string (text, str->str, attrs, str->len);

      pango_attr_list_unref (attrs);

      return TRUE;
    }
  else if (is_unicode_mode &&
           (keyval == CLUTTER_KEY_Return ||
            keyval == CLUTTER_KEY_KP_Enter ||
            keyval == CLUTTER_KEY_ISO_Enter ||
            keyval == CLUTTER_KEY_KP_Space))
    {
      GString *str = g_object_get_data (G_OBJECT (text), "unicode-str");
      const gchar *contents = clutter_text_get_text (text);
      gunichar uchar = 0;
      gchar ch;
      gint i;

      clutter_text_set_preedit_string (text, NULL, NULL, 0);

      g_object_set_data (G_OBJECT (text), "unicode-mode",
                         GINT_TO_POINTER (FALSE));
      g_object_set_data (G_OBJECT (text), "unicode-str",
                         NULL);

      for (i = 0; i < str->len; i++)
        {
          ch = str->str[i];

          if (is_hex_digit (ch))
            uchar += ((gunichar) to_hex_digit (ch) << ((4 - i) * 4));
        }

      g_assert (g_unichar_validate (uchar));

      g_string_overwrite (str, 0, contents);
      g_string_insert_unichar (str,
                               clutter_text_get_cursor_position (text),
                               uchar);

      i = clutter_text_get_cursor_position (text);
      clutter_text_set_text (text, str->str);

      if (i >= 0)
        i += 1;
      else
        i = -1;

      clutter_text_set_cursor_position (text, i);
      clutter_text_set_selection_bound (text, i);

      g_string_free (str, TRUE);

      return TRUE;
    }
  else
    return FALSE;
}

static ClutterActor *
create_label (const ClutterColor *color,
              const gchar        *text)
{
  ClutterActor *retval = clutter_text_new ();

  clutter_text_set_color (CLUTTER_TEXT (retval), color);
  clutter_text_set_markup (CLUTTER_TEXT (retval), text);
  clutter_text_set_editable (CLUTTER_TEXT (retval), FALSE);
  clutter_text_set_selectable (CLUTTER_TEXT (retval), FALSE);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (retval), TRUE);
  clutter_text_set_ellipsize (CLUTTER_TEXT (retval), PANGO_ELLIPSIZE_END);

  return retval;
}

static ClutterActor *
create_entry (const ClutterColor *color,
              const gchar        *text,
              gunichar            password_char,
              gint                max_length)
{
  ClutterActor *retval = clutter_text_new_full (NULL, text, color);
  ClutterColor selection = { 0, };

  clutter_actor_set_reactive (retval, TRUE);

  clutter_color_darken (color, &selection);

  clutter_text_set_editable (CLUTTER_TEXT (retval), TRUE);
  clutter_text_set_selectable (CLUTTER_TEXT (retval), TRUE);
  clutter_text_set_activatable (CLUTTER_TEXT (retval), TRUE);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (retval), TRUE);
  clutter_text_set_password_char (CLUTTER_TEXT (retval), password_char);
  clutter_text_set_cursor_color (CLUTTER_TEXT (retval), &selection);
  clutter_text_set_max_length (CLUTTER_TEXT (retval), max_length);

  g_signal_connect (retval, "activate",
                    G_CALLBACK (on_entry_activate),
                    NULL);
  g_signal_connect (retval, "paint",
                    G_CALLBACK (on_entry_paint),
                    NULL);
  g_signal_connect (retval, "captured-event",
                    G_CALLBACK (on_captured_event),
                    NULL);

  return retval;
}

G_MODULE_EXPORT gint
test_text_field_main (gint    argc,
                      gchar **argv)
{
  ClutterActor *stage;
  ClutterActor *box, *entry;
  ClutterLayoutManager *table;

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  stage = clutter_stage_new ();
  clutter_stage_set_title (CLUTTER_STAGE (stage), "Text Fields");
  clutter_stage_set_color (CLUTTER_STAGE (stage), CLUTTER_COLOR_Black);
  g_signal_connect (stage, "destroy", G_CALLBACK (clutter_main_quit), NULL);

  table = clutter_table_layout_new ();
  clutter_table_layout_set_column_spacing (CLUTTER_TABLE_LAYOUT (table), 6);
  clutter_table_layout_set_row_spacing (CLUTTER_TABLE_LAYOUT (table), 6);

  box = clutter_box_new (table);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), box);
  clutter_actor_add_constraint (box, clutter_bind_constraint_new (stage, CLUTTER_BIND_WIDTH, -24.0));
  clutter_actor_add_constraint (box, clutter_bind_constraint_new (stage, CLUTTER_BIND_HEIGHT, -24.0));
  clutter_actor_set_position (box, 12, 12);

  clutter_box_pack (CLUTTER_BOX (box),
                    create_label (CLUTTER_COLOR_White, "<b>Input field:</b>"),
                    "row", 0,
                    "column", 0,
                    "x-expand", FALSE,
                    "y-expand", FALSE,
                    NULL);

  entry = create_entry (CLUTTER_COLOR_LightGray, "<i>some</i> text", 0, 0);
  clutter_box_pack (CLUTTER_BOX (box),
                    entry,
                    "row", 0,
                    "column", 1,
                    "x-expand", TRUE,
                    "x-fill", TRUE,
                    "y-expand", FALSE,
                    NULL);

  clutter_box_pack (CLUTTER_BOX (box),
                    create_label (CLUTTER_COLOR_White, "<b>A very long password field:</b>"),
                    "row", 1,
                    "column", 0,
                    "x-expand", FALSE,
                    "y-expand", FALSE,
                    NULL);

  clutter_box_pack (CLUTTER_BOX (box),
                    create_entry (CLUTTER_COLOR_LightGray, "password", '*', 8),
                    "row", 1,
                    "column", 1,
                    "x-expand", TRUE,
                    "x-fill", TRUE,
                    "y-expand", FALSE,
                    NULL);

  clutter_actor_grab_key_focus (entry);
  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
