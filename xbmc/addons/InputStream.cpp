/*
 *      Copyright (C) 2016 Team Kodi
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
#include "InputStream.h"
#include "utils/StringUtils.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"


namespace ADDON
{

std::unique_ptr<CInputStream> CInputStream::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  std::string listitemprops = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@listitemprops");
  std::string name(ext->plugin->identifier);
  return std::unique_ptr<CInputStream>(new CInputStream(std::move(props),
                                                        std::move(name),
                                                        std::move(listitemprops)));
}

CInputStream::CInputStream(AddonProps props, std::string name, std::string listitemprops)
: InputStreamDll(std::move(props))
{
  m_fileItemProps = StringUtils::Tokenize(listitemprops, ",");
  for (auto &key : m_fileItemProps)
  {
    StringUtils::Trim(key);
    key = name + "." + key;
  }
}

AddonPtr CInputStream::Clone() const
{
  // Copy constructor is generated by compiler and calls parent copy constructor
  return AddonPtr(new CInputStream(*this));
}

bool CInputStream::Supports(CFileItem &fileitem)
{
  std::string pathList;
  try
  {
    pathList = m_pStruct->GetPathList();
  }
  catch (std::exception &e)
  {
    return false;
  }

  m_pathList = StringUtils::Tokenize(pathList, ",");
  for (auto &path : m_pathList)
  {
    StringUtils::Trim(path);
  }

  bool match = false;
  for (auto &path : m_pathList)
  {
    if (fileitem.GetPath().compare(0, path.length(), path) == 0)
    {
      match = true;
      break;
    }
  }
  if (!match)
    return false;

  return true;
}

bool CInputStream::Open(CFileItem &fileitem)
{
  INPUTSTREAM props;
  props.m_nCountInfoValues = 0;
  for (auto &key : m_fileItemProps)
  {
    if (fileitem.GetProperty(key).isNull())
      continue;
    props.m_ListItemProperties[props.m_nCountInfoValues].m_strKey = key.c_str();
    props.m_ListItemProperties[props.m_nCountInfoValues].m_strValue = fileitem.GetProperty(key).asString().c_str();
    props.m_nCountInfoValues++;
  }
  props.m_strURL = fileitem.GetPath().c_str();

  bool ret = false;
  try
  {
    ret = m_pStruct->Open(props);
    if (ret)
      m_caps = m_pStruct->GetCapabilities();
  }
  catch (std::exception &e)
  {
    return false;
  }

  UpdateStreams();
  return ret;
}

void CInputStream::Close()
{
  try
  {
    m_pStruct->Close();
  }
  catch (std::exception &e)
  {
    ;
  }
}

void CInputStream::UpdateStreams()
{
  DisposeStreams();

  INPUTSTREAM_IDS streamIDs;
  try
  {
    streamIDs = m_pStruct->GetStreamIds();
  }
  catch (std::exception &e)
  {
    DisposeStreams();
    return;
  }

  if (streamIDs.m_streamCount > INPUTSTREAM_IDS::MAX_STREAM_COUNT)
  {
    DisposeStreams();
    return;
  }

  for (int i=0; i<streamIDs.m_streamCount; i++)
  {
    INPUTSTREAM_INFO stream;
    try
    {
      stream = m_pStruct->GetStream(streamIDs.m_streamIds[i]);
      m_pStruct->EnableStream(streamIDs.m_streamIds[i], true);
    }
    catch (std::exception &e)
    {
      DisposeStreams();
      return;
    }

    if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_NONE)
      continue;

    std::string codecName(stream.m_codecName);
    StringUtils::ToLower(codecName);
    AVCodec *codec = avcodec_find_decoder_by_name(codecName.c_str());
    if (!codec)
      continue;

    CDemuxStream *demuxStream;

    if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_AUDIO)
    {
      CDemuxStreamAudio *audioStream = new CDemuxStreamAudio();

      audioStream->iChannels = stream.m_Channels;
      audioStream->iSampleRate = stream.m_SampleRate;
      audioStream->iBlockAlign = stream.m_BlockAlign;
      audioStream->iBitRate = stream.m_BitRate;
      audioStream->iBitsPerSample = stream.m_BitsPerSample;
      demuxStream = audioStream;
    }
    else if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_VIDEO)
    {
      CDemuxStreamVideo *videoStream = new CDemuxStreamVideo();

      videoStream->iFpsScale = stream.m_FpsScale;
      videoStream->iFpsRate = stream.m_FpsRate;
      videoStream->iHeight = stream.m_Width;
      videoStream->iWidth = stream.m_Height;
      videoStream->fAspect = stream.m_Aspect;
      videoStream->stereo_mode = "mono";
      demuxStream = videoStream;
    }
    else if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_SUBTITLE)
    {
      // TODO needs identifier in INPUTSTREAM_INFO
      continue;
    }
    else
      continue;

    demuxStream->iId = i;
    demuxStream->codec = codec->id;
    demuxStream->iPhysicalId = streamIDs.m_streamIds[i];
    demuxStream->language[0] = stream.m_language[0];
    demuxStream->language[1] = stream.m_language[1];
    demuxStream->language[2] = stream.m_language[2];
    demuxStream->language[3] = stream.m_language[3];

    if (stream.m_ExtraData && stream.m_ExtraSize)
    {
      demuxStream->ExtraData = new uint8_t[stream.m_ExtraSize];
      demuxStream->ExtraSize = stream.m_ExtraSize;
      for (unsigned int j=0; j<stream.m_ExtraSize; j++)
        demuxStream->ExtraData[j] = stream.m_ExtraData[j];
    }

    m_streams[i] = demuxStream;
  }
}

void CInputStream::DisposeStreams()
{
  for (auto &stream : m_streams)
    delete stream.second;
  m_streams.clear();
}

int CInputStream::GetNrOfStreams()
{
  return m_streams.size();
}

CDemuxStream* CInputStream::GetStream(int iStreamId)
{
  std::map<int, CDemuxStream*>::iterator it = m_streams.find(iStreamId);
  if (it != m_streams.end())
    return it->second;

  return nullptr;
}

DemuxPacket* CInputStream::ReadDemux()
{
  DemuxPacket* pPacket = nullptr;
  try
  {
    pPacket = m_pStruct->DemuxRead();
  }
  catch (std::exception &e)
  {
    return nullptr;
  }

  if (!pPacket)
  {
    return nullptr;
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    UpdateStreams();
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    UpdateStreams();
  }

  int id = 0;;
  for (auto &stream : m_streams)
  {
    if (stream.second->iPhysicalId == pPacket->iStreamId)
    {
      pPacket->iStreamId = id;
      return pPacket;
    }
    id++;
  }
  return pPacket;
}

} /*namespace ADDON*/

