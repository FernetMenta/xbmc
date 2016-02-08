#pragma once

/*
 *      Copyright (C) 2005-2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef BUILD_KODI_ADDON
#include "DVDDemuxPacket.h"
#else
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"
#endif

extern "C" {

  // this are properties given to the addon on create
  // at this time we have no parameters for the addon
  struct INPUTSTREAM_PROPS
  {
    int dummy;
  };

  struct INPUTSTREAM
  {
    static const unsigned int MAX_INFO_COUNT = 8;

    char *m_strURL;

    unsigned int m_nCountInfoValues;
    struct LISTITEMPROPERTY
    {
      const char *m_strKey;
      const char *m_strValue;
    } m_ListItemProperties[MAX_INFO_COUNT];
  };

  struct INPUTSTREAM_IDS
  {
    static const unsigned int MAX_STREAM_COUNT = 32;
    unsigned int m_streamIds[MAX_STREAM_COUNT];
  };

  struct INPUTSTREAM_INFO
  {
    enum STREAM_TYPE
    {
      TYPE_VIDEO,
      TYPE_AUDIO,
      TYPE_SUBTITLE,
      TYPE_TELETEXT
    } m_streamType;

    char m_codecName[32];                /*!< @brief (required) name of codec according to ffmpeg */
    const int m_pID;                     /*!< @brief (required) physical index */
    char m_language[4];                  /*!< @brief ISO 639 3-letter language code (empty string if undefined) */

    unsigned int m_FpsScale;             /*!< @brief Scale of 1000 and a rate of 29970 will result in 29.97 fps */
    unsigned int m_FpsRate;
    unsigned int m_Height;               /*!< @brief height of the stream reported by the demuxer */
    unsigned int m_Width;                /*!< @brief width of the stream reported by the demuxer */
    float m_Aspect;                      /*!< @brief display aspect of stream */

    unsigned int m_Channels;             /*!< @brief (required) amount of channels */
    unsigned int m_SampleRate;           /*!< @brief (required) sample rate */
    unsigned int m_BitRate;              /*!< @brief (required) bit rate */
    unsigned int m_BitsPerSample;        /*!< @brief (required) bits per sample */
    unsigned int m_BlockAlign;
  };

  /*!
   * @brief Structure to transfer the methods from xbmc_inputstream_dll.h to XBMC
   */
  typedef struct InputStream
  {
    bool (__cdecl* Open)(INPUTSTREAM&);
    void (__cdecl* Close)(void);
    INPUTSTREAM_IDS (__cdecl* GetStreamsIds)();
    INPUTSTREAM_INFO (__cdecl* GetStream)(int);
    void (__cdecl* EnableStream)(bool);
    int (__cdecl* ReadStream)(unsigned char*, unsigned int);
    long long (__cdecl* SeekStream)(long long, int);
    long long (__cdecl* PositionStream)(void);
    long long (__cdecl* LengthStream)(void);
    void (__cdecl* DemuxReset)(void);
    void (__cdecl* DemuxAbort)(void);
    void (__cdecl* DemuxFlush)(void);
    DemuxPacket* (__cdecl* DemuxRead)(void);
    bool (__cdecl* CanPauseStream)(void);
    void (__cdecl* PauseStream)(bool);
    bool (__cdecl* CanSeekStream)(void);
    bool (__cdecl* SeekTime)(int, bool, double*);
    void (__cdecl* SetSpeed)(int);
    bool (__cdecl* IsRealTimeStream)(void);
  } InputStream;

#ifdef __cplusplus
}
#endif

