/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDDemuxClient.h"
#include "DVDDemuxUtils.h"
#include "utils/log.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "settings/Settings.h"
#include "../DVDClock.h"

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - FF_INPUT_BUFFER_PADDING_SIZE)

using namespace PVR;

CDemuxStreamClientInternal::CDemuxStreamClientInternal(CDVDDemuxClient *parent)
 : m_parent(parent)
 , m_parser(NULL)
 , m_context(NULL)
 , m_parser_split(false)
{
}

CDemuxStreamClientInternal::~CDemuxStreamClientInternal()
{
  DisposeParser();
}

void CDemuxStreamClientInternal::DisposeParser()
{
  if (m_parser)
  {
    av_parser_close(m_parser);
    m_parser = NULL;
  }
  if (m_context)
  {
    avcodec_close(m_context);
    m_context = NULL;
  }
}

std::string CDemuxStreamVideoClient::GetStreamInfo()
{
  std::string strInfo;
  switch (codec)
  {
    case AV_CODEC_ID_MPEG2VIDEO:
      strInfo = "mpeg2video";
      break;
    case AV_CODEC_ID_H264:
      strInfo = "h264";
      break;
    default:
      break;
  }

  return strInfo;
}

std::string CDemuxStreamAudioClient::GetStreamInfo()
{
  std::string strInfo;
  switch (codec)
  {
    case AV_CODEC_ID_AC3:
      strInfo = "ac3";
      break;
    case AV_CODEC_ID_EAC3:
      strInfo = "eac3";
      break;
    case AV_CODEC_ID_MP2:
      strInfo = "mpeg2audio";
      break;
    case AV_CODEC_ID_AAC:
      strInfo = "aac";
      break;
    case AV_CODEC_ID_DTS:
      strInfo = "dts";
      break;
    default:
      break;
  }

  return strInfo;
}

std::string CDemuxStreamSubtitleClient::GetStreamInfo()
{
  return "";
}

CDVDDemuxClient::CDVDDemuxClient() : CDVDDemux()
{
  m_pInput = NULL;
  for (int i = 0; i < MAX_STREAMS; i++) m_streams[i] = NULL;
}

CDVDDemuxClient::~CDVDDemuxClient()
{
  Dispose();
}

bool CDVDDemuxClient::Open(CDVDInputStream* pInput)
{
  Abort();

  m_pInput = pInput;
  if (!g_PVRClients->GetPlayingClient(m_pvrClient))
    return false;

  return true;
}

void CDVDDemuxClient::Dispose()
{
  for (int i = 0; i < MAX_STREAMS; i++)
  {
    delete m_streams[i];
    m_streams[i] = NULL;
  }

  m_pInput = NULL;
}

void CDVDDemuxClient::DisposeStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS)
    return;
  delete m_streams[iStreamId];
  m_streams[iStreamId] = NULL;
}

void CDVDDemuxClient::Reset()
{
  if(m_pInput && g_PVRManager.IsStarted())
    m_pvrClient->DemuxReset();

  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxClient::Abort()
{
  if(m_pInput)
    m_pvrClient->DemuxAbort();
}

void CDVDDemuxClient::Flush()
{
  if(m_pInput && g_PVRManager.IsStarted())
    m_pvrClient->DemuxFlush();
}

void CDVDDemuxClient::ParsePacket(DemuxPacket* pkt)
{
  CDemuxStream* st = m_streams[pkt->iStreamId];
  if (st == NULL)
    return;

  if (st->ExtraSize)
    return;

  CDemuxStreamClientInternal* pvr = dynamic_cast<CDemuxStreamClientInternal*>(st);

  if(pvr == NULL
  || pvr->m_parser == NULL)
    return;

  if(pvr->m_context == NULL)
  {
    AVCodec *codec = avcodec_find_decoder(st->codec);
    if (codec == NULL)
    {
      CLog::Log(LOGERROR, "%s - can't find decoder", __FUNCTION__);
      pvr->DisposeParser();
      return;
    }

    pvr->m_context = avcodec_alloc_context3(codec);
    if(pvr->m_context == NULL)
    {
      CLog::Log(LOGERROR, "%s - can't allocate context", __FUNCTION__);
      pvr->DisposeParser();
      return;
    }
    pvr->m_context->time_base.num = 1;
    pvr->m_context->time_base.den = DVD_TIME_BASE;
  }

  if(pvr->m_parser_split && pvr->m_parser->parser->split)
  {
    int len = pvr->m_parser->parser->split(pvr->m_context, pkt->pData, pkt->iSize);
    if (len > 0 && len < FF_MAX_EXTRADATA_SIZE)
    {
      if (st->ExtraData)
        delete[] (uint8_t*)st->ExtraData;
      st->changes++;
      st->disabled = false;
      st->ExtraSize = len;
      st->ExtraData = new uint8_t[len+FF_INPUT_BUFFER_PADDING_SIZE];
      memcpy(st->ExtraData, pkt->pData, len);
      memset((uint8_t*)st->ExtraData + len, 0 , FF_INPUT_BUFFER_PADDING_SIZE);
      pvr->m_parser_split = false;
    }
  }


  uint8_t *outbuf = NULL;
  int      outbuf_size = 0;
  int len = av_parser_parse2(pvr->m_parser
                                        , pvr->m_context, &outbuf, &outbuf_size
                                        , pkt->pData, pkt->iSize
                                        , (int64_t)(pkt->pts * DVD_TIME_BASE)
                                        , (int64_t)(pkt->dts * DVD_TIME_BASE)
                                        , 0);
  /* our parse is setup to parse complete frames, so we don't care about outbufs */
  if(len >= 0)
  {
#define CHECK_UPDATE(st, trg, src, invalid) do { \
      if(src != invalid \
      && src != st->trg) { \
        CLog::Log(LOGDEBUG, "%s - {%d} " #trg " changed from %d to %d",  __FUNCTION__, st->iId, st->trg, src); \
        st->trg = src; \
        st->changes++; \
        st->disabled = false; \
      } \
    } while(0)


    CHECK_UPDATE(st, profile, pvr->m_context->profile , FF_PROFILE_UNKNOWN);
    CHECK_UPDATE(st, level  , pvr->m_context->level   , FF_LEVEL_UNKNOWN);

    switch (st->type)
    {
      case STREAM_AUDIO: {
        CDemuxStreamAudioClient* sta = static_cast<CDemuxStreamAudioClient*>(st);
        CHECK_UPDATE(sta, iChannels     , pvr->m_context->channels   , 0);
        CHECK_UPDATE(sta, iSampleRate   , pvr->m_context->sample_rate, 0);
        break;
      }
      case STREAM_VIDEO: {
        CDemuxStreamVideoClient* stv = static_cast<CDemuxStreamVideoClient*>(st);
        CHECK_UPDATE(stv, iWidth        , pvr->m_context->width , 0);
        CHECK_UPDATE(stv, iHeight       , pvr->m_context->height, 0);
        break;
      }

      default:
        break;
    }

#undef CHECK_UPDATE
  }
  else
    CLog::Log(LOGDEBUG, "%s - parser returned error %d", __FUNCTION__, len);

  return;
}

DemuxPacket* CDVDDemuxClient::Read()
{
  if (!g_PVRManager.IsStarted())
    return CDVDDemuxUtils::AllocateDemuxPacket(0);

  DemuxPacket* pPacket = m_pvrClient->DemuxRead();
  if (!pPacket)
  {
    if (m_pInput)
      m_pInput->Close();
    return NULL;
  }

  if (pPacket->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    RequestStreams();
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    RequestStreams();
  }
  else if (pPacket->iStreamId >= 0
        && pPacket->iStreamId < MAX_STREAMS
        && m_streams[pPacket->iStreamId])
  {
    ParsePacket(pPacket);
  }

  return pPacket;
}

CDemuxStream* CDVDDemuxClient::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS) return NULL;
    return m_streams[iStreamId];
}

void CDVDDemuxClient::RequestStreams()
{
  if (!g_PVRManager.IsStarted())
    return;

  PVR_STREAM_PROPERTIES props = {};
  m_pvrClient->GetStreamProperties(&props);
  unsigned int i;

  for (i = 0; i < props.iStreamCount; ++i)
  {
    CDemuxStream *stm = m_streams[i];

    if (props.stream[i].iCodecType == XBMC_CODEC_TYPE_AUDIO)
    {
      CDemuxStreamAudioClient* st = NULL;
      if (stm)
      {
        st = dynamic_cast<CDemuxStreamAudioClient*>(stm);
        if (!st || (st->codec != (AVCodecID)props.stream[i].iCodecId))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamAudioClient(this);
        st->m_parser = av_parser_init(props.stream[i].iCodecId);
        if(st->m_parser)
          st->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      }
      st->iChannels       = props.stream[i].iChannels;
      st->iSampleRate     = props.stream[i].iSampleRate;
      st->iBlockAlign     = props.stream[i].iBlockAlign;
      st->iBitRate        = props.stream[i].iBitRate;
      st->iBitsPerSample  = props.stream[i].iBitsPerSample;
      m_streams[i] = st;
      st->m_parser_split = true;
      st->changes++;
    }
    else if (props.stream[i].iCodecType == XBMC_CODEC_TYPE_VIDEO)
    {
      CDemuxStreamVideoClient* st = NULL;
      if (stm)
      {
        st = dynamic_cast<CDemuxStreamVideoClient*>(stm);
        if (!st
            || (st->codec != (AVCodecID)props.stream[i].iCodecId)
            || (st->iWidth != props.stream[i].iWidth)
            || (st->iHeight != props.stream[i].iHeight))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamVideoClient(this);
        st->m_parser = av_parser_init(props.stream[i].iCodecId);
        if(st->m_parser)
          st->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      }
      st->iFpsScale       = props.stream[i].iFPSScale;
      st->iFpsRate        = props.stream[i].iFPSRate;
      st->iHeight         = props.stream[i].iHeight;
      st->iWidth          = props.stream[i].iWidth;
      st->fAspect         = props.stream[i].fAspect;
      st->stereo_mode     = "mono";
      m_streams[i] = st;
      st->m_parser_split = true;
    }
    else if (props.stream[i].iCodecId == AV_CODEC_ID_DVB_TELETEXT)
    {
      if (stm)
      {
        if (stm->codec != (AVCodecID)props.stream[i].iCodecId)
          DisposeStream(i);
      }
      if (!m_streams[i])
        m_streams[i] = new CDemuxStreamTeletext();
    }
    else if (props.stream[i].iCodecType == XBMC_CODEC_TYPE_RDS &&
             CSettings::GetInstance().GetBool("pvrplayback.enableradiords"))
    {
      CDemuxStreamRadioRDS* st = NULL;
      if (stm)
      {
        st = dynamic_cast<CDemuxStreamRadioRDS*>(stm);
        if (!st || (st->codec != (AVCodecID)props.stream[i].iCodecId))
          DisposeStream(i);
      }
      if (!m_streams[i])
        m_streams[i] = new CDemuxStreamRadioRDS();
    }
    else if (props.stream[i].iCodecType == XBMC_CODEC_TYPE_SUBTITLE)
    {
      CDemuxStreamSubtitleClient* st = NULL;
      if (stm)
      {
        st = dynamic_cast<CDemuxStreamSubtitleClient*>(stm);
        if (!st || (st->codec != (AVCodecID)props.stream[i].iCodecId))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamSubtitleClient(this);
      }
      if(props.stream[i].iIdentifier)
      {
        st->ExtraData = new uint8_t[4];
        st->ExtraSize = 4;
        st->ExtraData[0] = (props.stream[i].iIdentifier >> 8) & 0xff;
        st->ExtraData[1] = (props.stream[i].iIdentifier >> 0) & 0xff;
        st->ExtraData[2] = (props.stream[i].iIdentifier >> 24) & 0xff;
        st->ExtraData[3] = (props.stream[i].iIdentifier >> 16) & 0xff;
      }
      m_streams[i] = st;
    }
    else
    {
      if (m_streams[i])
        DisposeStream(i);
      m_streams[i] = new CDemuxStream();
    }

    m_streams[i]->codec       = (AVCodecID)props.stream[i].iCodecId;
    m_streams[i]->iId         = i;
    m_streams[i]->iPhysicalId = props.stream[i].iPhysicalId;
    m_streams[i]->language[0] = props.stream[i].strLanguage[0];
    m_streams[i]->language[1] = props.stream[i].strLanguage[1];
    m_streams[i]->language[2] = props.stream[i].strLanguage[2];
    m_streams[i]->language[3] = props.stream[i].strLanguage[3];
    m_streams[i]->realtime = true;

    CLog::Log(LOGDEBUG,"CDVDDemuxClient::RequestStreams(): added/updated stream %d:%d with codec_id %d",
        m_streams[i]->iId,
        m_streams[i]->iPhysicalId,
        m_streams[i]->codec);
  }
  // check if we need to dispose any streams no longer in props
  for (unsigned int j = i; j < MAX_STREAMS; j++)
  {
    if (m_streams[j])
    {
      CLog::Log(LOGDEBUG,"CDVDDemuxClient::RequestStreams(): disposed stream %d:%d with codec_id %d",
          m_streams[j]->iId,
          m_streams[j]->iPhysicalId,
          m_streams[j]->codec);
      DisposeStream(j);
    }
  }
}

int CDVDDemuxClient::GetNrOfStreams()
{
  int i = 0;
  while (i < MAX_STREAMS && m_streams[i])
    i++;
  return i;
}

std::string CDVDDemuxClient::GetFileName()
{
  if(m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

std::string CDVDDemuxClient::GetStreamCodecName(int iStreamId)
{
  CDemuxStream *stream = GetStream(iStreamId);
  std::string strName;
  if (stream)
  {
    if (stream->codec == AV_CODEC_ID_AC3)
      strName = "ac3";
    else if (stream->codec == AV_CODEC_ID_MP2)
      strName = "mp2";
    else if (stream->codec == AV_CODEC_ID_AAC)
      strName = "aac";
    else if (stream->codec == AV_CODEC_ID_DTS)
      strName = "dca";
    else if (stream->codec == AV_CODEC_ID_MPEG2VIDEO)
      strName = "mpeg2video";
    else if (stream->codec == AV_CODEC_ID_H264)
      strName = "h264";
    else if (stream->codec == AV_CODEC_ID_EAC3)
      strName = "eac3";
  }
  return strName;
}

bool CDVDDemuxClient::SeekTime(int timems, bool backwards, double *startpts)
{
  if (m_pInput)
  {
    CDVDInputStream::IDemux *str = dynamic_cast<CDVDInputStream::IDemux*>(m_pInput);
    if (str)
      return str->SeekTime(timems, backwards, startpts);
  }
  return false;
}

void CDVDDemuxClient::SetSpeed (int speed)
{
  if (m_pInput)
  {
    CDVDInputStream::IDemux *str = dynamic_cast<CDVDInputStream::IDemux*>(m_pInput);
    if (str)
      str->SetSpeed(speed);
  }
}
