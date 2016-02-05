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

#include "kodi_inputstream_types.h"
#include "xbmc_addon_dll.h"

/*!
 * Functions that the InputStream client add-on must implement, but some can be empty.
 *
 * The 'remarks' field indicates which methods should be implemented, and which ones are optional.
 */

extern "C"
{
  /*! @name InputStream add-on methods */
  //@{
  /*!
   * Get the XBMC_INPUTSTREAM_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_INPUTSTREAM_API_VERSION that was used to compile this add-on.
   * @remarks Valid implementation required.
   */
  const char* GetInputStreamAPIVersion(void);

  /*!
   * Get the XBMC_INPUTSTREAM_MIN_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_INPUTSTREAM_MIN_API_VERSION that was used to compile this add-on.
   * @remarks Valid implementation required.
   */
  const char* GetMininumInputStreamAPIVersion(void);

  /*!
   * Get the XBMC_GUI_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_GUI_API_VERSION that was used to compile this add-on.
   * @remarks Valid implementation required.
   */
  const char* GetGUIAPIVersion(void);

  /*!
   * Get the XBMC_GUI_MIN_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_GUI_MIN_API_VERSION that was used to compile this add-on.
   * @remarks Valid implementation required.
   */
  const char* GetMininumGUIAPIVersion(void);

  /*!
   * Get the list of features that this add-on provides.
   * Called by XBMC to query the add-on's capabilities.
   * Used to check which options should be presented in the UI, which methods to call, etc.
   * All capabilities that the add-on supports should be set to true.
   * @param pCapabilities The add-on's capabilities.
   * @return INPUTSTREAM_ERROR_NO_ERROR if the properties were fetched successfully.
   * @remarks Valid implementation required.
   */
  INPUTSTREAM_ERROR GetAddonCapabilities(INPUTSTREAM_ADDON_CAPABILITIES *pCapabilities);

  /** @name InputStream stream methods, used to open and close a stream, and optionally perform read operations on the stream */
  //@{
  /*!
   * Open a stream.
   * @param stream The stream to open including license string.
   * @return True if the stream has been opened successfully, false otherwise.
   * @remarks
   */
  bool OpenStream(const INPUTSTREAM_STREAM& stream);

  /*!
   * Close an open stream.
   * @remarks
   */
  void CloseStream(void);

  /*!
   * Read from an open stream.
   * @param pBuffer The buffer to store the data in.
   * @param iBufferSize The amount of bytes to read.
   * @return The amount of bytes that were actually read from the stream.
   * @remarks Return -1 if this add-on won't provide this function.
   */
  int ReadStream(unsigned char* pBuffer, unsigned int iBufferSize);

  /*!
   * Seek in a stream.
   * @param iPosition The position to seek to.
   * @param iWhence ?
   * @return The new position.
   * @remarks Return -1 if this add-on won't provide this function.
   */
  long long SeekStream(long long iPosition, int iWhence = SEEK_SET);

  /*!
   * @return The position in the stream that's currently being read.
   * @remarks Return -1 if this add-on won't provide this function.
   */
  long long PositionStream(void);

  /*!
   * @return The total length of the stream that's currently being read.
   * @remarks Return -1 if this add-on won't provide this function.
   */
  long long LengthStream(void);

  /*!
   * Get the stream properties of the stream that's currently being read.
   * @param pProperties The properties of the currently playing stream.
   * @return INPUTSTREAM_ERROR_NO_ERROR if the properties have been fetched successfully.
   * @remarks Return INPUTSTREAM_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  INPUTSTREAM_ERROR GetStreamProperties(INPUTSTREAM_STREAM_PROPERTIES* pProperties);
  //@}

  /** @name InputStreamInputStreamInputStreamInputStream demultiplexer methods
   *  @remarks Only used by XBMC is bHandlesDemuxing is set to true.
   */
  //@{
  /*!
   * Reset the demultiplexer in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxReset(void);

  /*!
   * Abort the demultiplexer thread in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxAbort(void);

  /*!
   * Flush all data that's currently in the demultiplexer buffer in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxFlush(void);

  /*!
   * Read the next packet from the demultiplexer, if there is one.
   * @return The next packet.
   *         If there is no next packet, then the add-on should return the
   *         packet created by calling AllocateDemuxPacket(0) on the callback.
   *         If the stream changed and XBMC's player needs to be reinitialised,
   *         then, the add-on should call AllocateDemuxPacket(0) on the
   *         callback, and set the streamid to DMX_SPECIALID_STREAMCHANGE and
   *         return the value.
   *         The add-on should return NULL if an error occured.
   * @remarks Return NULL if this add-on won't provide this function.
   */
  DemuxPacket* DemuxRead(void);
  //@}

  /*!
   * Check if the backend support pausing the currently playing stream
   * This will enable/disable the pause button in XBMC based on the return value
   * @return false if the InputStream addon/backend does not support pausing, true if possible
   */
  bool CanPauseStream();

  /*!
   * Check if the backend supports seeking for the currently playing stream
   * This will enable/disable the rewind/forward buttons in XBMC based on the return value
   * @return false if the InputStream addon/backend does not support seeking, true if possible
   */
  bool CanSeekStream();

  /*!
   * @brief Notify the InputStream addon that XBMC (un)paused the currently playing stream
   */
  void PauseStream(bool bPaused);

  /*!
   * Notify the InputStream addon/demuxer that XBMC wishes to seek the stream by time
   * @param time The absolute time since stream start
   * @param backwards True to seek to keyframe BEFORE time, else AFTER
   * @param startpts can be updated to point to where display should start
   * @return True if the seek operation was possible
   * @remarks Optional, and only used if addon has its own demuxer. Return False if this add-on won't provide this function.
   */
  bool SeekTime(int time, bool backwards, double *startpts);

  /*!
   * Notify the InputStream addon/demuxer that XBMC wishes to change playback speed
   * @param speed The requested playback speed
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  void SetSpeed(int speed);

  /*!
   *  Check for real-time streaming
   *  @return true if current stream is real-time
   */
  bool IsRealTimeStream();

  /*!
   * Called by XBMC to assign the function pointers of this add-on to pClient.
   * @param pClient The struct to assign the function pointers to.
   */
  void __declspec(dllexport) get_addon(struct InputStream* pClient)
  {
    pClient->OpenStream                     = OpenLiveStream;
    pClient->CloseStream                    = CloseLiveStream;
    pClient->ReadStream                     = ReadLiveStream;
    pClient->SeekStream                     = SeekLiveStream;
    pClient->PositionStream                 = PositionLiveStream;
    pClient->LengthStream                   = LengthLiveStream;
    pClient->CanPauseStream                 = CanPauseStream;
    pClient->PauseStream                    = PauseStream;
    pClient->CanSeekStream                  = CanSeekStream;
    pClient->SeekTime                       = SeekTime;
    pClient->SetSpeed                       = SetSpeed;

    pClient->DemuxReset                     = DemuxReset;
    pClient->DemuxAbort                     = DemuxAbort;
    pClient->DemuxFlush                     = DemuxFlush;
    pClient->DemuxRead                      = DemuxRead;

    pClient->IsRealTimeStream               = IsRealTimeStream;
  };
};
