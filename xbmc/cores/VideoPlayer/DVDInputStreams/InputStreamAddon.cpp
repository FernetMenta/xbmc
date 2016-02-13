/*
 *      Copyright (C) 2015 Team Kodi
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

#include "InputStreamAddon.h"
#include "addons/InputStream.h"

CInputStreamAddon::CInputStreamAddon(CFileItem& fileitem, ADDON::CInputStream *inputStream)
: CDVDInputStream(DVDSTREAM_TYPE_ADDON, fileitem), m_addon(inputStream)
{
}

CInputStreamAddon::~CInputStreamAddon()
{
  Close();
  m_addon->Stop();
  delete m_addon;
}

bool CInputStreamAddon::Open()
{
  if (m_addon)
    return m_addon->Open(m_item);
  return false;
}

void CInputStreamAddon::Close()
{
  if (m_addon)
    return m_addon->Close();
}

bool CInputStreamAddon::IsEOF()
{
  return false;
}

int CInputStreamAddon::Read(uint8_t* buf, int buf_size)
{
  return 0;
}

int64_t CInputStreamAddon::Seek(int64_t offset, int whence)
{
  return -1;
}

int64_t CInputStreamAddon::GetLength()
{
  return -1;
}

bool CInputStreamAddon::Pause(double dTime)
{
  return true;
}

bool CInputStreamAddon::SeekTime(int ms)
{
  return false;
}

// ISekable
bool CInputStreamAddon::CanSeek()
{
  return false;
}

bool CInputStreamAddon::CanPause()
{
  return false;
}

// IDemux
bool CInputStreamAddon::OpenDemux()
{
  return false;
}

DemuxPacket* CInputStreamAddon::ReadDemux()
{
  if (!m_addon)
    return nullptr;

  return m_addon->ReadDemux();
}

CDemuxStream* CInputStreamAddon::GetStream(int iStreamId)
{
  if (!m_addon)
    return nullptr;

  return m_addon->GetStream(iStreamId);
}

int CInputStreamAddon::GetNrOfStreams()
{
  if (!m_addon)
    return 0;

  int count = m_addon->GetNrOfStreams();
  return count;
}

void CInputStreamAddon::SetSpeed(int iSpeed)
{

}

bool CInputStreamAddon::SeekTime(int time, bool backward, double* startpts)
{
  return false;
}

void CInputStreamAddon::AbortDemux()
{

}

void CInputStreamAddon::FlushDemux()
{

}
