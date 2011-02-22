#include <stdio.h>
#include <stdlib.h>
#include <gmodule.h>

#undef CLUTTER_DISABLE_DEPRECATED
#include <clutter/clutter.h>

typedef struct
{
  ClutterActor *stage;
  ClutterActor *label;
  ClutterActor *progress;

  ClutterTimeline *timeline;

  volatile gboolean cancelled;
} TestThreadData;

static TestThreadData *
test_thread_data_new (void)
{
  TestThreadData *data;

  data = g_new0 (TestThreadData, 1);

  return data;
}

static void
test_thread_data_free (TestThreadData *data)
{
  g_object_unref (data->progress);
  g_object_unref (data->label);
  g_object_unref (data->stage);
  g_object_unref (data->timeline);

  g_free (data);
}

static gboolean
test_thread_done_idle (gpointer user_data)
{
  TestThreadData *data = user_data;

  g_print ("Thread completed\n");

  clutter_text_set_text (CLUTTER_TEXT (data->label), "Completed");
  clutter_timeline_stop (data->timeline);

  test_thread_data_free (data);

  return FALSE;
}

static GStaticPrivate test_thread_data = G_STATIC_PRIVATE_INIT;

typedef struct
{
  gint count;
  TestThreadData *thread_data;
} TestUpdate;

static gboolean
update_label_idle (gpointer data)
{
  TestUpdate *update = data;
  guint width;
  gchar *text;

  text = g_strdup_printf ("Count to %d", update->count);

  clutter_text_set_text (CLUTTER_TEXT (update->thread_data->label), text);
  clutter_actor_set_width (update->thread_data->label, -1);

  if (update->count == 0)
    width = 0;
  else if (update->count == 100)
    width = 350;
  else
    width = (guint) (update->count / 100.0 * 350.0);

  clutter_actor_set_width (update->thread_data->progress, width);

  g_free (text);
  g_free (update);

  return FALSE;
}

static void
do_something_very_slow (void)
{
  TestThreadData *data;
  gint i;

  data = (TestThreadData *) g_static_private_get (&test_thread_data);
  if (data->cancelled)
    return;

  for (i = 0; i < 100; i++)
    {
      gint msecs;

      msecs = 1 + (int) (100.0 * rand () / ((RAND_MAX + 1.0) / 3));

      /* sleep for a while */
      g_usleep (msecs * 1000);

      if ((i % 10) == 0)
        {
          TestUpdate *update;

          update = g_new (TestUpdate, 1);
          update->count = i;
          update->thread_data = data;

          clutter_threads_add_idle_full (G_PRIORITY_DEFAULT + 30,
                                         update_label_idle,
                                         update, NULL);
        }
    }
}

static gpointer
test_thread_func (gpointer user_data)
{
  TestThreadData *data;

  data = user_data;
  g_static_private_set (&test_thread_data, data, NULL);

  do_something_very_slow ();

  clutter_threads_add_idle_full (G_PRIORITY_DEFAULT + 30,
                                 test_thread_done_idle,
                                 data, NULL);

  return NULL;
}

static ClutterTimeline *timeline   = NULL;
static ClutterActor *count_label   = NULL;
static ClutterActor *help_label    = NULL;
static ClutterActor *progress_rect = NULL;

static gboolean
on_key_press_event (ClutterStage *stage,
                    ClutterEvent *event,
                    gpointer      user_data)
{
  TestThreadData *data;

  switch (clutter_event_get_key_symbol (event))
    {
    case CLUTTER_KEY_s:
      clutter_text_set_text (CLUTTER_TEXT (help_label), "Press 'q' to quit");

      clutter_timeline_start (timeline);
      
      data = test_thread_data_new ();
      data->stage = g_object_ref (stage);
      data->label = g_object_ref (count_label);
      data->progress = g_object_ref (progress_rect);
      data->timeline = g_object_ref (timeline);
      g_thread_create (test_thread_func, data, FALSE, NULL);
      return TRUE;

    case CLUTTER_KEY_q:
      clutter_main_quit ();
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

G_MODULE_EXPORT int
test_threads_main (int argc, char *argv[])
{
  ClutterActor *stage;
  ClutterActor *rect;
  ClutterBehaviour *r_behaviour, *p_behaviour;
  ClutterAlpha *alpha;
  const ClutterKnot knots[] = {
    {  75, 150 },
    { 400, 150 }
  };

  g_thread_init (NULL);
  clutter_threads_init ();
  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  stage = clutter_stage_new ();
  clutter_stage_set_title (CLUTTER_STAGE (stage), "Threading");
  clutter_stage_set_color (CLUTTER_STAGE (stage), CLUTTER_COLOR_Aluminium3);
  clutter_actor_set_size (stage, 600, 300);
  g_signal_connect (stage, "destroy", G_CALLBACK (clutter_main_quit), NULL);
  
  count_label = clutter_text_new_with_text ("Mono 12", "Counter");
  clutter_actor_set_position (count_label, 350, 50);

  help_label = clutter_text_new_with_text ("Mono 12", "Press 's' to start");
  clutter_actor_set_position (help_label, 50, 50);

  rect = clutter_rectangle_new_with_color (CLUTTER_COLOR_LightScarletRed);
  clutter_actor_set_position (rect, 75, 150);
  clutter_actor_set_size (rect, 50, 50);
  clutter_actor_set_anchor_point (rect, 25, 25);
  clutter_actor_set_opacity (rect, 224);

  progress_rect = clutter_rectangle_new_with_color (CLUTTER_COLOR_DarkChameleon);
  clutter_actor_set_position (progress_rect, 50, 225);
  clutter_actor_set_size (progress_rect, 350, 50);

  clutter_container_add (CLUTTER_CONTAINER (stage),
                         count_label, help_label,
                         rect, progress_rect,
                         NULL);

  timeline = clutter_timeline_new (3000);
  clutter_timeline_set_loop (timeline, TRUE);
  clutter_timeline_set_auto_reverse (timeline, TRUE);

  alpha = clutter_alpha_new_full (timeline, CLUTTER_LINEAR);
  r_behaviour = clutter_behaviour_rotate_new (alpha,
                                              CLUTTER_Z_AXIS,
                                              CLUTTER_ROTATE_CW,
                                              0.0, 360.0);
  clutter_behaviour_apply (r_behaviour, rect);

  alpha = clutter_alpha_new_full (timeline, CLUTTER_LINEAR);
  p_behaviour = clutter_behaviour_path_new_with_knots (alpha,
                                                       knots,
                                                       G_N_ELEMENTS (knots));
  clutter_behaviour_apply (p_behaviour, rect);

  g_signal_connect (stage,
                    "button-press-event", G_CALLBACK (clutter_main_quit),
                    NULL);
  g_signal_connect (stage,
                    "key-press-event", G_CALLBACK (on_key_press_event),
                    NULL);

  clutter_actor_show (stage);

  clutter_threads_enter ();
  clutter_main ();
  clutter_threads_leave ();

  g_object_unref (p_behaviour);
  g_object_unref (r_behaviour);
  g_object_unref (timeline);

  return EXIT_SUCCESS;
}
