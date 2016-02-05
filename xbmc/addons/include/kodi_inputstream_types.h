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

  struct INPUTSTREAM_INFO
  {
    int dummy;
  };

  struct INPUTSTREAM_PROPS
  {
    int dummy;
  };

  /*!
   * @brief Structure to transfer the methods from xbmc_inputstream_dll.h to XBMC
   */
  typedef struct InputStream
  {
    bool         (__cdecl* OpenStream)(const char*);
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
  } InputStream;

#ifdef __cplusplus
}
#endif

