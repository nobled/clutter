/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Copyright (C) 2010  Intel Corporation.
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

 * Authors:
 *  Damien Lespiau <damien.lespiau@intel.com>
 */

#include <X11/extensions/XKBcommon.h>

#include "clutter-stage.h"
#include "clutter-event.h"
#include "clutter-input-device.h"

ClutterEvent *    _clutter_key_event_new_from_evdev (ClutterInputDevice *device,
                                                     ClutterStage       *stage,
                                                     struct xkb_desc    *xkb,
                                                     uint32_t            _time,
                                                     uint32_t            key,
                                                     uint32_t            state,
                                                     uint32_t           *modifier_state);
struct xkb_desc * _clutter_xkb_desc_new             (const gchar *model,
                                                     const gchar *layout,
                                                     const gchar *variant,
                                                     const gchar *options);
