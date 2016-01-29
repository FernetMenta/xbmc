#pragma once
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

#include "DVDDemux.h"
#include <map>
#include "pvr/addons/PVRClient.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class CDVDDemuxClient;
struct PVR_STREAM_PROPERTIES;

class CDemuxStreamClientInternal
{
public:
  CDemuxStreamClientInternal(CDVDDemuxClient *parent);
  ~CDemuxStreamClientInternal();

  void DisposeParser();

  CDVDDemuxClient *m_parent;
  AVCodecParserContext *m_parser;
  AVCodecContext *m_context;
  bool m_parser_split;
};

class CDemuxStreamVideoClient
  : public CDemuxStreamVideo
  , public CDemuxStreamClientInternal
{
public:
  CDemuxStreamVideoClient(CDVDDemuxClient *parent)
    : CDemuxStreamClientInternal(parent)
  {}
  virtual std::string GetStreamInfo() override;
};

class CDemuxStreamAudioClient
  : public CDemuxStreamAudio
  , public CDemuxStreamClientInternal
{
public:
  CDemuxStreamAudioClient(CDVDDemuxClient *parent)
    : CDemuxStreamClientInternal(parent)
  {}
  virtual std::string GetStreamInfo() override;
};

class CDemuxStreamSubtitleClient
  : public CDemuxStreamSubtitle
  , public CDemuxStreamClientInternal
{
public:
  CDemuxStreamSubtitleClient(CDVDDemuxClient *parent)
    : CDemuxStreamClientInternal(parent)
  {}
  virtual std::string GetStreamInfo() override;
};


class CDVDDemuxClient : public CDVDDemux
{
  friend class CDemuxStreamClientInternal;

public:

  CDVDDemuxClient();
  ~CDVDDemuxClient();

  bool Open(CDVDInputStream* pInput);
  void Dispose();
  void Reset();
  void Abort();
  void Flush();
  DemuxPacket* Read();
  bool SeekTime(int time, bool backwords = false, double* startpts = NULL);
  void SetSpeed(int iSpeed);
  int GetStreamLength() { return 0; }
  CDemuxStream* GetStream(int iStreamId);
  int GetNrOfStreams();
  std::string GetFileName();
  virtual std::string GetStreamCodecName(int iStreamId) override;

protected:
  CDVDInputStream* m_pInput;
#ifndef MAX_STREAMS
  #define MAX_STREAMS 100
#endif
  CDemuxStream* m_streams[MAX_STREAMS]; // maximum number of streams that ffmpeg can handle
  std::shared_ptr<PVR::CPVRClient> m_pvrClient;

private:
  void RequestStreams();
  void ParsePacket(DemuxPacket* pPacket);
  void DisposeStream(int iStreamId);
};

