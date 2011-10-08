/*
 * MPEG-4 / H.263 HW decode acceleration through VA API
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

#include "h263.h"


static int xvba_mpeg4_start_frame(AVCodecContext *avctx, av_unused const uint8_t *buffer, av_unused uint32_t size)
{
    MpegEncContext * const s = avctx->priv_data;
    return 0;
}

static int xvba_mpeg4_end_frame(AVCodecContext *avctx)
{
    return 0;
}

static int xvba_mpeg4_decode_slice(AVCodecContext *avctx, const uint8_t *buffer, uint32_t size)
{
    MpegEncContext * const s = avctx->priv_data;

    return 0;
}

#if CONFIG_MPEG4_XVBA_HWACCEL
AVHWAccel ff_mpeg4_xvba_hwaccel = {
    .name           = "mpeg4_xvba",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_MPEG4,
    .pix_fmt        = PIX_FMT_XVBA_VLD,
    .capabilities   = 0,
    .start_frame    = xvba_mpeg4_start_frame,
    .end_frame      = xvba_mpeg4_end_frame,
    .decode_slice   = xvba_mpeg4_decode_slice,
    .priv_data_size = 0,
};
#endif

