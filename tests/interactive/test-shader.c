/*#define TEST_GROUP */

#include <clutter/clutter.h>

#include <errno.h>
#include <stdlib.h>
#include <glib.h>
#include <gmodule.h>

/* Dynamic branching appeared in "Shader Model 3.0" that low-end IGPs
 * don't support.
 */
#define GPU_SUPPORTS_DYNAMIC_BRANCHING 0

typedef struct
{
  gchar *name;
  gchar *source;
} ShaderSource;

/* a couple of boilerplate defines that are common amongst all the
 * sample shaders
 */

/* FRAGMENT_SHADER_BEGIN: generate boilerplate with a local vec4 color already
 * initialized, from a sampler2D in a variable tex.
 */
#define FRAGMENT_SHADER_VARS					\
  "uniform sampler2D tex;"					\
  "uniform float x_step, y_step;"				\

#define FRAGMENT_SHADER_BEGIN					\
  "void main (){"						\
  "  vec4 color = texture2D (tex, vec2(cogl_tex_coord_in[0]));"

/* FRAGMENT_SHADER_END: apply the changed color to the output buffer correctly
 * blended with the gl specified color (makes the opacity of actors work
 * correctly).
 */
#define FRAGMENT_SHADER_END                    \
      "  gl_FragColor = color;"    \
      "  gl_FragColor = gl_FragColor * cogl_color_in;" \
      "}"

static ShaderSource shaders[]=
  {
    {"brightness-contrast",
     FRAGMENT_SHADER_VARS
     "uniform float brightness, contrast;"
     FRAGMENT_SHADER_BEGIN
     " color.rgb /= color.a;"
     " color.rgb = (color.rgb - vec3(0.5, 0.5, 0.5)) * contrast + "
          "vec3 (brightness + 0.5, brightness + 0.5, brightness + 0.5);"
     " color.rgb *= color.a;"
     FRAGMENT_SHADER_END
    },

    {"box-blur",
     FRAGMENT_SHADER_VARS

#if GPU_SUPPORTS_DYNAMIC_BRANCHING
     "uniform float radius;"
     FRAGMENT_SHADER_BEGIN
     "float u, v;"
     "int count = 1;"
     "for (u=-radius;u<radius;u++)"
     "  for (v=-radius;v<radius;v++)"
     "    {"
     "      color += texture2D(tex, "
     "          vec2(cogl_tex_coord_in[0].s + u * 2.0 * x_step, "
     "               cogl_tex_coord_in[0].t + v * 2.0 * y_step));"
     "      count ++;"
     "    }"
     "color = color / float(count);"
     FRAGMENT_SHADER_END
#else
     "vec4 get_rgba_rel(sampler2D tex, float dx, float dy)"
     "{"
     "  return texture2D (tex, cogl_tex_coord_in[0].st "
     "                         + vec2(dx, dy) * 2.0);"
     "}"

     FRAGMENT_SHADER_BEGIN
     "  float count = 1.0;"
     "  color += get_rgba_rel (tex, -x_step, -y_step); count++;"
     "  color += get_rgba_rel (tex, -x_step,  0.0);    count++;"
     "  color += get_rgba_rel (tex, -x_step,  y_step); count++;"
     "  color += get_rgba_rel (tex,  0.0,    -y_step); count++;"
     "  color += get_rgba_rel (tex,  0.0,     0.0);    count++;"
     "  color += get_rgba_rel (tex,  0.0,     y_step); count++;"
     "  color += get_rgba_rel (tex,  x_step, -y_step); count++;"
     "  color += get_rgba_rel (tex,  x_step,  0.0);    count++;"
     "  color += get_rgba_rel (tex,  x_step,  y_step); count++;"
     "  color = color / count;"
     FRAGMENT_SHADER_END
#endif
    },

    {"invert",
     FRAGMENT_SHADER_VARS
     FRAGMENT_SHADER_BEGIN
     "  color.rgb /= color.a;"
     "  color.rgb = vec3(1.0, 1.0, 1.0) - color.rgb;\n"
     "  color.rgb *= color.a;"
     FRAGMENT_SHADER_END
    },

    {"brightness-contrast",
     FRAGMENT_SHADER_VARS
     "uniform float brightness;"
     "uniform float contrast;"
     FRAGMENT_SHADER_BEGIN
     "  color.rgb /= color.a;"
     "  color.r = (color.r - 0.5) * contrast + brightness + 0.5;"
     "  color.g = (color.g - 0.5) * contrast + brightness + 0.5;"
     "  color.b = (color.b - 0.5) * contrast + brightness + 0.5;"
     "  color.rgb *= color.a;"
     FRAGMENT_SHADER_END
    },

    {"gray",
     FRAGMENT_SHADER_VARS
     FRAGMENT_SHADER_BEGIN
     "  float avg = (color.r + color.g + color.b) / 3.0;"
     "  color.r = avg;"
     "  color.g = avg;"
     "  color.b = avg;"
     FRAGMENT_SHADER_END
    },

    {"combined-mirror",
     FRAGMENT_SHADER_VARS
     FRAGMENT_SHADER_BEGIN 
     "  vec4 colorB = texture2D (tex, vec2(cogl_tex_coord_in[0].ts));"
     "  float avg = (color.r + color.g + color.b) / 3.0;"
     "  color.r = avg;"
     "  color.g = avg;"
     "  color.b = avg;"
     "  color = (color + colorB)/2.0;"
     FRAGMENT_SHADER_END
    },

    {"edge-detect",
     FRAGMENT_SHADER_VARS
     "float get_avg_rel(sampler2D texB, float dx, float dy)"
     "{"
     "  vec4 colorB = texture2D (texB, cogl_tex_coord_in[0].st + vec2(dx, dy));"
     "  return (colorB.r + colorB.g + colorB.b) / 3.0;"
     "}"
     FRAGMENT_SHADER_BEGIN
     "  mat3 sobel_h = mat3( 1.0,  2.0,  1.0,"
     "                       0.0,  0.0,  0.0,"
     "                      -1.0, -2.0, -1.0);"
     "  mat3 sobel_v = mat3( 1.0,  0.0, -1.0,"
     "                       2.0,  0.0, -2.0,"
     "                       1.0,  0.0, -1.0);"
     "  mat3 map = mat3( get_avg_rel(tex, -x_step, -y_step),"
     "                   get_avg_rel(tex, -x_step, 0.0),"
     "                   get_avg_rel(tex, -x_step, y_step),"
     "                   get_avg_rel(tex, 0.0, -y_step),"
     "                   get_avg_rel(tex, 0.0, 0.0),"
     "                   get_avg_rel(tex, 0.0, y_step),"
     "                   get_avg_rel(tex, x_step, -y_step),"
     "                   get_avg_rel(tex, x_step, 0.0),"
     "                   get_avg_rel(tex, x_step, y_step) );"
     "  mat3 gh = sobel_h * map;"
     "  mat3 gv = map * sobel_v;"
     "  float avgh = (gh[0][0] + gh[0][1] + gh[0][2] +"
     "                gh[1][0] + gh[1][1] + gh[1][2] +"
     "                gh[2][0] + gh[2][1] + gh[2][2]) / 18.0 + 0.5;"
     "  float avgv = (gv[0][0] + gv[0][1] + gv[0][2] +"
     "                gv[1][0] + gv[1][1] + gv[1][2] +"
     "                gv[2][0] + gv[2][1] + gv[2][2]) / 18.0 + 0.5;"
     "  float avg = (avgh + avgv) / 2.0;"
     "  color.r = avg * color.r;"
     "  color.g = avg * color.g;"
     "  color.b = avg * color.b;"
     FRAGMENT_SHADER_END
    },
    /* Terminating NULL sentinel */
    {NULL, NULL}
};

static gint shader_no = 0;

static int
next_p2 (gint a)
{
  int rval = 1;

  while (rval < a)
    rval <<= 1;

  return rval;
}

static void
set_shader_num (ClutterActor *actor, gint new_no)
{
  int  tex_width;
  int  tex_height;

  if (new_no >= 0 && shaders[new_no].name)
    {
      ClutterShader *shader;
      GError *error;
      shader_no = new_no;
      
      g_print ("setting shaders[%i] named '%s'\n",
               shader_no,
               shaders[shader_no].name);

      shader = clutter_shader_new ();
      
      error = NULL;
      g_object_set (G_OBJECT (shader),
                    "fragment-source", shaders[shader_no].source,
                    NULL);

      /* try to bind the shader, provoking an error we catch if there is issues
       * with the shader sources we've provided. At a later stage it should be
       * possible to iterate through a set of alternate shader sources (glsl ->
       * asm -> cg?) and the one that succesfully compiles is used.
       */
      clutter_shader_compile (shader, &error);
      if (error)
        {
          g_print ("unable to set shaders[%i] named '%s': %s",
                   shader_no, shaders[shader_no].name,
                   error->message);
          g_error_free (error);
          clutter_actor_set_shader (actor, NULL);
        }
      else
        {
          clutter_actor_set_shader (actor, NULL);
          clutter_actor_set_shader (actor, shader);
          clutter_actor_set_shader_param_int (actor, "tex", 0);
          clutter_actor_set_shader_param_float (actor, "radius", 3.0);
          clutter_actor_set_shader_param_float (actor, "brightness", 0.4);
          clutter_actor_set_shader_param_float (actor, "contrast", -1.9);

	  if (CLUTTER_IS_TEXTURE (actor))
	    {
              /* XXX - this assumes *a lot* about how things are done
               * internally on *some* hardware and driver
               */
	      tex_width = clutter_actor_get_width (actor);
	      tex_width = next_p2 (tex_width);

	      tex_height = clutter_actor_get_height (actor);
	      tex_height = next_p2 (tex_height);

	      clutter_actor_set_shader_param_float (actor, "x_step",
					            1.0f / tex_width);
	      clutter_actor_set_shader_param_float (actor, "y_step",
					            1.0f / tex_height);
  	    }
        }

      g_object_unref (shader);
    }
}

static gboolean
button_release_cb (ClutterActor    *actor,
                   ClutterEvent    *event,
                   gpointer         data)
{
  gint new_no;

  if (event->button.button == 1)
    {
      new_no = shader_no - 1;
    }
  else
    {
      new_no = shader_no + 1;
    }

  set_shader_num (actor, new_no);

  return FALSE;
}

#ifdef COGL_HAS_GLES2
static gboolean
timeout_cb (gpointer data)
{
  int new_no = shader_no + 1;

  if (shaders[new_no].name == NULL)
    new_no = 0;

  set_shader_num (CLUTTER_ACTOR (data), new_no);

  return TRUE;
}
#endif /* COGL_HAS_GLES2 */

G_MODULE_EXPORT gint
test_shader_main (gint argc, gchar *argv[])
{
  ClutterActor  *actor;
  ClutterActor  *stage;
  ClutterColor   stage_color = { 0x61, 0x64, 0x8c, 0xff };
  ClutterShader *shader;
  GError        *error;
  gchar         *file;

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  stage = clutter_stage_new ();
  clutter_stage_set_title (CLUTTER_STAGE (stage), "Shaders");
  clutter_actor_set_size (stage, 512, 384);
  g_signal_connect (stage, "destroy", G_CALLBACK (clutter_main_quit), NULL);

  g_print ("applying shaders[%i] named '%s'\n",
           shader_no,
           shaders[shader_no].name);

  shader = clutter_shader_new ();

  error = NULL;
  clutter_shader_set_fragment_source (shader, shaders[shader_no].source, -1);
  clutter_shader_compile (shader, &error);
  if (error)
    {
      g_print ("unable to load shaders[%d] named '%s': %s\n",
               shader_no,
               shaders[shader_no].name,
               error->message);
      g_error_free (error);

      return EXIT_FAILURE;
    }

  clutter_stage_set_title (CLUTTER_STAGE (stage), "Shader Test");
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

  file = g_build_filename (TESTS_DATADIR, "redhand.png", NULL);

#ifndef TEST_GROUP
  actor = clutter_texture_new_from_file (file, &error);
  if (!actor)
    g_error("pixbuf load failed: %s", error ? error->message : "Unknown");
#else
  actor = clutter_group_new ();

  {
    ClutterActor *child1, *child2, *child3, *child4;
    ClutterColor  color = { 0xff, 0x22, 0x66, 0x99 };

    child1 = clutter_texture_new_from_file (file, &error);
    if (!child1)
      g_error("pixbuf load failed: %s", error ? error->message : "Unknown");

    child2 = clutter_texture_new_from_file (file, &error);
    if (!child2)
      g_error("pixbuf load failed: %s", error ? error->message : "Unknown");

    child3 = clutter_rectangle_new ();
    child4 = clutter_text_new_with_text ("Sans 20px", "Shady stuff");

    clutter_rectangle_set_color (CLUTTER_RECTANGLE (child3), &color);
    clutter_actor_set_size (child3, 50, 50);

    clutter_actor_set_position (child1, 0, 0);
    clutter_actor_set_position (child2, 50, 100);
    clutter_actor_set_position (child3, 30, -30);
    clutter_actor_set_position (child4, -50, 20);

    clutter_container_add (CLUTTER_CONTAINER (actor),
                           child1,
                           child2,
                           child3,
                           child4,
                           NULL);

    clutter_actor_show_all (actor);
  }
#endif /* !TEST_GROUP */

  g_free (file);

  clutter_actor_set_shader (actor, shader);
  clutter_actor_set_position (actor, 100, 100);

  g_object_unref (shader);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);

  clutter_actor_set_shader_param_int (actor, "tex", 0);
  clutter_actor_set_shader_param_float (actor, "brightness", 0.4);
  clutter_actor_set_shader_param_float (actor, "contrast", -1.9);

  clutter_actor_set_reactive (actor, TRUE);
  g_signal_connect (actor, "button-release-event",
                    G_CALLBACK (button_release_cb), NULL);

#ifdef COGL_HAS_GLES2
  /* On an embedded platform it is difficult to right click so we will
     cycle through the shaders automatically */
  g_timeout_add_seconds (3, timeout_cb, actor);
#endif

  /* Show everying ( and map window ) */
  clutter_actor_show_all (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
