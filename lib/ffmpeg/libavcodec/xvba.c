/*
 * HW decode acceleration for MPEG-2, H.264 and VC-1
 *
 * Copyright (C) 2005-2011 Team XBMC
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


/**
 * \addtogroup XVBA_Decoding
 *
 * @{
 */

#include <stdint.h>
#include "xvba.h"
#include "xvba_internal.h"
#include "avcodec.h"
#include "mpegvideo.h"

int ff_xvba_translate_profile(int profile) {

  if (profile == 66)
    return 1;
  else if (profile == 77)
    return 2;
  else if (profile == 100)
    return 3;
  else if (profile == 0)
    return 4;
  else if (profile == 1)
    return 5;
  else if (profile == 3)
    return 6;
  else
    return -1;
}

// See page 25 of the documentation
int ff_xvba_translate_picture_structure(int picture_structure) {
  if(picture_structure == PICT_BOTTOM_FIELD)
    return 0;
  else if (picture_structure == PICT_TOP_FIELD)
    return 1;
  else //PICT_FRAME by default
    return 3;
}

void ff_xvba_add_slice_data(struct xvba_render_state *render, const uint8_t *buffer, uint32_t size) {

  render->buffers = av_fast_realloc(
         render->buffers,
         &render->buffers_alllocated,
         sizeof(struct xvba_bitstream_buffers)*(render->num_slices + 1)
  );

  render->buffers[render->num_slices].buffer = buffer;
  render->buffers[render->num_slices].size = size;

  render->num_slices++;
}

