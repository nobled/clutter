/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2009 Intel Corporation.
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

#ifndef __COGL_TEXTURE_2D_H
#define __COGL_TEXTURE_2D_H

#include "cogl-handle.h"
#include "cogl-pipeline-private.h"
#include "cogl-texture-private.h"

#define COGL_TEXTURE_2D(tex) ((CoglTexture2D *) tex)

typedef struct _CoglTexture2D CoglTexture2D;

struct _CoglTexture2D
{
  CoglTexture     _parent;

  /* The internal format of the GL texture represented as a
     CoglPixelFormat */
  CoglPixelFormat format;
  /* The internal format of the GL texture represented as a GL enum */
  GLenum          gl_format;
  /* The texture object number */
  GLuint          gl_texture;
  int             width;
  int             height;
  GLenum          min_filter;
  GLenum          mag_filter;
  GLint           wrap_mode_s;
  GLint           wrap_mode_t;
  gboolean        auto_mipmap;
  gboolean        mipmaps_dirty;
  gboolean        is_foreign;

  CoglTexturePixel first_pixel;
};

GQuark
_cogl_handle_texture_2d_get_type (void);

gboolean
_cogl_is_texture_2d (CoglHandle object);

CoglHandle
_cogl_texture_2d_new_with_size (unsigned int     width,
                                unsigned int     height,
                                CoglTextureFlags flags,
                                CoglPixelFormat  internal_format);

CoglHandle
_cogl_texture_2d_new_from_bitmap (CoglBitmap      *bmp,
                                  CoglTextureFlags flags,
                                  CoglPixelFormat  internal_format);

CoglHandle
_cogl_texture_2d_new_from_foreign (GLuint gl_handle,
                                   GLuint width,
                                   GLuint height,
                                   CoglPixelFormat format);

/*
 * _cogl_texture_2d_externally_modified:
 * @handle: A handle to a 2D texture
 *
 * This should be called whenever the texture is modified other than
 * by using cogl_texture_set_region. It will cause the mipmaps to be
 * invalidated
 */
void
_cogl_texture_2d_externally_modified (CoglHandle handle);

/*
 * _cogl_texture_2d_copy_from_framebuffer:
 * @handle: A handle to a 2D texture
 * @dst_x: X-position to store the image within the texture
 * @dst_y: Y-position to store the image within the texture
 * @src_x: X-position to within the framebuffer to read from
 * @src_y: Y-position to within the framebuffer to read from
 * @width: width of the rectangle to copy
 * @height: height of the rectangle to copy
 *
 * This copies a portion of the current read framebuffer into the
 * texture.
 */
void
_cogl_texture_2d_copy_from_framebuffer (CoglHandle handle,
                                        int dst_x,
                                        int dst_y,
                                        int src_x,
                                        int src_y,
                                        int width,
                                        int height);

#endif /* __COGL_TEXTURE_2D_H */
