/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2007,2008,2009 Intel Corporation.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cogl.h"
#include "cogl-debug.h"
#include "cogl-internal.h"
#include "cogl-context.h"
#include "cogl-handle.h"
#include "cogl-object-private.h"
#include "cogl-util.h"
#include "cogl-texture-private.h"
#include "cogl-framebuffer-private.h"
#include "cogl-clip-stack.h"
#include "cogl-journal-private.h"

#ifndef HAVE_COGL_GLES2

#define glGenRenderbuffers                ctx->drv.pf_glGenRenderbuffers
#define glDeleteRenderbuffers             ctx->drv.pf_glDeleteRenderbuffers
#define glBindRenderbuffer                ctx->drv.pf_glBindRenderbuffer
#define glRenderbufferStorage             ctx->drv.pf_glRenderbufferStorage
#define glGenFramebuffers                 ctx->drv.pf_glGenFramebuffers
#define glBindFramebuffer                 ctx->drv.pf_glBindFramebuffer
#define glFramebufferTexture2D            ctx->drv.pf_glFramebufferTexture2D
#define glFramebufferRenderbuffer         ctx->drv.pf_glFramebufferRenderbuffer
#define glCheckFramebufferStatus          ctx->drv.pf_glCheckFramebufferStatus
#define glDeleteFramebuffers              ctx->drv.pf_glDeleteFramebuffers
#define glGetFramebufferAttachmentParameteriv \
                                          ctx->drv.pf_glGetFramebufferAttachmentParameteriv

#endif

#define glBlitFramebuffer                 ctx->drv.pf_glBlitFramebuffer

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER		0x8D40
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER		0x8D41
#endif
#ifndef GL_STENCIL_ATTACHMENT
#define GL_STENCIL_ATTACHMENT	0x8D00
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0	0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_STENCIL_INDEX8
#define GL_STENCIL_INDEX8       0x8D48
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL        0x84F9
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT     0x8D00
#endif
#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16    0x81A5
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE      0x8212
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE    0x8213
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE     0x8214
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE    0x8215
#endif
#ifndef GL_FRAMEBUFFER_ATTCHMENT_DEPTH_SIZE
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE    0x8216
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE  0x8217
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER               0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#endif

typedef enum {
  _TRY_DEPTH_STENCIL = 1L<<0,
  _TRY_DEPTH         = 1L<<1,
  _TRY_STENCIL       = 1L<<2
} TryFBOFlags;

typedef struct _CoglFramebufferStackEntry
{
  CoglFramebuffer *draw_buffer;
  CoglFramebuffer *read_buffer;
} CoglFramebufferStackEntry;

static void _cogl_framebuffer_free (CoglFramebuffer *framebuffer);
static void _cogl_onscreen_free (CoglOnscreen *onscreen);
static void _cogl_offscreen_free (CoglOffscreen *offscreen);

COGL_OBJECT_INTERNAL_DEFINE (Onscreen, onscreen);
COGL_OBJECT_DEFINE (Offscreen, offscreen);
COGL_OBJECT_DEFINE_DEPRECATED_REF_COUNTING (offscreen);

/* XXX:
 * The CoglObject macros don't support any form of inheritance, so for
 * now we implement the CoglObject support for the CoglFramebuffer
 * abstract class manually.
 */

gboolean
_cogl_is_framebuffer (void *object)
{
  CoglHandleObject *obj = object;

  if (obj == NULL)
    return FALSE;

  return obj->klass->type == _cogl_handle_onscreen_get_type ()
    || obj->klass->type == _cogl_handle_offscreen_get_type ();
}

static void
_cogl_framebuffer_init (CoglFramebuffer *framebuffer,
                        CoglFramebufferType type,
                        CoglPixelFormat format,
                        int width,
                        int height)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  framebuffer->type             = type;
  framebuffer->width            = width;
  framebuffer->height           = height;
  framebuffer->format           = format;
  framebuffer->viewport_x       = 0;
  framebuffer->viewport_y       = 0;
  framebuffer->viewport_width   = width;
  framebuffer->viewport_height  = height;

  framebuffer->modelview_stack  = _cogl_matrix_stack_new ();
  framebuffer->projection_stack = _cogl_matrix_stack_new ();

  framebuffer->dirty_bitmasks   = TRUE;

  /* Initialise the clip stack */
  _cogl_clip_state_init (&framebuffer->clip_state);

  framebuffer->journal = _cogl_journal_new ();

  /* Ensure we know the framebuffer->clear_color* members can't be
   * referenced for our fast-path read-pixel optimization (see
   * _cogl_journal_try_read_pixel()) until some region of the
   * framebuffer is initialized.
   */
  framebuffer->clear_clip_dirty = TRUE;

  /* XXX: We have to maintain a central list of all framebuffers
   * because at times we need to be able to flush all known journals.
   *
   * Examples where we need to flush all journals are:
   * - because journal entries can reference OpenGL texture
   *   coordinates that may not survive texture-atlas reorganization
   *   so we need the ability to flush those entries.
   * - because although we generally advise against modifying
   *   pipelines after construction we have to handle that possibility
   *   and since pipelines may be referenced in journal entries we
   *   need to be able to flush them before allowing the pipelines to
   *   be changed.
   *
   * Note we don't maintain a list of journals and associate
   * framebuffers with journals by e.g. having a journal->framebuffer
   * reference since that would introduce a circular reference.
   *
   * Note: As a future change to try and remove the need to index all
   * journals it might be possible to defer resolving of OpenGL
   * texture coordinates for rectangle primitives until we come to
   * flush a journal. This would mean for instance that a single
   * rectangle entry in a journal could later be expanded into
   * multiple quad primitives to handle sliced textures but would mean
   * we don't have to worry about retaining references to OpenGL
   * texture coordinates that may later become invalid.
   */
  ctx->framebuffers = g_list_prepend (ctx->framebuffers, framebuffer);
}

void
_cogl_framebuffer_free (CoglFramebuffer *framebuffer)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  ctx->framebuffers = g_list_remove (ctx->framebuffers, framebuffer);

  _cogl_clip_state_destroy (&framebuffer->clip_state);

  cogl_object_unref (framebuffer->modelview_stack);
  framebuffer->modelview_stack = NULL;

  cogl_object_unref (framebuffer->projection_stack);
  framebuffer->projection_stack = NULL;

  cogl_object_unref (framebuffer->journal);
}

/* This version of cogl_clear can be used internally as an alternative
 * to avoid flushing the journal or the framebuffer state. This is
 * needed when doing operations that may be called whiling flushing
 * the journal */
void
_cogl_clear4f (unsigned long buffers,
               float red,
               float green,
               float blue,
               float alpha)
{
  GLbitfield gl_buffers = 0;

  if (buffers & COGL_BUFFER_BIT_COLOR)
    {
      GE( glClearColor (red, green, blue, alpha) );
      gl_buffers |= GL_COLOR_BUFFER_BIT;
    }

  if (buffers & COGL_BUFFER_BIT_DEPTH)
    gl_buffers |= GL_DEPTH_BUFFER_BIT;

  if (buffers & COGL_BUFFER_BIT_STENCIL)
    gl_buffers |= GL_STENCIL_BUFFER_BIT;

  if (!gl_buffers)
    {
      static gboolean shown = FALSE;

      if (!shown)
        {
	  g_warning ("You should specify at least one auxiliary buffer "
                     "when calling cogl_clear");
        }

      return;
    }

  GE (glClear (gl_buffers));
}

void
_cogl_framebuffer_dirty (CoglFramebuffer *framebuffer)
{
  framebuffer->clear_clip_dirty = TRUE;
}

void
_cogl_framebuffer_clear4f (CoglFramebuffer *framebuffer,
                           unsigned long buffers,
                           float red,
                           float green,
                           float blue,
                           float alpha)
{
  CoglClipStack *clip_stack = _cogl_framebuffer_get_clip_stack (framebuffer);
  int scissor_x0;
  int scissor_y0;
  int scissor_x1;
  int scissor_y1;

  _cogl_clip_stack_get_bounds (clip_stack,
                               &scissor_x0, &scissor_y0,
                               &scissor_x1, &scissor_y1);

  /* NB: the previous clear could have had an arbitrary clip.
   * NB: everything for the last frame might still be in the journal
   *     but we can't assume anything about how each entry was
   *     clipped.
   * NB: Clutter will scissor its pick renders which would mean all
   *     journal entries have a common ClipStack entry, but without
   *     a layering violation Cogl has to explicitly walk the journal
   *     entries to determine if this is the case.
   * NB: We have a software only read-pixel optimization in the
   *     journal that determines the color at a given framebuffer
   *     coordinate for simple scenes without rendering with the GPU.
   *     When Clutter is hitting this fast-path we can expect to
   *     receive calls to clear the framebuffer with an un-flushed
   *     journal.
   * NB: To fully support software based picking for Clutter we
   *     need to be able to reliably detect when the contents of a
   *     journal can be discarded and when we can skip the call to
   *     glClear because it matches the previous clear request.
   */

  /* Note: we don't check for the stencil buffer being cleared here
   * since there isn't any public cogl api to manipulate the stencil
   * buffer.
   *
   * Note: we check for an exact clip match here because
   * 1) a smaller clip could mean existing journal entries may
   *    need to contribute to regions outside the new clear-clip
   * 2) a larger clip would mean we need to issue a real
   *    glClear and we only care about cases avoiding a
   *    glClear.
   *
   * Note: Comparing without an epsilon is considered
   * appropriate here.
   */
  if (buffers & COGL_BUFFER_BIT_COLOR &&
      buffers & COGL_BUFFER_BIT_DEPTH &&
      !framebuffer->clear_clip_dirty &&
      framebuffer->clear_color_red == red &&
      framebuffer->clear_color_green == green &&
      framebuffer->clear_color_blue == blue &&
      framebuffer->clear_color_alpha == alpha &&
      scissor_x0 == framebuffer->clear_clip_x0 &&
      scissor_y0 == framebuffer->clear_clip_y0 &&
      scissor_x1 == framebuffer->clear_clip_x1 &&
      scissor_y1 == framebuffer->clear_clip_y1)
    {
      /* NB: We only have to consider the clip state of journal
       * entries if the current clear is clipped since otherwise we
       * know every pixel of the framebuffer is affected by the clear
       * and so all journal entries become redundant and can simply be
       * discarded.
       */
      if (clip_stack)
        {
          /*
           * Note: the function for checking the journal entries is
           * quite strict. It avoids detailed checking of all entry
           * clip_stacks by only checking the details of the first
           * entry and then it only verifies that the remaining
           * entries share the same clip_stack ancestry. This means
           * it's possible for some false negatives here but that will
           * just result in us falling back to a real clear.
           */
          if (_cogl_journal_all_entries_within_bounds (framebuffer->journal,
                                                       scissor_x0, scissor_y0,
                                                       scissor_x1, scissor_y1))
            {
              _cogl_journal_discard (framebuffer->journal);
              goto cleared;
            }
        }
      else
        {
          _cogl_journal_discard (framebuffer->journal);
          goto cleared;
        }
    }

  COGL_NOTE (DRAW, "Clear begin");

  _cogl_framebuffer_flush_journal (framebuffer);

  /* NB: _cogl_framebuffer_flush_state may disrupt various state (such
   * as the pipeline state) when flushing the clip stack, so should
   * always be done first when preparing to draw. */
  _cogl_framebuffer_flush_state (framebuffer, framebuffer, 0);

  _cogl_clear4f (buffers, red, green, blue, alpha);;

  /* This is a debugging variable used to visually display the quad
   * batches from the journal. It is reset here to increase the
   * chances of getting the same colours for each frame during an
   * animation */
  if (G_UNLIKELY (COGL_DEBUG_ENABLED (COGL_DEBUG_RECTANGLES)) &&
      buffers & COGL_BUFFER_BIT_COLOR)
    {
      _COGL_GET_CONTEXT (ctxt, NO_RETVAL);
      ctxt->journal_rectangles_color = 1;
    }

  COGL_NOTE (DRAW, "Clear end");

cleared:

  if (buffers & COGL_BUFFER_BIT_COLOR && buffers & COGL_BUFFER_BIT_DEPTH)
    {
      /* For our fast-path for reading back a single pixel of simple
       * scenes where the whole frame is in the journal we need to
       * track the cleared color of the framebuffer in case the point
       * read doesn't intersect any of the journal rectangles. */
      framebuffer->clear_clip_dirty = FALSE;
      framebuffer->clear_color_red = red;
      framebuffer->clear_color_green = green;
      framebuffer->clear_color_blue = blue;
      framebuffer->clear_color_alpha = alpha;

      /* NB: A clear may be scissored so we need to track the extents
       * that the clear is applicable too... */
      if (clip_stack)
        {
          _cogl_clip_stack_get_bounds (clip_stack,
                                       &framebuffer->clear_clip_x0,
                                       &framebuffer->clear_clip_y0,
                                       &framebuffer->clear_clip_x1,
                                       &framebuffer->clear_clip_y1);
        }
      else
        {
          /* FIXME: set degenerate clip */
        }
    }
  else
    _cogl_framebuffer_dirty (framebuffer);
}

/* XXX: We'll need to consider if this API is a good approach for the
 * planned, public, CoglFramebuffer API. A framebuffer may have
 * multiple color buffers associated with it and the user may want to
 * only clear a subset of those buffers. Flags aren't a great
 * mechanism for handling this, but I don't think it would be very
 * convenient if you had to explicitly enumerate the individual
 * ancillary buffers to clear them.
 *
 * My current expectation is that we'll keep this flag based API but
 * also add a way to enumerate the individual color buffers for
 * clearing individually.
 *
 * Note: the 'buffers' and 'color' arguments were switched around on
 * purpose compared to the original cogl_clear API since it was odd
 * that you would be expected to specify a color before even
 * necessarily choosing to clear the color buffer.
 */
void
_cogl_framebuffer_clear (CoglFramebuffer *framebuffer,
                         unsigned long buffers,
                         const CoglColor *color)
{
  _cogl_framebuffer_clear4f (framebuffer, buffers,
                             cogl_color_get_red_float (color),
                             cogl_color_get_green_float (color),
                             cogl_color_get_blue_float (color),
                             cogl_color_get_alpha_float (color));
}

void
_cogl_framebuffer_swap_notify (CoglFramebuffer *framebuffer)
{
}

int
_cogl_framebuffer_get_width (CoglFramebuffer *framebuffer)
{
  return framebuffer->width;
}

int
_cogl_framebuffer_get_height (CoglFramebuffer *framebuffer)
{
  return framebuffer->height;
}

CoglClipState *
_cogl_framebuffer_get_clip_state (CoglFramebuffer *framebuffer)
{
  return &framebuffer->clip_state;
}

CoglClipStack *
_cogl_framebuffer_get_clip_stack (CoglFramebuffer *framebuffer)
{
  CoglClipState *clip_state = _cogl_framebuffer_get_clip_state (framebuffer);

  return _cogl_clip_state_get_stack (clip_state);
}

void
_cogl_framebuffer_set_clip_stack (CoglFramebuffer *framebuffer,
                                  CoglClipStack *stack)
{
  CoglClipState *clip_state = _cogl_framebuffer_get_clip_state (framebuffer);

  _cogl_clip_state_set_stack (clip_state, stack);
}

void
_cogl_framebuffer_set_viewport (CoglFramebuffer *framebuffer,
                                int x,
                                int y,
                                int width,
                                int height)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  if (framebuffer->viewport_x == x &&
      framebuffer->viewport_y == y &&
      framebuffer->viewport_width == width &&
      framebuffer->viewport_height == height)
    return;

  _cogl_framebuffer_flush_journal (framebuffer);

  framebuffer->viewport_x = x;
  framebuffer->viewport_y = y;
  framebuffer->viewport_width = width;
  framebuffer->viewport_height = height;

  if (_cogl_get_draw_buffer () == framebuffer)
    ctx->dirty_gl_viewport = TRUE;
}

int
_cogl_framebuffer_get_viewport_x (CoglFramebuffer *framebuffer)
{
  return framebuffer->viewport_x;
}

int
_cogl_framebuffer_get_viewport_y (CoglFramebuffer *framebuffer)
{
  return framebuffer->viewport_y;
}

int
_cogl_framebuffer_get_viewport_width (CoglFramebuffer *framebuffer)
{
  return framebuffer->viewport_width;
}

int
_cogl_framebuffer_get_viewport_height (CoglFramebuffer *framebuffer)
{
  return framebuffer->viewport_height;
}

void
_cogl_framebuffer_get_viewport4fv (CoglFramebuffer *framebuffer, int *viewport)
{
  viewport[0] = framebuffer->viewport_x;
  viewport[1] = framebuffer->viewport_y;
  viewport[2] = framebuffer->viewport_width;
  viewport[3] = framebuffer->viewport_height;
}

CoglMatrixStack *
_cogl_framebuffer_get_modelview_stack (CoglFramebuffer *framebuffer)
{
  return framebuffer->modelview_stack;
}

CoglMatrixStack *
_cogl_framebuffer_get_projection_stack (CoglFramebuffer *framebuffer)
{
  return framebuffer->projection_stack;
}

void
_cogl_framebuffer_add_dependency (CoglFramebuffer *framebuffer,
                                  CoglFramebuffer *dependency)
{
  GList *l;

  for (l = framebuffer->deps; l; l = l->next)
    {
      CoglFramebuffer *existing_dep = l->data;
      if (existing_dep == dependency)
        return;
    }

  /* TODO: generalize the primed-array type structure we e.g. use for
   * cogl_object_set_user_data or for pipeline children as a way to
   * avoid quite a lot of mid-scene micro allocations here... */
  framebuffer->deps =
    g_list_prepend (framebuffer->deps, cogl_object_ref (dependency));
}

void
_cogl_framebuffer_remove_all_dependencies (CoglFramebuffer *framebuffer)
{
  GList *l;
  for (l = framebuffer->deps; l; l = l->next)
    cogl_object_unref (l->data);
  g_list_free (framebuffer->deps);
  framebuffer->deps = NULL;
}

void
_cogl_framebuffer_flush_journal (CoglFramebuffer *framebuffer)
{
  _cogl_journal_flush (framebuffer->journal, framebuffer);
}

void
_cogl_framebuffer_flush_dependency_journals (CoglFramebuffer *framebuffer)
{
  GList *l;
  for (l = framebuffer->deps; l; l = l->next)
    _cogl_framebuffer_flush_journal (l->data);
  _cogl_framebuffer_remove_all_dependencies (framebuffer);
}

static inline void
_cogl_framebuffer_init_bits (CoglFramebuffer *framebuffer)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  if (G_LIKELY (!framebuffer->dirty_bitmasks))
    return;

#ifdef HAVE_COGL_GL
  if (cogl_features_available (COGL_FEATURE_OFFSCREEN)
      && framebuffer->type == COGL_FRAMEBUFFER_TYPE_OFFSCREEN)
    {
      GLenum attachment, pname;

      attachment = GL_COLOR_ATTACHMENT0;

      pname = GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE;
      GE( glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER,
                                                 attachment,
                                                 pname,
                                                 &framebuffer->red_bits) );

      pname = GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE;
      GE( glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER,
                                                 attachment,
                                                 pname,
                                                 &framebuffer->green_bits) );

      pname = GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE;
      GE( glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER,
                                                 attachment,
                                                 pname,
                                                 &framebuffer->blue_bits) );

      pname = GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE;
      GE( glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER,
                                                 attachment,
                                                 pname,
                                                 &framebuffer->alpha_bits) );
    }
  else
#endif /* HAVE_COGL_GL */
    {
      GE( glGetIntegerv (GL_RED_BITS,   &framebuffer->red_bits)   );
      GE( glGetIntegerv (GL_GREEN_BITS, &framebuffer->green_bits) );
      GE( glGetIntegerv (GL_BLUE_BITS,  &framebuffer->blue_bits)  );
      GE( glGetIntegerv (GL_ALPHA_BITS, &framebuffer->alpha_bits) );
    }


  COGL_NOTE (OFFSCREEN,
             "RGBA Bits for framebuffer[%p, %s]: %d, %d, %d, %d",
             framebuffer,
             framebuffer->type == COGL_FRAMEBUFFER_TYPE_OFFSCREEN
               ? "offscreen"
               : "onscreen",
             framebuffer->red_bits,
             framebuffer->blue_bits,
             framebuffer->green_bits,
             framebuffer->alpha_bits);

  framebuffer->dirty_bitmasks = FALSE;
}

typedef struct
{
  CoglHandle texture;
  unsigned int level;
  unsigned int level_width;
  unsigned int level_height;
} CoglFramebufferTryFBOData;

static gboolean
try_creating_fbo (CoglOffscreen *offscreen,
                  TryFBOFlags flags,
                  CoglFramebufferTryFBOData *data)
{
  GLuint gl_depth_stencil_handle;
  GLuint gl_depth_handle;
  GLuint gl_stencil_handle;
  GLuint tex_gl_handle;
  GLenum tex_gl_target;
  GLuint fbo_gl_handle;
  GLenum status;

  _COGL_GET_CONTEXT (ctx, FALSE);

  if (!cogl_texture_get_gl_texture (data->texture,
                                    &tex_gl_handle, &tex_gl_target))
    return FALSE;

  if (tex_gl_target != GL_TEXTURE_2D
#ifdef HAVE_COGL_GL
      && tex_gl_target != GL_TEXTURE_RECTANGLE_ARB
#endif
      )
    return FALSE;

  /* We are about to generate and bind a new fbo, so we pretend to
   * change framebuffer state so that the old framebuffer will be
   * rebound again before drawing. */
  ctx->dirty_bound_framebuffer = 1;

  /* Generate framebuffer */
  glGenFramebuffers (1, &fbo_gl_handle);
  GE (glBindFramebuffer (GL_FRAMEBUFFER, fbo_gl_handle));
  offscreen->fbo_handle = fbo_gl_handle;

  GE (glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              tex_gl_target, tex_gl_handle, data->level));

  if (flags & _TRY_DEPTH_STENCIL)
    {
      /* Create a renderbuffer for depth and stenciling */
      GE (glGenRenderbuffers (1, &gl_depth_stencil_handle));
      GE (glBindRenderbuffer (GL_RENDERBUFFER, gl_depth_stencil_handle));
      GE (glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_STENCIL,
                                 data->level_width,
                                 data->level_height));
      GE (glBindRenderbuffer (GL_RENDERBUFFER, 0));
      GE (glFramebufferRenderbuffer (GL_FRAMEBUFFER,
                                     GL_STENCIL_ATTACHMENT,
                                     GL_RENDERBUFFER, gl_depth_stencil_handle));
      GE (glFramebufferRenderbuffer (GL_FRAMEBUFFER,
                                     GL_DEPTH_ATTACHMENT,
                                     GL_RENDERBUFFER, gl_depth_stencil_handle));
      offscreen->renderbuffers =
        g_slist_prepend (offscreen->renderbuffers,
                         GUINT_TO_POINTER (gl_depth_stencil_handle));
    }

  if (flags & _TRY_DEPTH)
    {
      GE (glGenRenderbuffers (1, &gl_depth_handle));
      GE (glBindRenderbuffer (GL_RENDERBUFFER, gl_depth_handle));
      /* For now we just ask for GL_DEPTH_COMPONENT16 since this is all that's
       * available under GLES */
      GE (glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                                 data->level_width,
                                 data->level_height));
      GE (glBindRenderbuffer (GL_RENDERBUFFER, 0));
      GE (glFramebufferRenderbuffer (GL_FRAMEBUFFER,
                                     GL_DEPTH_ATTACHMENT,
                                     GL_RENDERBUFFER, gl_depth_handle));
      offscreen->renderbuffers =
        g_slist_prepend (offscreen->renderbuffers,
                         GUINT_TO_POINTER (gl_depth_handle));
    }

  if (flags & _TRY_STENCIL)
    {
      GE (glGenRenderbuffers (1, &gl_stencil_handle));
      GE (glBindRenderbuffer (GL_RENDERBUFFER, gl_stencil_handle));
      GE (glRenderbufferStorage (GL_RENDERBUFFER, GL_STENCIL_INDEX8,
                                 data->level_width,
                                 data->level_height));
      GE (glBindRenderbuffer (GL_RENDERBUFFER, 0));
      GE (glFramebufferRenderbuffer (GL_FRAMEBUFFER,
                                     GL_STENCIL_ATTACHMENT,
                                     GL_RENDERBUFFER, gl_stencil_handle));
      offscreen->renderbuffers =
        g_slist_prepend (offscreen->renderbuffers,
                         GUINT_TO_POINTER (gl_stencil_handle));
    }

  /* Make sure it's complete */
  status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

  if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      GSList *l;

      GE (glDeleteFramebuffers (1, &fbo_gl_handle));

      for (l = offscreen->renderbuffers; l; l = l->next)
        {
          GLuint renderbuffer = GPOINTER_TO_UINT (l->data);
          GE (glDeleteRenderbuffers (1, &renderbuffer));
        }

      g_slist_free (offscreen->renderbuffers);
      offscreen->renderbuffers = NULL;

      return FALSE;
    }

  return TRUE;
}

CoglHandle
_cogl_offscreen_new_to_texture_full (CoglHandle texhandle,
                                     CoglOffscreenFlags create_flags,
                                     unsigned int level)
{
  CoglOffscreen      *offscreen;
  static TryFBOFlags  flags;
  static gboolean     have_working_flags = FALSE;
  unsigned int        i;
  CoglFramebufferTryFBOData data;
  gboolean            fbo_created;

  _COGL_GET_CONTEXT (ctx, COGL_INVALID_HANDLE);

  if (!cogl_features_available (COGL_FEATURE_OFFSCREEN))
    return COGL_INVALID_HANDLE;

  /* Make texhandle is a valid texture object */
  if (!cogl_is_texture (texhandle))
    return COGL_INVALID_HANDLE;

  /* The texture must not be sliced */
  if (cogl_texture_is_sliced (texhandle))
    return COGL_INVALID_HANDLE;

  data.texture = texhandle;
  data.level = level;

  /* Calculate the size of the texture at this mipmap level to ensure
     that it's a valid level */
  data.level_width = cogl_texture_get_width (texhandle);
  data.level_height = cogl_texture_get_height (texhandle);

  for (i = 0; i < level; i++)
    {
      /* If neither dimension can be further divided then the level is
         invalid */
      if (data.level_width == 1 && data.level_height == 1)
        return COGL_INVALID_HANDLE;

      if (data.level_width > 1)
        data.level_width >>= 1;
      if (data.level_height > 1)
        data.level_height >>= 1;
    }

  /* XXX: The framebuffer_object spec isn't clear in defining whether attaching
   * a texture as a renderbuffer with mipmap filtering enabled while the
   * mipmaps have not been uploaded should result in an incomplete framebuffer
   * object. (different drivers make different decisions)
   *
   * To avoid an error with drivers that do consider this a problem we
   * explicitly set non mipmapped filters here. These will later be reset when
   * the texture is actually used for rendering according to the filters set on
   * the corresponding CoglPipeline.
   */
  _cogl_texture_set_filters (texhandle, GL_NEAREST, GL_NEAREST);

  offscreen = g_new0 (CoglOffscreen, 1);
  offscreen->texture = cogl_handle_ref (texhandle);

  if ((create_flags & COGL_OFFSCREEN_DISABLE_DEPTH_AND_STENCIL))
    fbo_created = try_creating_fbo (offscreen, 0, &data);
  else
    {
      if ((have_working_flags &&
           try_creating_fbo (offscreen, flags, &data)) ||
#ifdef HAVE_COGL_GL
          try_creating_fbo (offscreen, flags = _TRY_DEPTH_STENCIL, &data) ||
#endif
          try_creating_fbo (offscreen, flags = _TRY_DEPTH | _TRY_STENCIL,
                            &data) ||
          try_creating_fbo (offscreen, flags = _TRY_STENCIL, &data) ||
          try_creating_fbo (offscreen, flags = _TRY_DEPTH, &data) ||
          try_creating_fbo (offscreen, flags = 0, &data))
        {
          /* Record that the last set of flags succeeded so that we can
             try that set first next time */
          have_working_flags = TRUE;
          fbo_created = TRUE;
        }
      else
        fbo_created = FALSE;
    }

  if (fbo_created)
    {
      CoglOffscreen *ret;

      _cogl_framebuffer_init (COGL_FRAMEBUFFER (offscreen),
                              COGL_FRAMEBUFFER_TYPE_OFFSCREEN,
                              cogl_texture_get_format (texhandle),
                              data.level_width,
                              data.level_height);

      ret = _cogl_offscreen_object_new (offscreen);
      _cogl_texture_associate_framebuffer (texhandle, COGL_FRAMEBUFFER (ret));
      return ret;
    }
  else
    {
      g_free (offscreen);
      /* XXX: This API should probably have been defined to take a GError */
      g_warning ("%s: Failed to create an OpenGL framebuffer", G_STRLOC);
      return COGL_INVALID_HANDLE;
    }
}

CoglHandle
cogl_offscreen_new_to_texture (CoglHandle texhandle)
{
  return _cogl_offscreen_new_to_texture_full (texhandle, 0, 0);
}

static void
_cogl_offscreen_free (CoglOffscreen *offscreen)
{
  GSList *l;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  /* Chain up to parent */
  _cogl_framebuffer_free (COGL_FRAMEBUFFER (offscreen));

  for (l = offscreen->renderbuffers; l; l = l->next)
    {
      GLuint renderbuffer = GPOINTER_TO_UINT (l->data);
      GE (glDeleteRenderbuffers (1, &renderbuffer));
    }
  g_slist_free (offscreen->renderbuffers);

  GE (glDeleteFramebuffers (1, &offscreen->fbo_handle));

  if (offscreen->texture != COGL_INVALID_HANDLE)
    cogl_handle_unref (offscreen->texture);

  g_free (offscreen);
}

CoglHandle
_cogl_onscreen_new (void)
{
  CoglOnscreen *onscreen;

  /* XXX: Until we have full winsys support in Cogl then we can't fully
   * implement CoglOnscreen framebuffers, since we can't, e.g. keep track of
   * the window size. */

  /* FIXME: We are assuming onscreen buffers will always be
     premultiplied so we'll set the premult flag on the bitmap
     format. This will usually be correct because the result of the
     default blending operations for Cogl ends up with premultiplied
     data in the framebuffer. However it is possible for the
     framebuffer to be in whatever format depending on what
     CoglPipeline is used to render to it. Eventually we may want to
     add a way for an application to inform Cogl that the framebuffer
     is not premultiplied in case it is being used for some special
     purpose. */

  onscreen = g_new0 (CoglOnscreen, 1);
  _cogl_framebuffer_init (COGL_FRAMEBUFFER (onscreen),
                          COGL_FRAMEBUFFER_TYPE_ONSCREEN,
                          COGL_PIXEL_FORMAT_RGBA_8888_PRE,
                          0xdeadbeef, /* width */
                          0xdeadbeef); /* height */

  return _cogl_onscreen_object_new (onscreen);
}

static void
_cogl_onscreen_free (CoglOnscreen *onscreen)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  /* Chain up to parent */
  _cogl_framebuffer_free (COGL_FRAMEBUFFER (onscreen));

  g_free (onscreen);
}

void
_cogl_onscreen_clutter_backend_set_size (int width, int height)
{
  CoglFramebuffer *framebuffer;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  framebuffer = COGL_FRAMEBUFFER (ctx->window_buffer);

  if (framebuffer->width == width && framebuffer->height == height)
    return;

  framebuffer->width = width;
  framebuffer->height = height;

  /* We'll need to recalculate the GL viewport state derived
   * from the Cogl viewport */
  ctx->dirty_gl_viewport = 1;
}

static CoglFramebufferStackEntry *
create_stack_entry (CoglFramebuffer *draw_buffer,
                    CoglFramebuffer *read_buffer)
{
  CoglFramebufferStackEntry *entry = g_slice_new (CoglFramebufferStackEntry);

  entry->draw_buffer = draw_buffer;
  entry->read_buffer = read_buffer;

  return entry;
}

GSList *
_cogl_create_framebuffer_stack (void)
{
  CoglFramebufferStackEntry *entry;
  GSList *stack = NULL;

  entry = create_stack_entry (COGL_INVALID_HANDLE, COGL_INVALID_HANDLE);

  return g_slist_prepend (stack, entry);
}

void
_cogl_free_framebuffer_stack (GSList *stack)
{
  GSList *l;

  for (l = stack; l != NULL; l = l->next)
    {
      CoglFramebufferStackEntry *entry = l->data;

      if (entry->draw_buffer->type == COGL_FRAMEBUFFER_TYPE_OFFSCREEN)
        _cogl_offscreen_free (COGL_OFFSCREEN (entry->draw_buffer));
      else
        _cogl_onscreen_free (COGL_ONSCREEN (entry->draw_buffer));

      if (entry->read_buffer->type == COGL_FRAMEBUFFER_TYPE_OFFSCREEN)
        _cogl_offscreen_free (COGL_OFFSCREEN (entry->read_buffer));
      else
        _cogl_onscreen_free (COGL_ONSCREEN (entry->read_buffer));

      g_slice_free (CoglFramebufferStackEntry, entry);
    }
  g_slist_free (stack);
}

/* Set the current framebuffer without checking if it's already the
 * current framebuffer. This is used by cogl_pop_framebuffer while
 * the top of the stack is currently not up to date. */
static void
_cogl_set_framebuffers_real (CoglFramebuffer *draw_buffer,
                             CoglFramebuffer *read_buffer)
{
  CoglFramebufferStackEntry *entry;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  entry = ctx->framebuffer_stack->data;

  ctx->dirty_bound_framebuffer = 1;
  ctx->dirty_gl_viewport = 1;

  if (draw_buffer)
    cogl_object_ref (draw_buffer);
  if (entry->draw_buffer)
    cogl_object_unref (entry->draw_buffer);

  if (read_buffer)
    cogl_object_ref (read_buffer);
  if (entry->read_buffer)
    cogl_object_unref (entry->read_buffer);

  entry->draw_buffer = draw_buffer;
  entry->read_buffer = read_buffer;

  /* We've effectively just switched the current modelview and
   * projection matrix stacks and clip state so we need to dirty
   * them to ensure they get flushed for the next batch of geometry
   * we flush */
  _cogl_matrix_stack_dirty (draw_buffer->modelview_stack);
  _cogl_matrix_stack_dirty (draw_buffer->projection_stack);
  _cogl_clip_stack_dirty ();
}

static void
_cogl_set_framebuffers (CoglFramebuffer *draw_buffer,
                        CoglFramebuffer *read_buffer)
{
  CoglFramebuffer *current_draw_buffer;
  CoglFramebuffer *current_read_buffer;

  g_return_if_fail (_cogl_is_framebuffer (draw_buffer));
  g_return_if_fail (_cogl_is_framebuffer (read_buffer));

  current_draw_buffer = _cogl_get_draw_buffer ();
  current_read_buffer = _cogl_get_read_buffer ();

  if (current_draw_buffer != draw_buffer ||
      current_read_buffer != read_buffer)
    {
      /* XXX: eventually we want to remove this implicit journal flush
       * so we can log into the journal beyond framebuffer changes to
       * support batching scenes that depend on the results of
       * mid-scene renders to textures. Current will be NULL when the
       * framebuffer stack is first created so we need to guard
       * against that here */
      if (current_draw_buffer)
        _cogl_framebuffer_flush_journal (current_draw_buffer);
      if (current_read_buffer)
        _cogl_framebuffer_flush_journal (current_read_buffer);
      _cogl_set_framebuffers_real (draw_buffer, read_buffer);
    }
}

void
cogl_set_framebuffer (CoglFramebuffer *framebuffer)
{
  _cogl_set_framebuffers (framebuffer, framebuffer);
}

/* XXX: deprecated API */
void
cogl_set_draw_buffer (CoglBufferTarget target, CoglHandle handle)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  if (target == COGL_WINDOW_BUFFER)
    handle = ctx->window_buffer;

  /* This is deprecated public API. The public API doesn't currently
     really expose the concept of separate draw and read buffers so
     for the time being this actually just sets both buffers */
  cogl_set_framebuffer (handle);
}

CoglFramebuffer *
_cogl_get_draw_buffer (void)
{
  CoglFramebufferStackEntry *entry;

  _COGL_GET_CONTEXT (ctx, NULL);

  g_assert (ctx->framebuffer_stack);

  entry = ctx->framebuffer_stack->data;

  return entry->draw_buffer;
}

CoglFramebuffer *
_cogl_get_read_buffer (void)
{
  CoglFramebufferStackEntry *entry;

  _COGL_GET_CONTEXT (ctx, NULL);

  g_assert (ctx->framebuffer_stack);

  entry = ctx->framebuffer_stack->data;

  return entry->read_buffer;
}

void
_cogl_push_framebuffers (CoglFramebuffer *draw_buffer,
                         CoglFramebuffer *read_buffer)
{
  CoglFramebuffer *old_draw_buffer, *old_read_buffer;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  g_return_if_fail (_cogl_is_framebuffer (draw_buffer));
  g_return_if_fail (_cogl_is_framebuffer (read_buffer));
  g_assert (ctx->framebuffer_stack);

  /* Copy the top of the stack so that when we call cogl_set_framebuffer
     it will still know what the old framebuffer was */
  old_draw_buffer = cogl_object_ref (_cogl_get_draw_buffer ());
  old_read_buffer = cogl_object_ref (_cogl_get_read_buffer ());
  ctx->framebuffer_stack =
    g_slist_prepend (ctx->framebuffer_stack,
                     create_stack_entry (old_draw_buffer,
                                         old_read_buffer));

  _cogl_set_framebuffers (draw_buffer, read_buffer);
}

void
cogl_push_framebuffer (CoglFramebuffer *buffer)
{
  _cogl_push_framebuffers (buffer, buffer);
}

/* XXX: deprecated API */
void
cogl_push_draw_buffer (void)
{
  cogl_push_framebuffer (_cogl_get_draw_buffer ());
}

void
cogl_pop_framebuffer (void)
{
  CoglFramebufferStackEntry *to_pop;
  CoglFramebufferStackEntry *to_restore;
  gboolean changed = FALSE;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  g_assert (ctx->framebuffer_stack != NULL);
  g_assert (ctx->framebuffer_stack->next != NULL);

  to_pop = ctx->framebuffer_stack->data;
  to_restore = ctx->framebuffer_stack->next->data;

  if (to_pop->draw_buffer != to_restore->draw_buffer ||
      to_pop->read_buffer != to_restore->read_buffer)
    {
      /* XXX: eventually we want to remove this implicit journal flush
       * so we can log into the journal beyond framebuffer changes to
       * support batching scenes that depend on the results of
       * mid-scene renders to textures. */
      _cogl_framebuffer_flush_journal (to_pop->draw_buffer);
      _cogl_framebuffer_flush_journal (to_pop->read_buffer);

      changed = TRUE;
    }

  cogl_object_unref (to_pop->draw_buffer);
  cogl_object_unref (to_pop->read_buffer);
  g_slice_free (CoglFramebufferStackEntry, to_pop);

  ctx->framebuffer_stack =
    g_slist_delete_link (ctx->framebuffer_stack,
                         ctx->framebuffer_stack);

  /* If the framebuffer has changed as a result of popping the top
   * then re-assert the current buffer so as to dirty state as
   * necessary. */
  if (changed)
    _cogl_set_framebuffers_real (to_restore->draw_buffer,
                                 to_restore->read_buffer);
}

/* XXX: deprecated API */
void
cogl_pop_draw_buffer (void)
{
  cogl_pop_framebuffer ();
}

static void
bind_gl_framebuffer (GLenum target, CoglFramebuffer *framebuffer)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  if (framebuffer->type == COGL_FRAMEBUFFER_TYPE_OFFSCREEN)
    GE (glBindFramebuffer (target,
                           COGL_OFFSCREEN (framebuffer)->fbo_handle));
  else
    GE (glBindFramebuffer (target, 0));
}

void
_cogl_framebuffer_flush_state (CoglFramebuffer *draw_buffer,
                               CoglFramebuffer *read_buffer,
                               CoglFramebufferFlushFlags flags)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  if (cogl_features_available (COGL_FEATURE_OFFSCREEN) &&
      ctx->dirty_bound_framebuffer)
    {
      if (!cogl_features_available (COGL_FEATURE_OFFSCREEN_BLIT) ||
          draw_buffer == read_buffer)
        bind_gl_framebuffer (GL_FRAMEBUFFER, draw_buffer);
      else
        {
          bind_gl_framebuffer (GL_DRAW_FRAMEBUFFER, draw_buffer);
          bind_gl_framebuffer (GL_READ_FRAMEBUFFER, read_buffer);
        }

      ctx->dirty_bound_framebuffer = FALSE;
    }

  if (ctx->dirty_gl_viewport)
    {
      int gl_viewport_y;

      /* Convert the Cogl viewport y offset to an OpenGL viewport y offset
       * NB: OpenGL defines its window and viewport origins to be bottom
       * left, while Cogl defines them to be top left.
       * NB: We render upside down to offscreen framebuffers so we don't
       * need to convert the y offset in this case. */
      if (cogl_is_offscreen (draw_buffer))
        gl_viewport_y = draw_buffer->viewport_y;
      else
        gl_viewport_y = draw_buffer->height -
          (draw_buffer->viewport_y + draw_buffer->viewport_height);

      COGL_NOTE (OPENGL, "Calling glViewport(%d, %d, %d, %d)",
                 draw_buffer->viewport_x,
                 gl_viewport_y,
                 draw_buffer->viewport_width,
                 draw_buffer->viewport_height);

      GE (glViewport (draw_buffer->viewport_x,
                      gl_viewport_y,
                      draw_buffer->viewport_width,
                      draw_buffer->viewport_height));
      ctx->dirty_gl_viewport = FALSE;
    }

  /* since we might have changed the framebuffer, we should initialize
   * the bits; this is a no-op if they have already been initialized
   */
  _cogl_framebuffer_init_bits (draw_buffer);
  _cogl_framebuffer_init_bits (read_buffer);

  /* XXX: Flushing clip state may trash the modelview and projection
   * matrices so we must do it before flushing the matrices...
   */
  if (!(flags & COGL_FRAMEBUFFER_FLUSH_SKIP_CLIP_STATE))
    _cogl_clip_state_flush (&draw_buffer->clip_state);

  if (!(flags & COGL_FRAMEBUFFER_FLUSH_SKIP_MODELVIEW))
    _cogl_matrix_stack_flush_to_gl (draw_buffer->modelview_stack,
                                    COGL_MATRIX_MODELVIEW);

  _cogl_matrix_stack_flush_to_gl (draw_buffer->projection_stack,
                                  COGL_MATRIX_PROJECTION);
}

int
_cogl_framebuffer_get_red_bits (CoglFramebuffer *framebuffer)
{
  _cogl_framebuffer_init_bits (framebuffer);

  return framebuffer->red_bits;
}

int
_cogl_framebuffer_get_green_bits (CoglFramebuffer *framebuffer)
{
  _cogl_framebuffer_init_bits (framebuffer);

  return framebuffer->green_bits;
}

int
_cogl_framebuffer_get_blue_bits (CoglFramebuffer *framebuffer)
{
  _cogl_framebuffer_init_bits (framebuffer);

  return framebuffer->blue_bits;
}

int
_cogl_framebuffer_get_alpha_bits (CoglFramebuffer *framebuffer)
{
  _cogl_framebuffer_init_bits (framebuffer);

  return framebuffer->alpha_bits;
}

gboolean
_cogl_framebuffer_try_fast_read_pixel (CoglFramebuffer *framebuffer,
                                       int x,
                                       int y,
                                       CoglReadPixelsFlags source,
                                       CoglPixelFormat format,
                                       guint8 *pixel)
{
  gboolean found_intersection;

  if (G_UNLIKELY (COGL_DEBUG_ENABLED (COGL_DEBUG_DISABLE_FAST_READ_PIXEL)))
    return FALSE;

  if (source != COGL_READ_PIXELS_COLOR_BUFFER)
    return FALSE;

  if (format != COGL_PIXEL_FORMAT_RGBA_8888_PRE &&
      format != COGL_PIXEL_FORMAT_RGBA_8888)
    return FALSE;

  if (!_cogl_journal_try_read_pixel (framebuffer->journal,
                                     x, y, format, pixel,
                                     &found_intersection))
    return FALSE;

  /* If we can't determine the color from the primitives in the
   * journal then see if we can use the last recorded clear color
   */

  /* If _cogl_journal_try_read_pixel() failed even though there was an
   * intersection of the given point with a primitive in the journal
   * then we can't fallback to the framebuffer's last clear color...
   * */
  if (found_intersection)
    return TRUE;

  /* If the framebuffer has been rendered too since it was last
   * cleared then we can't return the last known clear color. */
  if (framebuffer->clear_clip_dirty)
    return FALSE;

  if (x >= framebuffer->clear_clip_x0 &&
      x < framebuffer->clear_clip_x1 &&
      y >= framebuffer->clear_clip_y0 &&
      y < framebuffer->clear_clip_y1)
    {

      /* we currently only care about cases where the premultiplied or
       * unpremultipled colors are equivalent... */
      if (framebuffer->clear_color_alpha != 1.0)
        return FALSE;

      pixel[0] = framebuffer->clear_color_red * 255.0;
      pixel[1] = framebuffer->clear_color_green * 255.0;
      pixel[2] = framebuffer->clear_color_blue * 255.0;
      pixel[3] = framebuffer->clear_color_alpha * 255.0;

      return TRUE;
    }

  return FALSE;
}

void
_cogl_blit_framebuffer (unsigned int src_x,
                        unsigned int src_y,
                        unsigned int dst_x,
                        unsigned int dst_y,
                        unsigned int width,
                        unsigned int height)
{
  CoglFramebuffer *draw_buffer;
  CoglFramebuffer *read_buffer;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  draw_buffer = _cogl_get_draw_buffer ();
  read_buffer = _cogl_get_read_buffer ();

  g_return_if_fail (cogl_features_available (COGL_FEATURE_OFFSCREEN_BLIT));
  /* We can only support blitting between offscreen buffers because
     otherwise we would need to mirror the image and GLES2.0 doesn't
     support this */
  g_return_if_fail (cogl_is_offscreen (draw_buffer));
  g_return_if_fail (cogl_is_offscreen (read_buffer));
  /* The buffers must be the same format */
  g_return_if_fail (draw_buffer->format == read_buffer->format);

  /* Make sure the current framebuffers are bound. We explicitly avoid
     flushing the clip state so we can bind our own empty state */
  _cogl_framebuffer_flush_state (_cogl_get_draw_buffer (),
                                 _cogl_get_read_buffer (),
                                 COGL_FRAMEBUFFER_FLUSH_SKIP_CLIP_STATE);

  /* Flush any empty clip stack because glBlitFramebuffer is affected
     by the scissor and we want to hide this feature for the Cogl API
     because it's not obvious to an app how the clip state will affect
     the scissor */
  _cogl_clip_stack_flush (NULL);

  glBlitFramebuffer (src_x, src_y,
                     src_x + width, src_y + height,
                     dst_x, dst_y,
                     dst_x + width, dst_y + height,
                     GL_COLOR_BUFFER_BIT,
                     GL_NEAREST);
}
