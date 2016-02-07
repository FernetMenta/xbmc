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

CInputStreamAddon::CInputStreamAddon(CFileItem& fileitem)
: CDVDInputStream(DVDSTREAM_TYPE_ADDON, fileitem)
{
}

CInputStreamAddon::~CInputStreamAddon()
{
  Close();
}

bool CInputStreamAddon::IsEOF()
{
  return false;
}

bool CInputStreamAddon::Open()
{
  return false;
}

void CInputStreamAddon::Close()
{

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

bool CInputStreamAddon::OpenDemux()
{
  return false;
}

DemuxPacket* CInputStreamAddon::ReadDemux()
{
  return nullptr;
}

CDemuxStream* CInputStreamAddon::GetStream(int iStreamId)
{
  return nullptr;
}

int CInputStreamAddon::GetNrOfStreams()
{
  return 0;
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
