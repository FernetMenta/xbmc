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

std::string CDemuxStream::GetStreamInfo()
{
  return streamInfo;
}

std::string CDemuxStream::GetStreamName()
{
  return streamName;
}

/**************************    AUDIO   **************************/

std::string CDemuxStreamAudio::GetStreamTypeName()
{
  char sInfo[64] = { 0 };

  if (codec == AV_CODEC_ID_AC3) strcpy(sInfo, "AC3 ");
  else if (codec == AV_CODEC_ID_DTS)
  {
#ifdef FF_PROFILE_DTS_HD_MA
    if (profile == FF_PROFILE_DTS_HD_MA)
      strcpy(sInfo, "DTS-HD MA ");
    else if (profile == FF_PROFILE_DTS_HD_HRA)
      strcpy(sInfo, "DTS-HD HRA ");
    else
#endif
      strcpy(sInfo, "DTS ");
  }
  else if (codec == AV_CODEC_ID_MP2) strcpy(sInfo, "MP2 ");
  else if (codec == AV_CODEC_ID_TRUEHD) strcpy(sInfo, "Dolby TrueHD ");
  else strcpy(sInfo, "");

  if (iChannels == 1) strcat(sInfo, "Mono");
  else if (iChannels == 2) strcat(sInfo, "Stereo");
  else if (iChannels == 6) strcat(sInfo, "5.1");
  else if (iChannels == 8) strcat(sInfo, "7.1");
  else if (iChannels != 0)
  {
    char temp[32];
    sprintf(temp, " %d%s", iChannels, "-chs");
    strcat(sInfo, temp);
  }
  return sInfo;
}

std::string CDemuxStreamAudio::GetStreamInfo()
{
  if (!streamInfo.empty())
    return streamInfo;

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

  char buffer[64];
  if (iSampleRate > 0)
  {
    sprintf(buffer, ", %d Hz", iSampleRate);
    strInfo += buffer;
  }

  if (iChannels == 1) strInfo += ", mono";
  else if (iChannels == 2) ", stereo";
  else if (iChannels == 6) ", 5.1";
  else if (iChannels == 8) ", 7.1";

  return strInfo;
}

/**************************    VIDEO   **************************/

std::string CDemuxStreamVideo::GetStreamInfo()
{
  if (!streamInfo.empty())
    return streamInfo;

  std::string strInfo;
  switch (codec)
  {
  case AV_CODEC_ID_MPEG2VIDEO:
    strInfo = "mpeg2video";
    break;
  case AV_CODEC_ID_H264:
    strInfo = "h264";
    break;
  case AV_CODEC_ID_HEVC:
    strInfo = "hevc";
    break;
  default:
    strInfo = "[unknown]";
    break;
  }

  char buffer[64];
  if (iWidth > 0 || iHeight > 0)
  {
    sprintf(buffer, ", %dx%d [SAR ? DAR ?]", iWidth, iHeight);
    strInfo += buffer;
  }

  return strInfo;
}
