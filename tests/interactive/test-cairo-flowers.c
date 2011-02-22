/*
 * Pretty cairo flower hack.
 */

#include <clutter/clutter.h>

#ifndef _MSC_VER
#include <unistd.h> 		/* for sleep(), used for screenshots */
#endif
#include <stdlib.h>
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#define PETAL_MIN 20
#define PETAL_VAR 40
#define N_FLOWERS 40 /* reduce if you have a small card */

typedef struct Flower
{
  ClutterActor *ctex;
  gint          x,y,rot,v,rv;
}
Flower;

ClutterActor*
make_flower_actor (void)
{
  /* No science here, just a hack from toying */
  gint i, j;

  double colors[] = {
    0.71, 0.81, 0.83,
    1.0,  0.78, 0.57,
    0.64, 0.30, 0.35,
    0.73, 0.40, 0.39,
    0.91, 0.56, 0.64,
    0.70, 0.47, 0.45,
    0.92, 0.75, 0.60,
    0.82, 0.86, 0.85,
    0.51, 0.56, 0.67,
    1.0, 0.79, 0.58,

  };

  gint size;
  gint petal_size;
  gint n_groups;    /* Num groups of petals 1-3 */
  gint n_petals;    /* num of petals 4 - 8  */
  gint pm1, pm2;

  gint idx, last_idx = -1;

  ClutterActor *ctex;
  cairo_t      *cr;

  petal_size = PETAL_MIN + rand() % PETAL_VAR;
  size = petal_size * 8;

  n_groups = rand() % 3 + 1;

  ctex = clutter_cairo_texture_new (size, size);

  cr = clutter_cairo_texture_create (CLUTTER_CAIRO_TEXTURE (ctex));

  cairo_set_tolerance (cr, 0.1);

  /* Clear */
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_translate(cr, size/2, size/2);

  for (i=0; i<n_groups; i++)
    {
      n_petals = rand() % 5 + 4;
      cairo_save (cr);

      cairo_rotate (cr, rand() % 6);

      do {
	idx = (rand() % (sizeof (colors) / sizeof (double) / 3)) * 3;
      } while (idx == last_idx);

      cairo_set_source_rgba (cr, colors[idx], colors[idx+1],
			     colors[idx+2], 0.5);

      last_idx = idx;

      /* some bezier randomness */
      pm1 = rand() % 20;
      pm2 = rand() % 4;

      for (j=1; j<n_petals+1; j++)
	{
	  cairo_save (cr);
	  cairo_rotate (cr, ((2*M_PI)/n_petals)*j);

	  /* Petals are made up beziers */
	  cairo_new_path (cr);
	  cairo_move_to (cr, 0, 0);
	  cairo_rel_curve_to (cr,
			      petal_size, petal_size,
			      (pm2+2)*petal_size, petal_size,
			      (2*petal_size) + pm1, 0);
	  cairo_rel_curve_to (cr,
			      0 + (pm2*petal_size), -petal_size,
			      -petal_size, -petal_size,
			      -((2*petal_size) + pm1), 0);
	  cairo_close_path (cr);
	  cairo_fill (cr);
	  cairo_restore (cr);
	}

      petal_size -= rand() % (size/8);

      cairo_restore (cr);
    }

  /* Finally draw flower center */
  do {
      idx = (rand() % (sizeof (colors) / sizeof (double) / 3)) * 3;
  } while (idx == last_idx);

  if (petal_size < 0)
    petal_size = rand() % 10;

  cairo_set_source_rgba (cr, colors[idx], colors[idx+1], colors[idx+2], 0.5);

  cairo_arc(cr, 0, 0, petal_size, 0, M_PI * 2);
  cairo_fill(cr);

  cairo_destroy(cr);

  return ctex;
}

static void
tick (ClutterTimeline *timeline,
      gint             msecs,
      gpointer         data)
{
  Flower **flowers = (Flower**)data;
  gint i = 0;

  for (i = 0; i < N_FLOWERS; i++)
    {
      ClutterActor *stage;

      flowers[i]->y   += flowers[i]->v;
      flowers[i]->rot += flowers[i]->rv;

      stage = clutter_stage_get_default ();
      if (flowers[i]->y > (gint) clutter_actor_get_height (stage))
        flowers[i]->y = -clutter_actor_get_height (flowers[i]->ctex);

      clutter_actor_set_position (flowers[i]->ctex,
				  flowers[i]->x, flowers[i]->y);

      clutter_actor_set_rotation (flowers[i]->ctex,
                                  CLUTTER_Z_AXIS,
                                  flowers[i]->rot,
                                  clutter_actor_get_width (flowers[i]->ctex)/2,
                                  clutter_actor_get_height (flowers[i]->ctex)/2,
                                  0);
    }
}

int
test_cairo_flowers_main (int argc, char **argv)
{
  int              i;
  ClutterActor    *stage;
  ClutterColor     stage_color = { 0x0, 0x0, 0x0, 0xff };
  ClutterTimeline *timeline;
  Flower          *flowers[N_FLOWERS];

  srand (time (NULL));

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  stage = clutter_stage_get_default ();

  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

  for (i=0; i< N_FLOWERS; i++)
    {
      flowers[i]       = g_new0(Flower, 1);
      flowers[i]->ctex = make_flower_actor();
      flowers[i]->x    = rand() % (int) clutter_actor_get_width (stage)
                       - (PETAL_MIN + PETAL_VAR) * 2;
      flowers[i]->y    = rand() % (int) clutter_actor_get_height (stage);
      flowers[i]->rv   = rand() % 5 + 1;
      flowers[i]->v    = rand() % 10 + 2;

      clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                   flowers[i]->ctex);
      clutter_actor_set_position (flowers[i]->ctex,
				  flowers[i]->x, flowers[i]->y);
    }

  /* Create a timeline to manage animation */
  timeline = clutter_timeline_new (6000);
  clutter_timeline_set_loop (timeline, TRUE);

  /* fire a callback for frame change */
  g_signal_connect (timeline, "new-frame", G_CALLBACK (tick), flowers);

  clutter_actor_show (stage);

  clutter_timeline_start (timeline);

  g_signal_connect (stage, "key-press-event",
		    G_CALLBACK (clutter_main_quit),
		    NULL);

  clutter_main();

  return EXIT_SUCCESS;
}

