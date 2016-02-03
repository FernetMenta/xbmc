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

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif
#include <string.h>
#include <stdint.h>

#include "xbmc_addon_types.h"
#include "xbmc_codec_types.h"

/*! @note Define "USE_INPUTSTREAM" at compile time if demuxing in the InputStream add-on is used.
 *        Also XBMC's "DVDDemuxPacket.h" file must be in the include path of the add-on,
 *        and the add-on should set bHandlesDemuxing to true.
 */
#ifdef USE_INPUTSTREAM_DEMUX
#include "DVDDemuxPacket.h"
#else
struct DemuxPacket;
#endif

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

/* using the default avformat's MAX_STREAMS value to be safe */
#define INPUTSTREAM_STREAM_MAX_STREAMS 20

/* current INPUTSTREAM API version */
#define XBMC_INPUTSTREAM_API_VERSION "0.0.1"

/* min. INPUTSTREAM API version */
#define XBMC_INPUTSTREAM_MIN_API_VERSION "0.0.1"

#ifdef __cplusplus
extern "C" {
#endif

  static const unsigned int INPUTSTREAM_ADDON_URL_STRING_LENGTH             = 1024;
  static const unsigned int INPUTSTREAM_ADDON_LICENSE_SYSTEM_STRING_LENGTH  = 128;
  static const unsigned int INPUTSTREAM_ADDON_LICENSE_KEY_STRING_LENGTH     = 1024;

  /*!
   * @brief INPUTSTREAM add-on error codes
   */
  typedef enum
  {
    INPUTSTREAM_ERROR_NO_ERROR           = 0,  /*!< @brief no error occurred */
    INPUTSTREAM_ERROR_UNKNOWN            = -1, /*!< @brief an unknown error occurred */
    INPUTSTREAM_ERROR_NOT_IMPLEMENTED    = -2, /*!< @brief the method that XBMC called is not implemented by the add-on */
    INPUTSTREAM_ERROR_INVALID_PARAMETERS = -3, /*!< @brief the parameters of the method that was called are invalid for this operation */
    INPUTSTREAM_ERROR_FAILED             = -4, /*!< @brief the command failed */
  } INPUTSTREAM_ERROR;

  /*!
   * @brief Properties passed to the Create() method of an add-on.
   */
  typedef struct INPUTSTREAM_PROPERTIES
  {
    const char* strUserPath;           /*!< @brief path to the user profile */
    const char* strClientPath;         /*!< @brief path to this add-on */
  } INPUTSTREAM_PROPERTIES;

  /*!
   * @brief INPUTSTREAM add-on capabilities. All capabilities are set to "false" as default.
   * If a capabilty is set to true, then the corresponding methods from xbmc_inputstream_dll.h need to be implemented.
   */
  typedef struct INPUTSTREAM_ADDON_CAPABILITIES
  {
    bool bHandlesDemuxing;              /*!< @brief true if this add-on demultiplexes packets. */
  } ATTRIBUTE_PACKED INPUTSTREAM_ADDON_CAPABILITIES;

  /*!
   * @brief INPUTSTREAM stream properties
   */
  typedef struct INPUTSTREAM_STREAM_PROPERTIES
  {
    unsigned int iStreamCount;
    struct INPUTSTREAM_STREAM
    {
      unsigned int      iPhysicalId;        /*!< @brief (required) physical index */
      xbmc_codec_type_t iCodecType;         /*!< @brief (required) codec type this stream */
      xbmc_codec_id_t   iCodecId;           /*!< @brief (required) codec id of this stream */
      char              strLanguage[4];     /*!< @brief (required) language id */
      int               iIdentifier;        /*!< @brief (required) stream id */
      int               iFPSScale;          /*!< @brief (required) scale of 1000 and a rate of 29970 will result in 29.97 fps */
      int               iFPSRate;           /*!< @brief (required) FPS rate */
      int               iHeight;            /*!< @brief (required) height of the stream reported by the demuxer */
      int               iWidth;             /*!< @brief (required) width of the stream reported by the demuxer */
      float             fAspect;            /*!< @brief (required) display aspect ratio of the stream */
      int               iSampleRate;        /*!< @brief (required) sample rate */
      int               iBlockAlign;        /*!< @brief (required) block alignment */
      int               iBitRate;           /*!< @brief (required) bit rate */
      int               iBitsPerSample;     /*!< @brief (required) bits per sample */
     } stream[INPUTSTREAM_STREAM_MAX_STREAMS];      /*!< @brief (required) the streams */
   } ATTRIBUTE_PACKED INPUTSTREAM_STREAM_PROPERTIES;

  /*!
   * @brief Representation of a input stream.
   */
  typedef struct INPUTSTREAM_STREAM
  {
    char         strStreamURL[INPUTSTREAM_ADDON_URL_STRING_LENGTH];                   /*!< @brief the URL of this stream. */

    char         strLicenseSystem[INPUTSTREAM_ADDON_LICENSE_SYSTEM_STRING_LENGTH];    /*!< @brief the license system for decrypting data. 
                                                                                       This is normaly the hex string of the reverse URL (com.widevine.aplha) */
    char         strLicenseKey[INPUTSTREAM_ADDON_LICENSE_KEY_STRING_LENGTH];          /*!< @brief the license key for decrypting data. */
  } ATTRIBUTE_PACKED INPUTSTREAM_STREAM;
   
  /*!
   * @brief Structure to transfer the methods from xbmc_inputstream_dll.h to XBMC
   */
  typedef struct InputStreamClient
  {
    const char*  (__cdecl* GetInputStreamAPIVersion)(void);
    const char*  (__cdecl* GetMininumInputStreamAPIVersion)(void);
    const char*  (__cdecl* GetGUIAPIVersion)(void);
    const char*  (__cdecl* GetMininumGUIAPIVersion)(void);
    INPUTSTREAM_ERROR    (__cdecl* GetAddonCapabilities)(INPUTSTREAM_ADDON_CAPABILITIES*);
    INPUTSTREAM_ERROR    (__cdecl* GetStreamProperties)(INPUTSTREAM_STREAM_PROPERTIES*);
    bool         (__cdecl* OpenStream)(const INPUTSTREAM_CHANNEL&);
    void         (__cdecl* CloseStream)(void);
    int          (__cdecl* ReadStream)(unsigned char*, unsigned int);
    long long    (__cdecl* SeekStream)(long long, int);
    long long    (__cdecl* PositionStream)(void);
    long long    (__cdecl* LengthStream)(void);
    void         (__cdecl* DemuxReset)(void);
    void         (__cdecl* DemuxAbort)(void);
    void         (__cdecl* DemuxFlush)(void);
    DemuxPacket* (__cdecl* DemuxRead)(void);
    bool         (__cdecl* CanPauseStream)(void);
    void         (__cdecl* PauseStream)(bool);
    bool         (__cdecl* CanSeekStream)(void);
    bool         (__cdecl* SeekTime)(int, bool, double*);
    void         (__cdecl* SetSpeed)(int);
    bool         (__cdecl* IsRealTimeStream)(void);
  } InputStreamClient;

#ifdef __cplusplus
}
#endif

