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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __COGL_CONTEXT_DRIVER_H
#define __COGL_CONTEXT_DRIVER_H

#include "cogl.h"

typedef struct _CoglContextDriver
{
  /* Relying on glext.h to define these */
  COGL_PFNGLGENRENDERBUFFERSEXTPROC                pf_glGenRenderbuffersEXT;
  COGL_PFNGLDELETERENDERBUFFERSEXTPROC             pf_glDeleteRenderbuffersEXT;
  COGL_PFNGLBINDRENDERBUFFEREXTPROC                pf_glBindRenderbufferEXT;
  COGL_PFNGLRENDERBUFFERSTORAGEEXTPROC             pf_glRenderbufferStorageEXT;
  COGL_PFNGLGENFRAMEBUFFERSEXTPROC                 pf_glGenFramebuffersEXT;
  COGL_PFNGLBINDFRAMEBUFFEREXTPROC                 pf_glBindFramebufferEXT;
  COGL_PFNGLFRAMEBUFFERTEXTURE2DEXTPROC            pf_glFramebufferTexture2DEXT;
  COGL_PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC         pf_glFramebufferRenderbufferEXT;
  COGL_PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC          pf_glCheckFramebufferStatusEXT;
  COGL_PFNGLDELETEFRAMEBUFFERSEXTPROC              pf_glDeleteFramebuffersEXT;
  COGL_PFNGLBLITFRAMEBUFFEREXTPROC                 pf_glBlitFramebufferEXT;
  COGL_PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC  pf_glRenderbufferStorageMultisampleEXT;
  COGL_PFNGLGENERATEMIPMAPEXTPROC                  pf_glGenerateMipmapEXT;

  COGL_PFNGLCREATEPROGRAMOBJECTARBPROC             pf_glCreateProgramObjectARB;
  COGL_PFNGLCREATESHADEROBJECTARBPROC              pf_glCreateShaderObjectARB;
  COGL_PFNGLSHADERSOURCEARBPROC                    pf_glShaderSourceARB;
  COGL_PFNGLCOMPILESHADERARBPROC                   pf_glCompileShaderARB;
  COGL_PFNGLATTACHOBJECTARBPROC                    pf_glAttachObjectARB;
  COGL_PFNGLLINKPROGRAMARBPROC                     pf_glLinkProgramARB;
  COGL_PFNGLUSEPROGRAMOBJECTARBPROC                pf_glUseProgramObjectARB;
  COGL_PFNGLGETUNIFORMLOCATIONARBPROC              pf_glGetUniformLocationARB;
  COGL_PFNGLDELETEOBJECTARBPROC                    pf_glDeleteObjectARB;
  COGL_PFNGLGETINFOLOGARBPROC                      pf_glGetInfoLogARB;
  COGL_PFNGLGETOBJECTPARAMETERIVARBPROC            pf_glGetObjectParameterivARB;

  COGL_PFNGLVERTEXATTRIBPOINTERARBPROC		   pf_glVertexAttribPointerARB;
  COGL_PFNGLENABLEVERTEXATTRIBARRAYARBPROC	   pf_glEnableVertexAttribArrayARB;
  COGL_PFNGLDISABLEVERTEXATTRIBARRAYARBPROC	   pf_glDisableVertexAttribArrayARB;

  COGL_PFNGLGENBUFFERSARBPROC			   pf_glGenBuffersARB;
  COGL_PFNGLBINDBUFFERARBPROC			   pf_glBindBufferARB;
  COGL_PFNGLBUFFERDATAARBPROC			   pf_glBufferDataARB;
  COGL_PFNGLBUFFERSUBDATAARBPROC		   pf_glBufferSubDataARB;
  COGL_PFNGLMAPBUFFERARBPROC			   pf_glMapBufferARB;
  COGL_PFNGLUNMAPBUFFERARBPROC			   pf_glUnmapBufferARB;
  COGL_PFNGLDELETEBUFFERSARBPROC		   pf_glDeleteBuffersARB;

  COGL_PFNGLUNIFORM1FARBPROC                       pf_glUniform1fARB;
  COGL_PFNGLUNIFORM2FARBPROC                       pf_glUniform2fARB;
  COGL_PFNGLUNIFORM3FARBPROC                       pf_glUniform3fARB;
  COGL_PFNGLUNIFORM4FARBPROC                       pf_glUniform4fARB;
  COGL_PFNGLUNIFORM1FVARBPROC                      pf_glUniform1fvARB;
  COGL_PFNGLUNIFORM2FVARBPROC                      pf_glUniform2fvARB;
  COGL_PFNGLUNIFORM3FVARBPROC                      pf_glUniform3fvARB;
  COGL_PFNGLUNIFORM4FVARBPROC                      pf_glUniform4fvARB;
  COGL_PFNGLUNIFORM1IARBPROC                       pf_glUniform1iARB;
  COGL_PFNGLUNIFORM2IARBPROC                       pf_glUniform2iARB;
  COGL_PFNGLUNIFORM3IARBPROC                       pf_glUniform3iARB;
  COGL_PFNGLUNIFORM4IARBPROC                       pf_glUniform4iARB;
  COGL_PFNGLUNIFORM1IVARBPROC                      pf_glUniform1ivARB;
  COGL_PFNGLUNIFORM2IVARBPROC                      pf_glUniform2ivARB;
  COGL_PFNGLUNIFORM3IVARBPROC                      pf_glUniform3ivARB;
  COGL_PFNGLUNIFORM4IVARBPROC                      pf_glUniform4ivARB;
  COGL_PFNGLUNIFORMMATRIX2FVARBPROC                pf_glUniformMatrix2fvARB;
  COGL_PFNGLUNIFORMMATRIX3FVARBPROC                pf_glUniformMatrix3fvARB;
  COGL_PFNGLUNIFORMMATRIX4FVARBPROC                pf_glUniformMatrix4fvARB;

  COGL_PFNGLDRAWRANGEELEMENTSPROC                  pf_glDrawRangeElements;

  COGL_PFNGLACTIVETEXTUREPROC                      pf_glActiveTexture;
  COGL_PFNGLCLIENTACTIVETEXTUREPROC                pf_glClientActiveTexture;

  COGL_PFNGLBLENDEQUATIONPROC                      pf_glBlendEquation;
  COGL_PFNGLBLENDCOLORPROC                         pf_glBlendColor;
  COGL_PFNGLBLENDFUNCSEPARATEPROC                  pf_glBlendFuncSeparate;
  COGL_PFNGLBLENDEQUATIONSEPARATEPROC              pf_glBlendEquationSeparate;

} CoglContextDriver;

#endif /* __COGL_CONTEXT_DRIVER_H */
