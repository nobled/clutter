/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2010 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#ifndef __COGL_TEXTURE_PIXMAP_X11_PRIVATE_H
#define __COGL_TEXTURE_PIXMAP_X11_PRIVATE_H

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xdamage.h>

#include <sys/shm.h>

#ifdef COGL_HAS_GLX_SUPPORT
#include <GL/glx.h>
#endif

#include "cogl-handle.h"
#include "cogl-texture-private.h"

#define COGL_TEXTURE_PIXMAP_X11(tex) ((CoglTexturePixmapX11 *) tex)

typedef struct _CoglDamageRectangle CoglDamageRectangle;

struct _CoglDamageRectangle
{
  unsigned int x1;
  unsigned int y1;
  unsigned int x2;
  unsigned int y2;
};

typedef struct _CoglTexturePixmapX11 CoglTexturePixmapX11;

struct _CoglTexturePixmapX11
{
  CoglTexture _parent;

  Pixmap pixmap;
  CoglHandle tex;

  unsigned int depth;
  Visual *visual;
  unsigned int width;
  unsigned int height;

  XImage *image;

  XShmSegmentInfo shm_info;

  Damage damage;
  CoglTexturePixmapX11ReportLevel damage_report_level;
  gboolean damage_owned;
  CoglDamageRectangle damage_rect;

#ifdef COGL_HAS_GLX_SUPPORT
  /* During the pre_paint method, this will be set to TRUE if we
     should use the GLX texture, otherwise we will use the regular
     texture */
  gboolean use_glx_texture;
  CoglHandle glx_tex;
  GLXPixmap glx_pixmap;
  gboolean glx_pixmap_has_mipmap;
  gboolean glx_can_mipmap;
  gboolean bind_tex_image_queued;
  gboolean pixmap_bound;
#endif
};

GQuark
_cogl_handle_texture_pixmap_x11_get_type (void);

#endif /* __COGL_TEXTURE_PIXMAP_X11_PRIVATE_H */
