/*
 * MPEG-2 HW decode acceleration through XVBA
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

#include "dsputil.h"

static int start_frame(AVCodecContext *avctx, av_unused const uint8_t *buffer, av_unused uint32_t size)
{
    struct MpegEncContext * const s = avctx->priv_data;
    return 0;
}

static int end_frame(AVCodecContext *avctx)
{
    return 0;
}

static int decode_slice(AVCodecContext *avctx, const uint8_t *buffer, uint32_t size)
{
    struct MpegEncContext * const s = avctx->priv_data;
    return 0;
}

AVHWAccel ff_mpeg2_xvba_hwaccel = {
    .name           = "mpeg2_xvba",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_MPEG2VIDEO,
    .pix_fmt        = PIX_FMT_XVBA_VLD,
    .capabilities   = 0,
    .start_frame    = start_frame,
    .end_frame      = end_frame,
    .decode_slice   = decode_slice,
    .priv_data_size = 0,
};
