#ifndef _HAVE_CLTR_H
#define _HAVE_CLTR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <GL/glx.h>
#include <GL/gl.h>

#include <glib.h>

#include <gst/gconf/gconf.h>

#include "pixbuf.h"
#include "fonts.h"


typedef enum CltrDirection
{
  CLTR_NORTH,
  CLTR_SOUTH,
  CLTR_EAST,
  CLTR_WEST
} 
CltrDirection;

typedef struct CltrRect
{
  int x, y, width, height;
}
CltrRect;

typedef struct CltrTexture CltrTexture;

#define cltr_rect_x1(r) ((r).x)
#define cltr_rect_y1(r) ((r).y)
#define cltr_rect_x2(r) ((r).x + (r).width)
#define cltr_rect_y2(r) ((r).y + (r).height)

/* texture stuff */

/* ******************* */

#include "cltr-core.h"
#include "cltr-glu.h"
#include "cltr-texture.h"
#include "cltr-events.h"
#include "cltr-widget.h"
#include "cltr-window.h"
#include "cltr-overlay.h"
#include "cltr-label.h"
#include "cltr-button.h"
#include "cltr-photo-grid.h"
#include "cltr-video.h"

#endif
