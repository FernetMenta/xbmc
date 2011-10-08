/*
 * H.264 HW decode acceleration through XVBA
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
#include "h264.h"
#include <assert.h>

/** @file
 *  This file implements the glue code between FFmpeg's and XvBA API's
 *  structures for H.264 decoding.
 */


/** Initialize and start decoding a frame with XVBA. */
static int start_frame(AVCodecContext          *avctx,
                       av_unused const uint8_t *buffer,
                       av_unused uint32_t       size)
{
  H264Context * const h = avctx->priv_data;
  struct xvba_render_state *render;
  XVBAPictureDescriptor *pic_descriptor;
  int i;

  render = (struct xvba_render_state *)h->cur_pic_ptr->f.data[0];
  assert(render);

  if (render->picture_descriptor == 0)
    return -1;

  pic_descriptor = render->picture_descriptor;

  for (i = 0; i < 2; ++i) {
      int foc = h->cur_pic_ptr->field_poc[i];
      if (foc == INT_MAX)
          foc = 0;
      pic_descriptor->avc_curr_field_order_cnt_list[i] = foc;
  }

  pic_descriptor->avc_frame_num = h->frame_num;

  render->num_slices = 0;

  return 0;
}

/** End a hardware decoding based frame. */
static int end_frame(AVCodecContext *avctx)
{
  H264Context * const h = avctx->priv_data;
  struct xvba_render_state *render;
  XVBAPictureDescriptor *pic_descriptor;
  XVBAQuantMatrixAvc *iq_matrix;

  render = (struct xvba_render_state *)h->cur_pic_ptr->f.data[0];
  assert(render);

  if (render->picture_descriptor == 0 || render->iq_matrix == 0)
    return -1;

  pic_descriptor = render->picture_descriptor;
  iq_matrix = render->iq_matrix;

  av_dlog(avctx, "end_frame()\n");

  /* Fill in Picture Parameters*/
  pic_descriptor->profile                                     = ff_xvba_translate_profile(avctx->profile);
  pic_descriptor->level                                       = avctx->level;
  pic_descriptor->width_in_mb                                 = h->mb_width;
  pic_descriptor->height_in_mb                                = h->mb_height;

  pic_descriptor->picture_structure                           = ff_xvba_translate_picture_structure(h->picture_structure);

  pic_descriptor->chroma_format                               = h->chroma_format_idc ? h->chroma_format_idc : 1;
  pic_descriptor->avc_intra_flag                              = (h->slice_type == AV_PICTURE_TYPE_I) ? 1 : 0;
  pic_descriptor->avc_reference                               = (h->cur_pic_ptr->f.reference & 3) ? 1 : 0;

  pic_descriptor->avc_bit_depth_luma_minus8                   = h->sps.bit_depth_luma - 8;
  pic_descriptor->avc_bit_depth_chroma_minus8                 = h->sps.bit_depth_chroma - 8;
  pic_descriptor->avc_log2_max_frame_num_minus4               = h->sps.log2_max_frame_num -4;
  pic_descriptor->avc_pic_order_cnt_type                      = h->sps.poc_type;
  pic_descriptor->avc_log2_max_pic_order_cnt_lsb_minus4       = h->sps.log2_max_poc_lsb - 4;
  pic_descriptor->avc_num_ref_frames                          = h->sps.ref_frame_count;
  pic_descriptor->avc_reserved_8bit                           = 0;

  /* Set a level that can decode stuff in every case without a lookup table 
     xvba seems to have problems only when the number of Reframes goes beyond
     the max support number of Level4.1@High. So in praxis decoding a Level 3.0
     file that in deed has level4.1@High specs does not matter. We use this fact
     and check if the ref_frames stay in the range Level4.1@high can decode if
     not, we set Level5.1 */
  if (pic_descriptor->avc_num_ref_frames > 4) {
    const unsigned int mbw = pic_descriptor->width_in_mb;
    const unsigned int mbh = pic_descriptor->height_in_mb;
    // this matches Level4.1@High stats to differ between <= 4.1 and 5.1
    const unsigned int max_ref_frames = 12288 * 1024 / (mbw * mbh * 384);
    const unsigned int num_ref_frames = pic_descriptor->avc_num_ref_frames;
    if (max_ref_frames < num_ref_frames)
        pic_descriptor->level = 51;
  }

  pic_descriptor->avc_num_slice_groups_minus1                 = h->pps.slice_group_count - 1;
  pic_descriptor->avc_num_ref_idx_l0_active_minus1            = h->pps.ref_count[0] - 1;
  pic_descriptor->avc_num_ref_idx_l1_active_minus1            = h->pps.ref_count[1] - 1;

  pic_descriptor->avc_pic_init_qp_minus26                     = h->pps.init_qp - 26;
  pic_descriptor->avc_pic_init_qs_minus26                     = h->pps.init_qs - 26;
  pic_descriptor->avc_chroma_qp_index_offset                  = h->pps.chroma_qp_index_offset[0];
  pic_descriptor->avc_second_chroma_qp_index_offset           = h->pps.chroma_qp_index_offset[1];
  pic_descriptor->avc_slice_group_change_rate_minus1          = 0; // not implemented in ffmpeg
  pic_descriptor->avc_reserved_16bit                          = 0; // must be 0
  memset(pic_descriptor->avc_field_order_cnt_list,0,sizeof(pic_descriptor->avc_field_order_cnt_list)); // must be 0
  memset(pic_descriptor->avc_slice_group_map,0,sizeof(pic_descriptor->avc_slice_group_map)); // must be 0

  // sps
  pic_descriptor->sps_info.avc.delta_pic_always_zero_flag     = h->sps.delta_pic_order_always_zero_flag;
  pic_descriptor->sps_info.avc.direct_8x8_inference_flag      = h->sps.direct_8x8_inference_flag;
  pic_descriptor->sps_info.avc.frame_mbs_only_flag            = h->sps.frame_mbs_only_flag;
  pic_descriptor->sps_info.avc.gaps_in_frame_num_value_allowed_flag = h->sps.gaps_in_frame_num_allowed_flag;
  pic_descriptor->sps_info.avc.mb_adaptive_frame_field_flag   = h->sps.mb_aff;
  pic_descriptor->sps_info.avc.residual_colour_transform_flag = h->sps.residual_color_transform_flag;
  pic_descriptor->sps_info.avc.xvba_avc_sps_reserved          = 0;

  // pps
  pic_descriptor->pps_info.avc.entropy_coding_mode_flag       = h->pps.cabac;
  pic_descriptor->pps_info.avc.pic_order_present_flag         = h->pps.pic_order_present;
  pic_descriptor->pps_info.avc.weighted_pred_flag             = h->pps.weighted_pred;
  pic_descriptor->pps_info.avc.weighted_bipred_idc            = h->pps.weighted_bipred_idc;
  pic_descriptor->pps_info.avc.deblocking_filter_control_present_flag = h->pps.deblocking_filter_parameters_present;
  pic_descriptor->pps_info.avc.constrained_intra_pred_flag    = h->pps.constrained_intra_pred;
  pic_descriptor->pps_info.avc.redundant_pic_cnt_present_flag = h->pps.redundant_pic_cnt_present;
  pic_descriptor->pps_info.avc.transform_8x8_mode_flag        = h->pps.transform_8x8_mode;
  pic_descriptor->pps_info.avc.xvba_avc_pps_reserved          = 0; // must be 0

  memcpy(iq_matrix->bScalingLists4x4, h->pps.scaling_matrix4, sizeof(iq_matrix->bScalingLists4x4));
  memcpy(iq_matrix->bScalingLists8x8[0], h->pps.scaling_matrix8[0], sizeof(iq_matrix->bScalingLists8x8[0]));
  memcpy(iq_matrix->bScalingLists8x8[1], h->pps.scaling_matrix8[3], sizeof(iq_matrix->bScalingLists8x8[0]));

  // Wait for an I-frame before start decoding. Workaround for ATI UVD and UVD+ GPUs
  if (!h->got_first_iframe) {
      if (h->slice_type != AV_PICTURE_TYPE_I && h->slice_type != AV_PICTURE_TYPE_SI)
          return -1;
      h->got_first_iframe = 1;
  }

  ff_h264_draw_horiz_band(h, 0, h->avctx->height);

  return 0;
}

/** Decode the given H.264 slice with XVBA. */
static int decode_slice(AVCodecContext *avctx,
                        const uint8_t  *buffer,
                        uint32_t        size)
{
  H264Context * const h = avctx->priv_data;
  struct xvba_render_state *render;

  render = (struct xvba_render_state *)h->cur_pic_ptr->f.data[0];
  assert(render);

  ff_xvba_add_slice_data(render, buffer, size);

  return 0;
}

AVHWAccel ff_h264_xvba_hwaccel = {
    .name           = "h264_xvba",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_H264,
    .pix_fmt        = AV_PIX_FMT_XVBA_VLD,
    .start_frame    = start_frame,
    .end_frame      = end_frame,
    .decode_slice   = decode_slice,
};
