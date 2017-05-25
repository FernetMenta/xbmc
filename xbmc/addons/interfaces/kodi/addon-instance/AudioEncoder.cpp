/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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

#include "AudioEncoder.h"
#include "utils/log.h"

namespace ADDON
{

CAudioEncoder::CAudioEncoder(AddonInfoPtr addonInfo)
  : IAddonInstanceHandler(ADDON_AUDIOENCODER, addonInfo)
{
  m_struct = { 0 };
}

bool CAudioEncoder::Init(AddonToKodiFuncTable_AudioEncoder &callbacks)
{
  m_struct.toKodi = callbacks;
  if (!CreateInstance(ADDON_INSTANCE_AUDIOENCODER, &m_struct) || !m_struct.toAddon.Start)
    return false;

  return m_struct.toAddon.Start(&m_struct,
                                m_iInChannels,
                                m_iInSampleRate,
                                m_iInBitsPerSample,
                                m_strTitle.c_str(),
                                m_strArtist.c_str(),
                                m_strAlbumArtist.c_str(),
                                m_strAlbum.c_str(),
                                m_strYear.c_str(),
                                m_strTrack.c_str(),
                                m_strGenre.c_str(),
                                m_strComment.c_str(),
                                m_iTrackLength);
}

int CAudioEncoder::Encode(int nNumBytesRead, uint8_t* pbtStream)
{
  if (m_struct.toAddon.Encode)
    return m_struct.toAddon.Encode(&m_struct, nNumBytesRead, pbtStream);
  return 0;
}

bool CAudioEncoder::Close()
{
  bool ret = false;
  if (m_struct.toAddon.Finish)
    ret = m_struct.toAddon.Finish(&m_struct);

  DestroyInstance();
  m_struct = { 0 };

  return ret;
}

} /* namespace ADDON */
