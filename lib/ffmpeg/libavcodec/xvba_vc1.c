/*
 * VC-1 HW decode acceleration through XVBA
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

#include "xvba.h"
#include "xvba_internal.h"
#include "vc1.h"
#include "vc1data.h"
#include <assert.h>


/** @file
 * Implement structures of ffmpeg <-> XvBA
 */

/* Initialize and start decoding a frame with XvBA */
static int start_frame(AVCodecContext *avctx,
				av_unused const uint8_t *buffer, 
				av_unused uint32_t size)
{
    VC1Context * const v = avctx->priv_data;
    MpegEncContext * const s = &v->s;
    struct xvba_render_state *render;

    render = (struct xvba_render_state *)s->current_picture_ptr->f.data[0];
    assert(render);
    
    render->num_slices = 0;
    return 0;
}

/* End a hardware decoding based frame */
static int end_frame(AVCodecContext *avctx)
{
    VC1Context* const v = avctx->priv_data;
    MpegEncContext* const s = &v->s;
    struct xvba_render_state *render, *last, *next;
    XVBAPictureDescriptor *pic_descriptor;

    render = (struct xvba_render_state *)s->current_picture_ptr->f.data[0];
    assert(render);

    if (render->picture_descriptor == 0)
      return -1;

    pic_descriptor = render->picture_descriptor;
    
    av_dlog(avctx, "xvba_vc1_end_frame()\n");
    
    memset(pic_descriptor, 0, sizeof(*pic_descriptor));

    /* Fill in Parameters - for reference see AMD sdk documentation */
    pic_descriptor->profile                                 = ff_xvba_translate_profile(v->profile);
    pic_descriptor->level                                   = v->level;
    //done like in va-driver and vaapi
    if (v->profile == PROFILE_ADVANCED) {
      pic_descriptor->width_in_mb                           = s->avctx->coded_width;
      pic_descriptor->height_in_mb                          = s->avctx->coded_height;
    } else {
      pic_descriptor->width_in_mb                           = s->mb_width;
      pic_descriptor->height_in_mb                          = s->mb_height;
    }
    pic_descriptor->picture_structure                       = s->picture_structure;
    // xvba-video set this to 1 only 4:2:0 supported
    // doc says: if not set, choose 1 - we try this
    pic_descriptor->chroma_format                           = 1;
    pic_descriptor->avc_intra_flag                          = s->pict_type == AV_PICTURE_TYPE_I || v->bi_type == 1;
    pic_descriptor->avc_reference                           = (s->current_picture_ptr->f.reference & 3) ? 1 : 0;
    
    // VC-1 explicit parameters see page 30 of sdk
    // sps_info
    pic_descriptor->sps_info.vc1.postprocflag               = v->postprocflag;
    
    // done as in vaapi
    pic_descriptor->sps_info.vc1.pulldown                   = v->broadcast;
    pic_descriptor->sps_info.vc1.interlace                  = v->interlace;
    pic_descriptor->sps_info.vc1.tfcntrflag                 = v->tfcntrflag;
    pic_descriptor->sps_info.vc1.finterpflag                = v->finterpflag;
    pic_descriptor->sps_info.vc1.reserved                   = 1;
    // eventually check if this makes sense together with interlace
    pic_descriptor->sps_info.vc1.psf                        = v->psf;
    // what about if it is a frame (page 31)
    // looked at xvba-driver
    pic_descriptor->sps_info.vc1.second_field               = v->interlace && v->second_field && (v->fcm == ILACE_FIELD);
    pic_descriptor->sps_info.vc1.xvba_vc1_sps_reserved      = 0;
    
    // VC-1 explicit parameters see page 30 of sdk
    // pps_info
    pic_descriptor->pps_info.vc1.panscan_flag               = v->panscanflag;
    pic_descriptor->pps_info.vc1.refdist_flag               = v->refdist_flag;
    pic_descriptor->pps_info.vc1.loopfilter                 = s->loop_filter;
    pic_descriptor->pps_info.vc1.fastuvmc                   = v->fastuvmc;
    pic_descriptor->pps_info.vc1.extended_mv                = v->extended_mv;
    pic_descriptor->pps_info.vc1.dquant                     = v->dquant;
    pic_descriptor->pps_info.vc1.vstransform                = v->vstransform;
    pic_descriptor->pps_info.vc1.overlap                    = v->overlap;
    pic_descriptor->pps_info.vc1.quantizer                  = v->quantizer_mode;
    pic_descriptor->pps_info.vc1.extended_dmv               = v->extended_dmv;   
    pic_descriptor->pps_info.vc1.maxbframes                 = s->avctx->max_b_frames;
    pic_descriptor->pps_info.vc1.rangered                   = (pic_descriptor->profile == PROFILE_SIMPLE) ? 0 : v->rangered;   
    pic_descriptor->pps_info.vc1.syncmarker                 = (pic_descriptor->profile == PROFILE_SIMPLE) ? 0 : s->resync_marker;
    pic_descriptor->pps_info.vc1.multires                   = v->multires;
    pic_descriptor->pps_info.vc1.reserved                   = 1;
    pic_descriptor->pps_info.vc1.range_mapy_flag            = v->range_mapy_flag;
    pic_descriptor->pps_info.vc1.range_mapy                 = v->range_mapy;
    pic_descriptor->pps_info.vc1.range_mapuv_flag           = v->range_mapuv_flag;
    pic_descriptor->pps_info.vc1.range_mapuv                = v->range_mapuv;
    pic_descriptor->pps_info.vc1.xvba_vc1_pps_reserved      = 0;
    
    pic_descriptor->past_surface                            = 0;
    pic_descriptor->future_surface                          = 0;
    switch (s->pict_type) {
    case AV_PICTURE_TYPE_B:
        next = (struct xvba_render_state *)s->next_picture.f.data[0];
        assert(next);
        if (next)
          pic_descriptor->past_surface = next->surface;
        // fall-through
    case AV_PICTURE_TYPE_P:
        last = (struct xvba_render_state *)s->last_picture.f.data[0];
        assert(last);
        if (last)
          pic_descriptor->future_surface = last->surface;
        break;
    }

    ff_mpeg_draw_horiz_band(s, 0, s->avctx->height);

    return 0;
}

static int decode_slice(AVCodecContext *avctx, const uint8_t *buffer, uint32_t size)
{
  VC1Context* const v = avctx->priv_data;
  MpegEncContext* const s = &v->s;
  struct xvba_render_state *render;

  render = (struct xvba_render_state *)s->current_picture_ptr->f.data[0];
  assert(render);

  if (avctx->codec_id == CODEC_ID_VC1 &&
      size >= 4 && IS_MARKER(AV_RB32(buffer))) {
      buffer += 4;
      size   -= 4;
  }

  ff_xvba_add_slice_data(render, buffer, size);

  return 0;
}

#if CONFIG_WMV3_XVBA_HWACCEL
AVHWAccel ff_wmv3_xvba_hwaccel = {
    .name           = "wmv3_xvba",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_WMV3,
    .pix_fmt        = AV_PIX_FMT_XVBA_VLD,
    .start_frame    = start_frame,
    .end_frame      = end_frame,
    .decode_slice   = decode_slice,
};
#endif

AVHWAccel ff_vc1_xvba_hwaccel = {
    .name           = "vc1_xvba",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_VC1,
    .pix_fmt        = AV_PIX_FMT_XVBA_VLD,
    .start_frame    = start_frame,
    .end_frame      = end_frame,
    .decode_slice   = decode_slice,
};
