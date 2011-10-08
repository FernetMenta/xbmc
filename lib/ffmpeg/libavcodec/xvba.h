/*
 * Video Acceleration API (shared data between FFmpeg and the video player)
 * HW decode acceleration for MPEG-2, MPEG-4, H.264 and VC-1
 *
 * Copyright (C) 2008-2009 Splitted-Desktop Systems
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

#ifndef AVCODEC_XVBA_H
#define AVCODEC_XVBA_H

#include <stdint.h>
#include <X11/Xlib.h>
#include <amd/amdxvba.h>


/**
 * \defgroup XVBA_Decoding VA API Decoding
 * \ingroup Decoder
 * @{
 */

/** \brief The videoSurface is used for rendering. */
#define FF_XVBA_STATE_USED_FOR_RENDER 1

/**
 * \brief The videoSurface is needed for reference/prediction.
 * The codec manipulates this.
 */
#define FF_XVBA_STATE_USED_FOR_REFERENCE 2

/**
 * This structure is used to share data between the FFmpeg library and
 * the client video application.
 * This shall be zero-allocated and available as
 * AVCodecContext.hwaccel_context. All user members can be set once
 * during initialization or through each AVCodecContext.get_buffer()
 * function call. In any case, they must be valid prior to calling
 * decoding functions.
 */
struct xvba_context {
  XVBABufferDescriptor *picture_descriptor_buffer;
  XVBABufferDescriptor *iq_matrix_buffer;
  XVBABufferDescriptor *data_buffer;
  unsigned int         *data_control;
  unsigned int          data_control_size;
  unsigned int          num_slices;
};

/* @} */

struct xvba_render_state {

  int state; ///< Holds FF_XVBA_STATE_* values.
  void *surface;
};

#endif /* AVCODEC_XVBA_H */
