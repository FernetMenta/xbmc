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
 * \brief The videoSurface holds a decoded frame.
 * The codec manipulates this.
 */
#define FF_XVBA_STATE_DECODED 4

/* @} */

struct xvba_bitstream_buffers
{
  const void *buffer;
  unsigned int size;
};

struct xvba_render_state {

  int state; ///< Holds FF_XVBA_STATE_* values.
  void *surface;
  XVBAPictureDescriptor *picture_descriptor;
  XVBAQuantMatrixAvc *iq_matrix;
  unsigned int num_slices;
  struct xvba_bitstream_buffers *buffers;
  uint32_t buffers_alllocated;
  int psf;
};

#endif /* AVCODEC_XVBA_H */
