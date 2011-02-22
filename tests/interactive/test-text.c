#include <stdlib.h>

#include <gmodule.h>
#include <clutter/clutter.h>

#define FONT "Mono Bold 24px"

static const gchar *runes =
"ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗ\n"
"ᛋᚳᛖᚪᛚ᛫ᚦᛖᚪᚻ᛫ᛗᚪᚾᚾᚪ᛫ᚷᛖᚻᚹᛦᛚᚳ᛫ᛗᛁᚳᛚᚢᚾ᛫ᚻᛦᛏ᛫ᛞᚫᛚᚪᚾ\n"
"ᚷᛁᚠ᛫ᚻᛖ᛫ᚹᛁᛚᛖ᛫ᚠᚩᚱ᛫ᛞᚱᛁᚻᛏᚾᛖ᛫ᛞᚩᛗᛖᛋ᛫ᚻᛚᛇᛏᚪᚾ᛬\n";

G_MODULE_EXPORT gint
test_text_main (gint    argc,
                gchar **argv)
{
  ClutterActor *stage;
  ClutterActor *text;
  ClutterColor  text_color       = { 0x33, 0xff, 0x33, 0xff };
  ClutterColor  cursor_color     = { 0xff, 0x33, 0x33, 0xff };
  ClutterColor  background_color = { 0x00, 0x00, 0x00, 0xff };

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  stage = clutter_stage_get_default ();
  clutter_stage_set_color (CLUTTER_STAGE (stage), &background_color);

  text = clutter_text_new_full (FONT, "·", &text_color);

  clutter_container_add (CLUTTER_CONTAINER (stage), text, NULL);
  clutter_actor_set_position (text, 40, 30);
  clutter_actor_set_width (text, 1024);
  clutter_text_set_line_wrap (CLUTTER_TEXT (text), TRUE);

  clutter_actor_set_reactive (text, TRUE);
  clutter_stage_set_key_focus (CLUTTER_STAGE (stage), text);

  clutter_text_set_editable (CLUTTER_TEXT (text), TRUE);
  clutter_text_set_selectable (CLUTTER_TEXT (text), TRUE);
  clutter_text_set_cursor_color (CLUTTER_TEXT (text), &cursor_color);

  if (argv[1])
    {
      GError *error = NULL;
      gchar *utf8;

      g_file_get_contents (argv[1], &utf8, NULL, &error);
      if (error)
        {
          utf8 = g_strconcat ("Unable to open '", argv[1], "':\n",
                              error->message,
                              NULL);
          g_error_free (error);
        }

      clutter_text_set_text (CLUTTER_TEXT (text), utf8);
    }
  else
    clutter_text_set_text (CLUTTER_TEXT (text), runes);

  clutter_actor_set_size (stage, 1024, 768);
  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
