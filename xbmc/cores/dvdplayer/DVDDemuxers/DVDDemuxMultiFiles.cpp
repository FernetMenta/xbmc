/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "DVDDemuxMultiFiles.h"
#include "DVDFactoryDemuxer.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "utils/log.h"
#include <map>



CDVDDemuxMultiFiles::CDVDDemuxMultiFiles()
{
  m_nextDemuxerToRead = 0;
  m_streamsRead = 0;
}

CDVDDemuxMultiFiles::~CDVDDemuxMultiFiles()
{
  Dispose();
}

CDemuxStream* CDVDDemuxMultiFiles::GetStream(int iStreamId)
{
  if (iStreamId < m_StreamMap.size())
    return m_StreamMap[iStreamId];
  else
    return NULL;
  //std::map<unsigned int, std::pair<unsigned int, unsigned int>>::iterator iter = m_StreamMap.find(iStreamId);
  //if (iter == m_StreamMap.end())
  //  return NULL;

  //std::map<int, DemuxPtr>::iterator demuxIter = m_pDemuxers.find(iter->second.first);
  //if (demuxIter == m_pDemuxers.end())
  //  return NULL;

  //CDemuxStream* stream = demuxIter->second.get()->GetStream(iter->second.second);
  //if (stream)
  //  stream->iId = iStreamId;
  //return stream;
}

bool CDVDDemuxMultiFiles::UpdateStreamMap(int inputIndex, DemuxPtr demuxer)
{
  if (!demuxer.get())
    return false;

  for (int i = 0; i < demuxer.get()->GetNrOfStreams(); ++i)
  {
    CDemuxStream* stream = demuxer.get()->GetStream(i);
    if (stream)
    {
      CDemuxStream* streamInternal = CopyStream(stream);
      if (!streamInternal)
        continue;
      unsigned int idx = m_StreamMap.size();
      streamInternal->iId = idx;
      m_InternalToExternalStreamMap[std::make_pair(stream->iId, demuxer)] = idx;
      m_StreamMap.push_back(streamInternal);
    }
    
    //firstFree++;
  }

  return true;
}

bool CDVDDemuxMultiFiles::RebuildStreamMap()
{
  return true;
}

int CDVDDemuxMultiFiles::GetNrOfStreams()
{
  int streamsCount = 0;
  for (std::map<int, DemuxPtr>::iterator iter = m_pDemuxers.begin(); iter != m_pDemuxers.end(); ++iter)
    streamsCount += iter->second->GetNrOfStreams();

  return streamsCount;
}

int CDVDDemuxMultiFiles::GetStreamLength()
{
  if (m_pDemuxer)
    return m_pDemuxer->GetStreamLength();
  else
    return 0;
}

bool CDVDDemuxMultiFiles::Open(CDVDInputStream* pInput)
{
  if (!pInput || !pInput->IsStreamType(DVDSTREAM_TYPE_MULTIFILES))
    return false;

  m_pInput = dynamic_cast<CDVDInputStreamMultiFiles*>(pInput);

  if (!m_pInput)
    return false;

  std::map<int, InputStreamPtr>::iterator iter = m_pInput->m_pInputStreams.begin();
  while (iter != m_pInput->m_pInputStreams.end())
  {
    int inputIndex = iter->first;
    DemuxPtr demuxer = DemuxPtr(CDVDFactoryDemuxer::CreateDemuxer(iter->second.get()));
    if (!demuxer)
    {
      if (inputIndex == 0)
        return false;
      m_pInput->m_pInputStreams.erase(iter++);
    }
    else
    {
      UpdateStreamMap(inputIndex, demuxer);
      m_pDemuxers[inputIndex] = demuxer;
      ++iter;
    }
  }

  m_pDemuxer = m_pDemuxers.begin()->second.get();
  return (m_pDemuxer != NULL);
}

void CDVDDemuxMultiFiles::Reset()
{
  for (std::map<int, DemuxPtr>::iterator iter = m_pDemuxers.begin(); iter != m_pDemuxers.end(); ++iter)
    iter->second->Reset();
}

void CDVDDemuxMultiFiles::Abort()
{
  for (std::map<int, DemuxPtr>::iterator iter = m_pDemuxers.begin(); iter != m_pDemuxers.end(); ++iter)
    iter->second->Abort();
}

void CDVDDemuxMultiFiles::Flush()
{
  for (std::map<int, DemuxPtr>::iterator iter = m_pDemuxers.begin(); iter != m_pDemuxers.end(); ++iter)
    iter->second->Flush();
}

void CDVDDemuxMultiFiles::Dispose()
{
  for each (CDemuxStream* stream in m_StreamMap)
  {
    delete stream;
  }
  m_StreamMap.clear();
  m_pDemuxers.clear();
  m_pDemuxer = NULL;
}

DemuxPacket* CDVDDemuxMultiFiles::Read()
{
  std::map<int, DemuxPtr>::iterator demuxIter = m_pDemuxers.find(m_nextDemuxerToRead);

  if (demuxIter == m_pDemuxers.end())
    return NULL;

  DemuxPtr demuxer = demuxIter->second;


  DemuxPacket* packet = demuxer->Read();
  if (packet)
  {
    std::map<std::pair<int, DemuxPtr>, unsigned int>::iterator iter = m_InternalToExternalStreamMap.find(std::make_pair(packet->iStreamId, demuxer));
    if (iter != m_InternalToExternalStreamMap.end())
    {
      packet->iStreamId = iter->second;
    }
    else //new stream
    {
      //UpdateStreamMap()
    }
  }

  m_streamsRead++;


  // check if it's next demuxer's turn
  if (demuxer->GetNrOfStreams() <= m_streamsRead)
  {
    m_nextDemuxerToRead = (m_nextDemuxerToRead + 1) % m_pDemuxers.size();
    m_streamsRead = 0;
  }

  return packet;
}

bool CDVDDemuxMultiFiles::SeekTime(int time, bool backwords, double* startpts)
{
  for (std::map<int, DemuxPtr>::iterator iter = m_pDemuxers.begin(); iter != m_pDemuxers.end(); ++iter)
  {
    if (iter->second->SeekTime(time, false, startpts))
      CLog::Log(LOGDEBUG, "%s - starting demuxer from: %d", __FUNCTION__, time);
    else
    {
      if (iter->first == 0)
        return false;
      CLog::Log(LOGDEBUG, "%s - failed to start demuxing from: %d", __FUNCTION__, time);
    }
  }
  return true;
}

CDemuxStream* CDVDDemuxMultiFiles::CopyStream(CDemuxStream* right)
{
  CDemuxStream* stream = NULL;
  switch (right->type)
  {
  case STREAM_VIDEO:
  {
    CDemuxStreamVideo* rightS = dynamic_cast<CDemuxStreamVideo*>(right);
    CDemuxStreamVideo* streamV = new CDemuxStreamVideo();
    streamV->iFpsScale = rightS->iFpsScale;
    streamV->iFpsRate = rightS->iFpsRate;
    streamV->irFpsScale = rightS->irFpsScale;
    streamV->irFpsRate = rightS->irFpsRate;
    streamV->iHeight = rightS->iHeight;
    streamV->iWidth = rightS->iWidth;
    streamV->fAspect = rightS->fAspect;
    streamV->bVFR = rightS->bVFR;
    streamV->bPTSInvalid = rightS->bPTSInvalid;
    streamV->bForcedAspect = rightS->bForcedAspect;
    streamV->type = STREAM_VIDEO;
    streamV->iOrientation = rightS->iOrientation;
    streamV->iBitsPerPixel = rightS->iBitsPerPixel;
    stream = streamV;
    break;
  }
  case STREAM_AUDIO:
  {
    CDemuxStreamAudio* rightS = dynamic_cast<CDemuxStreamAudio*>(right);
    CDemuxStreamAudio* streamA = new CDemuxStreamAudio();
    streamA->iChannels = rightS->iChannels;
    streamA->iSampleRate = rightS->iSampleRate;
    streamA->iBlockAlign = rightS->iBlockAlign;
    streamA->iBitRate = rightS->iBitRate;
    streamA->iBitsPerSample = rightS->iBitsPerSample;
    streamA->type = STREAM_AUDIO;
    stream = streamA;
    break;
  }
  case STREAM_TELETEXT:
  {
    CDemuxStreamTeletext* streamT = new CDemuxStreamTeletext();
    streamT->type = STREAM_TELETEXT;
    stream = streamT;
    break;
  }
  case STREAM_SUBTITLE:
  {
    CDemuxStreamSubtitle* streamS = new CDemuxStreamSubtitle();
    streamS->type = STREAM_SUBTITLE;
    stream = streamS;
    break;
  }
  default:
    return NULL;
    break;
  }

  if (!stream)
    return NULL;

  stream->iId = right->iId;
  stream->iPhysicalId = right->iPhysicalId;
  stream->codec = right->codec;
  stream->codec_fourcc = right->codec_fourcc;
  stream->profile = right->profile;
  stream->level = right->level;
  stream->type = right->type;
  stream->source = right->source;
  stream->iDuration = right->iDuration;
  stream->pPrivate = right->pPrivate; //mmmh
  stream->ExtraData = NULL;
  stream->ExtraSize = right->ExtraSize;
  if (stream->ExtraSize)
  {
    stream->ExtraData = (uint8_t*) malloc(stream->ExtraSize);
    if (!stream->ExtraData)
      return NULL;
    memcpy(stream->ExtraData, right->ExtraData, stream->ExtraSize);
  }
  memcpy(stream->language, right->language, sizeof(right->language));
  stream->disabled = right->disabled;
  stream->changes = right->changes;
  stream->flags = right->flags;
  stream->orig_type = right->orig_type;

  return stream;
}