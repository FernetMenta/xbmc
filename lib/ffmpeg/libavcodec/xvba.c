/*
 * Video Acceleration API (video decoding)
 * HW decode acceleration for MPEG-2, MPEG-4, H.264 and VC-1
 *
 * Copyright (C) 2005-2011 Team XBMA
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
#include "avcodec.h"

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

void ff_xvba_add_slice_data(struct xvba_context *hwaccel_context, const uint8_t *buffer, uint32_t size, int append) {

  void *bitstream_buffer;
  bitstream_buffer = hwaccel_context->data_buffer->bufferXVBA;

  memcpy((uint8_t*)bitstream_buffer+hwaccel_context->data_buffer->data_size_in_buffer, buffer, size);
  hwaccel_context->data_buffer->data_size_in_buffer += size;

  if (append)
    hwaccel_context->data_control[hwaccel_context->num_slices] += hwaccel_context->data_buffer->data_size_in_buffer;
  else {
    hwaccel_context->data_control = av_fast_realloc(
       hwaccel_context->data_control,
       &hwaccel_context->data_control_size,
       sizeof(unsigned int)*(hwaccel_context->num_slices + 1)
    );
    hwaccel_context->data_control[hwaccel_context->num_slices] = hwaccel_context->data_buffer->data_size_in_buffer;
  }
}

