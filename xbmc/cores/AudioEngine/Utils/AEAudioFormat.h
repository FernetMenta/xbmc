#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AEChannelInfo.h"
#include "AEStreamInfo.h"

#define AE_IS_RAW(x) ((x) >= AE_FMT_RAW && (x) < AE_FMT_U8P)
#define AE_IS_PLANAR(x) ((x) >= AE_FMT_U8P && (x) <= AE_FMT_FLOATP)

/**
 * The audio format structure that fully defines a stream's audio information
 */
struct AEAudioFormat
{
  /**
   * The stream's data format (eg, AE_FMT_S16LE)
   */
  enum AEDataFormat m_dataFormat;

  /**
   * The stream's sample rate (eg, 48000)
   */
  unsigned int m_sampleRate;

  /**
   * The stream's channel layout
   */
  CAEChannelInfo m_channelLayout;

  /**
   * The number of frames per period
   */
  unsigned int m_frames;

  /**
   * The number of samples in one frame
   */
  unsigned int m_frameSamples;

  /**
   * The size of one frame in bytes
   */
  unsigned int m_frameSize;

  /**
   * Stream info of raw passthrough
   */
  CAEStreamInfo m_streamInfo;

  struct IECPack
  {
    enum AEDataFormat m_dataFormat;
    unsigned int m_sampleRate;
    CAEChannelInfo m_channelLayout;
  };
  IECPack m_iecPack;

  bool m_isIecPacked;
 
  AEAudioFormat()
  {
    m_dataFormat = AE_FMT_INVALID;
    m_sampleRate = 0;
    m_frames = 0;
    m_frameSamples = 0;
    m_frameSize = 0;
    m_isIecPacked = false;
  }

  bool operator==(const AEAudioFormat& fmt) const
  {
    return  m_dataFormat    ==  fmt.m_dataFormat    &&
            m_sampleRate    ==  fmt.m_sampleRate    &&
            m_channelLayout ==  fmt.m_channelLayout &&
            m_frames        ==  fmt.m_frames        &&
            m_frameSamples  ==  fmt.m_frameSamples  &&
            m_frameSize     ==  fmt.m_frameSize     &&
            m_streamInfo    ==  fmt.m_streamInfo;
  }
 
  AEAudioFormat& operator=(const AEAudioFormat& fmt)
  {
    m_dataFormat = fmt.m_dataFormat;
    m_sampleRate = fmt.m_sampleRate;
    m_channelLayout = fmt.m_channelLayout;
    m_frames = fmt.m_frames;
    m_frameSamples = fmt.m_frameSamples;
    m_frameSize = fmt.m_frameSize;
    m_streamInfo = fmt.m_streamInfo;

    return *this;
  }
};

