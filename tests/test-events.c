#include <clutter/clutter.h>

gboolean IsFullScreen = FALSE;

static void
stage_state_cb (ClutterStage    *stage,
		gpointer         data)
{
  gchar *detail = (gchar*)data;

  printf("[stage signal] %s\n", detail);
}

static void
blue_button_cb (ClutterActor    *actor,
		ClutterEvent    *event,
		gpointer         data)
{
  ClutterActor *stage;

  stage = clutter_stage_get_default ();

  if (IsFullScreen)
    IsFullScreen = FALSE;
  else
    IsFullScreen = TRUE;

  g_object_set (stage, "fullscreen", IsFullScreen, NULL);
}

void
key_focus_in_cb (ClutterActor    *actor,
		 gpointer         data)
{
  ClutterActor *focus_box = CLUTTER_ACTOR(data);  

  if (actor == clutter_stage_get_default ())
    {
      clutter_actor_hide (focus_box);
    }
  else
    {
      clutter_actor_show (focus_box);
      clutter_actor_set_position (focus_box,
				  clutter_actor_get_x (actor) - 5,
				  clutter_actor_get_y (actor) - 5);

      clutter_actor_set_size (focus_box,
			      clutter_actor_get_width (actor) + 10,
			      clutter_actor_get_height (actor) + 10);
    }
}

static void
input_cb (ClutterActor    *actor,
	  ClutterEvent    *event,
	  gpointer         data)
{
  
  gchar keybuf[9], *source = (gchar*)data;
  int   len = 0;

  switch (event->type)
    {
    case CLUTTER_KEY_PRESS:
      len = g_unichar_to_utf8 (clutter_keysym_to_unicode (event->key.keyval),
			       keybuf);
      keybuf[len] = '\0';
      printf ("[%s] KEY PRESS '%s'", source, keybuf);
      break;
    case CLUTTER_KEY_RELEASE:
      len = g_unichar_to_utf8 (clutter_keysym_to_unicode (event->key.keyval),
			       keybuf);
      keybuf[len] = '\0';
      printf ("[%s] KEY RELEASE '%s'", source, keybuf);
      break;
    case CLUTTER_MOTION:
      printf("[%s] MOTION", source);
      break;
    case CLUTTER_BUTTON_PRESS:
      printf("[%s] BUTTON PRESS", source);
      break;
    case CLUTTER_2BUTTON_PRESS:
      printf("[%s] BUTTON 2 PRESS", source);
      break;
    case CLUTTER_3BUTTON_PRESS:
      printf("[%s] BUTTON 3 PRESS", source);
      break;
    case CLUTTER_BUTTON_RELEASE:
      printf("[%s] BUTTON RELEASE", source);
      if (clutter_event_get_source (event) == actor)
	clutter_stage_set_key_focus 
	  (CLUTTER_STAGE(clutter_stage_get_default ()), actor);
      break;
    case CLUTTER_SCROLL:
      printf("[%s] BUTTON SCROLL", source);
      break;
    case CLUTTER_STAGE_STATE:
      printf("[%s] STAGE STATE", source);
      break;
    case CLUTTER_DESTROY_NOTIFY:
      printf("[%s] DESTROY NOTIFY", source);
      break;
    case CLUTTER_CLIENT_MESSAGE:
      printf("[%s] CLIENT MESSAGE\n", source);
      break;
    case CLUTTER_DELETE:
      printf("[%s] DELETE", source);
      break;
    case CLUTTER_NOTHING:
      return;
    }

  if (clutter_event_get_source (event) == actor)
    printf(" *source*");
  printf("\n");

}

int
main (int argc, char *argv[])
{
  ClutterActor    *stage, *actor, *focus_box;
  ClutterColor    rcol = { 0xff, 0, 0, 0xff}, 
                  bcol = { 0, 0, 0xff, 0xff },
		  gcol = { 0, 0xff, 0, 0xff },
		  ncol = { 0, 0, 0, 0xff };

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  g_signal_connect (stage, "event", G_CALLBACK (input_cb), "stage");
  g_signal_connect (stage, "fullscreen", 
		    G_CALLBACK (stage_state_cb), "fullscreen");
  g_signal_connect (stage, "unfullscreen", 
		    G_CALLBACK (stage_state_cb), "unfullscreen");
  g_signal_connect (stage, "activate", 
		    G_CALLBACK (stage_state_cb), "activate");
  g_signal_connect (stage, "deactivate", 
		    G_CALLBACK (stage_state_cb), "deactivate");

  focus_box = clutter_rectangle_new_with_color (&ncol);
  clutter_container_add (CLUTTER_CONTAINER(stage), focus_box, NULL);

  actor = clutter_rectangle_new_with_color (&rcol);
  clutter_actor_set_size (actor, 100, 100);
  clutter_actor_set_position (actor, 100, 100);

  clutter_actor_set_reactive (actor);

  clutter_container_add (CLUTTER_CONTAINER(stage), actor, NULL);

  g_signal_connect (actor, "event", G_CALLBACK (input_cb), "red box");
  g_signal_connect (actor, "focus-in", G_CALLBACK (key_focus_in_cb), 
		    focus_box);

  clutter_stage_set_key_focus (CLUTTER_STAGE (stage), actor);

  actor = clutter_rectangle_new_with_color (&gcol);
  clutter_actor_set_size (actor, 100, 100);
  clutter_actor_set_position (actor, 250, 100);

  clutter_actor_set_reactive (actor);

  clutter_container_add (CLUTTER_CONTAINER(stage), actor, NULL);

  g_signal_connect (actor, "event", G_CALLBACK (input_cb), "green box");
  g_signal_connect (actor, "focus-in", G_CALLBACK (key_focus_in_cb), 
		    focus_box);

  actor = clutter_rectangle_new_with_color (&bcol);
  clutter_actor_set_size (actor, 100, 100);
  clutter_actor_set_position (actor, 400, 100);

  clutter_actor_set_reactive (actor);

  clutter_container_add (CLUTTER_CONTAINER(stage), actor, NULL);

  g_signal_connect (actor, "event", G_CALLBACK (input_cb), "blue box");
  g_signal_connect (actor, "focus-in", G_CALLBACK (key_focus_in_cb), 
		    focus_box);
  g_signal_connect (actor, "button-press-event", G_CALLBACK (blue_button_cb), NULL);

  actor = clutter_rectangle_new_with_color (&ncol);
  clutter_actor_set_size (actor, 400, 50);
  clutter_actor_set_position (actor, 100, 250);

  clutter_container_add (CLUTTER_CONTAINER(stage), actor, NULL);

  g_signal_connect (actor, "event", G_CALLBACK (input_cb), "blue box");
  g_signal_connect (actor, "focus-in", G_CALLBACK (key_focus_in_cb), 
		    focus_box);

  g_signal_connect (stage, "focus-in", G_CALLBACK (key_focus_in_cb), 
		    focus_box);


  clutter_actor_show_all (CLUTTER_ACTOR (stage));

  clutter_main();

  return 0;
}
