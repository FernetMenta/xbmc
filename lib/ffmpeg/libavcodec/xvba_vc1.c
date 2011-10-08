/*
 * VC-1 HW decode acceleration through VA API
 *
 * Copyright (C) 2005-2011 team XBMA
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

#include "vc1.h"
#include "vc1data.h"

static int xvba_vc1_start_frame(AVCodecContext *avctx, av_unused const uint8_t *buffer, av_unused uint32_t size)
{
    VC1Context * const v = avctx->priv_data;
    MpegEncContext * const s = &v->s;
    return 0;
}

static int xvba_vc1_end_frame(AVCodecContext *avctx)
{
    VC1Context * const v = avctx->priv_data;

    return 0;
}

static int xvba_vc1_decode_slice(AVCodecContext *avctx, const uint8_t *buffer, uint32_t size)
{
    VC1Context * const v = avctx->priv_data;
    MpegEncContext * const s = &v->s;
    return 0;
}

#if CONFIG_WMV3_XVBA_HWACCEL
AVHWAccel ff_wmv3_xvba_hwaccel = {
    .name           = "wmv3_xvba",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_WMV3,
    .pix_fmt        = PIX_FMT_XVBA_VLD,
    .capabilities   = 0,
    .start_frame    = xvba_vc1_start_frame,
    .end_frame      = xvba_vc1_end_frame,
    .decode_slice   = xvba_vc1_decode_slice,
    .priv_data_size = 0,
};
#endif

AVHWAccel ff_vc1_xvba_hwaccel = {
    .name           = "vc1_xvba",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_VC1,
    .pix_fmt        = PIX_FMT_XVBA_VLD,
    .capabilities   = 0,
    .start_frame    = xvba_vc1_start_frame,
    .end_frame      = xvba_vc1_end_frame,
    .decode_slice   = xvba_vc1_decode_slice,
    .priv_data_size = 0,
};
