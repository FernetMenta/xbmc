/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"

int CDVDDemux::GetNrOfSubtitleStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_SUBTITLE) iCounter++;
  }

  return iCounter;
}

void CDemuxStream::GetSelectionInfo(DemuxSelectInfo &info)
{
  info.type = type;
  info.id = iId;
  info.language = g_LangCodeExpander.ConvertToISO6392T(language);
  info.flags = flags;
  info.channels = 0;
  info.width = 0;
  info.height = 0;
}

bool CDemuxStream::SetCodecName(const std::string avCodecName, const std::string internalCodecName)
{
  std::string codecName(avCodecName);
  StringUtils::ToLower(codecName);
  AVCodec *avcodec = avcodec_find_decoder_by_name(codecName.c_str());
  if (!avcodec)
    return false;
  
  codec = avcodec->id;

  codecName = internalCodecName;
  
  return true;
}

/**************************    AUDIO   **************************/

void CDemuxStreamAudio::GetSelectionInfo(DemuxSelectInfo &info)
{
  CDemuxStream::GetSelectionInfo(info);

  info.channels = iChannels;

  if (streamName.empty())
  {
    switch (codec)
    {
    case AV_CODEC_ID_DTS:
#ifdef FF_PROFILE_DTS_HD_MA
      if (profile == FF_PROFILE_DTS_HD_MA)
        info.name = "DTS-HD MA ";
      else if (profile == FF_PROFILE_DTS_HD_HRA)
        info.name = "DTS-HD HRA ";
      else
#endif
      info.name = "DTS";
      break;
    case AV_CODEC_ID_TRUEHD:
      info.name = "Dolby TrueHD ";
      break;
    default:
      info.name = codecName;
      break;
    }
  }
  else
    info.name = streamName;

  if (iChannels == 1) info.name += " - Mono";
  else if (iChannels == 2) info.name += " - Stereo";
  else if (iChannels == 6) info.name += " - 5.1";
  else if (iChannels == 8) info.name += " - 7.1";
  else if (iChannels != 0)
  {
    char temp[32];
    sprintf(temp, " - %d-chs", iChannels);
    info.name += temp;
  }
}

/**************************    VIDEO   **************************/

void CDemuxStreamVideo::GetSelectionInfo(DemuxSelectInfo &info)
{
  CDemuxStream::GetSelectionInfo(info);

  info.width = iWidth;
  info.height = iHeight;
  info.aspect_ratio = fAspect;

  if (streamName.empty())
    info.name = codecName;
  else
    info.name = streamName;

  if (iWidth > 0 || iHeight > 0)
  {
    char buffer[64];
    sprintf(buffer, ", %dx%d", iWidth, iHeight);
    info.name += buffer;
  }
}
