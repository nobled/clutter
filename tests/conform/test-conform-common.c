#include "config.h"

#include <clutter/clutter.h>
#include <stdlib.h>

#ifdef COGL_HAS_XLIB
#include <X11/Xlib.h>
#include <clutter/x11/clutter-x11.h>
#endif

#include "test-conform-common.h"

/**
 * test_conform_simple_fixture_setup:
 *
 * Initialise stuff before each test is run
 */
void
test_conform_simple_fixture_setup (TestConformSimpleFixture *fixture,
				   gconstpointer data)
{
  const TestConformSharedState *shared_state = data;
  static int counter = 0;

  if (counter != 0)
    g_critical ("We don't support running more than one test at a time\n"
                "in a single test run due to the state leakage that often\n"
                "causes subsequent tests to fail.\n"
                "\n"
                "If you want to run all the tests you should run\n"
                "$ make test-report");
  counter++;

#ifdef HAVE_CLUTTER_GLX
  {
    /* on X11 we need a display connection to run the test suite */
    const gchar *display = g_getenv ("DISPLAY");
    if (!display || *display == '\0')
      {
        g_print ("No DISPLAY found. Unable to run the conformance "
                 "test suite without a display.\n");

        exit (EXIT_SUCCESS);
      }
  }
#endif

  g_assert (clutter_init (shared_state->argc_addr, shared_state->argv_addr)
            == CLUTTER_INIT_SUCCESS);

#ifdef COGL_HAS_XLIB
  /* A lot of the tests depend on a specific stage / framebuffer size
   * when they read pixels back to verify the results of the test.
   *
   * Normally the asynchronous nature of X means that setting the
   * clutter stage size may really happen an indefinite amount of time
   * later but since the tests are so short lived and may only render
   * a single frame this is not an acceptable semantic.
   */
  XSynchronize (clutter_x11_get_default_display(), TRUE);
#endif
}


/**
 * test_conform_simple_fixture_teardown:
 *
 * Cleanup stuff after each test has finished
 */
void
test_conform_simple_fixture_teardown (TestConformSimpleFixture *fixture,
				      gconstpointer data)
{
  /* const TestConformSharedState *shared_state = data; */
}

