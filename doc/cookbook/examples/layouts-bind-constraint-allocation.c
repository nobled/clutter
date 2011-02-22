#include <stdlib.h>
#include <clutter/clutter.h>

#define OVERLAY_FACTOR 1.1

static const ClutterColor stage_color = { 0x33, 0x33, 0x55, 0xff };
static const ClutterColor red = { 0xff, 0x00, 0x00, 0xff };
static const ClutterColor blue = { 0x00, 0x00, 0xff, 0x66 };

void
allocation_changed_cb (ClutterActor           *actor,
                       const ClutterActorBox  *allocation,
                       ClutterAllocationFlags  flags,
                       gpointer                user_data)
{
  ClutterActor *overlay = CLUTTER_ACTOR (user_data);

  gfloat width, height, x, y;
  clutter_actor_box_get_size (allocation, &width, &height);
  clutter_actor_box_get_origin (allocation, &x, &y);

  clutter_actor_set_size (overlay,
                          width * OVERLAY_FACTOR,
                          height * OVERLAY_FACTOR);

  clutter_actor_set_position (overlay,
                              x - ((OVERLAY_FACTOR - 1) * width * 0.5),
                              y - ((OVERLAY_FACTOR - 1) * width * 0.5));
}

int
main (int argc, char *argv[])
{
  ClutterActor *stage;
  ClutterActor *actor;
  ClutterActor *overlay;

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 400, 400);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  g_signal_connect (stage, "destroy", G_CALLBACK (clutter_main_quit), NULL);

  actor = clutter_rectangle_new_with_color (&red);
  clutter_actor_set_size (actor, 100, 100);
  clutter_actor_set_position (actor, 150, 150);

  overlay = clutter_rectangle_new_with_color (&blue);

  g_signal_connect (actor,
                    "allocation-changed",
                    G_CALLBACK (allocation_changed_cb),
                    overlay);

  clutter_container_add (CLUTTER_CONTAINER (stage), actor, overlay, NULL);

  clutter_actor_animate (actor, CLUTTER_LINEAR, 2000,
                         "width", 300.0,
                         "height", 300.0,
                         "x", 50.0,
                         "y", 50.0,
                         NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
