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

#include "DVDInputStreamMultiFiles.h"
#include "DVDFactoryInputStream.h"
#include "filesystem/File.h"
#include "filesystem/IFile.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include<map>

using namespace XFILE;

CDVDInputStreamMultiFiles::CDVDInputStreamMultiFiles(IDVDPlayer* pPlayer, const std::vector<std::string>& filenames) : CDVDInputStream(DVDSTREAM_TYPE_MULTIFILES),
  m_pPlayer(pPlayer),
  m_filenames(filenames)
{
}

CDVDInputStreamMultiFiles::~CDVDInputStreamMultiFiles()
{
  Close();
}

bool CDVDInputStreamMultiFiles::IsEOF()
{
  return !m_pInputStream || m_pInputStream->IsEOF();
}

bool CDVDInputStreamMultiFiles::Open(const char* strFile, const std::string& content)
{
  if (!m_pPlayer || m_filenames.empty())
    return false;

  for (unsigned int i = 0, j = 0; i < m_filenames.size(); i++)
  {
    CFileItem fileitem = CFileItem(m_filenames[i]);
    std::string filemimetype = fileitem.GetMimeType();
    InputStreamPtr inputstream(CDVDFactoryInputStream::CreateInputStream(m_pPlayer, m_filenames[i], filemimetype));
    if (!inputstream)
    {
      CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - unable to create input stream for file [%s]", m_filenames[i].c_str());
      // if we can't create an input stream for the "master" file return false.
      if (i == 0)
        return false;
      continue;
    }
    else
      inputstream->SetFileItem(fileitem);

    if (!inputstream->Open(m_filenames[i].c_str(), filemimetype))
    {
      CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - error opening file [%s]", m_filenames[i].c_str());
      // if we can't open an input stream for the "master" file return false.
      if (i == 0)
        return false;
      continue;
    }
    m_pInputStreams[j++] = inputstream;
  }
  m_pInputStream = m_pInputStreams[0].get();
  return true;
}

void CDVDInputStreamMultiFiles::Abort()
{
  for (std::map<int, InputStreamPtr>::iterator iter = m_pInputStreams.begin(); iter != m_pInputStreams.end(); ++iter)
    iter->second->Abort();
}

void CDVDInputStreamMultiFiles::Close()
{
  m_pInputStreams.clear();
  m_pInputStream = NULL;
  CDVDInputStream::Close();
}

int CDVDInputStreamMultiFiles::Read(uint8_t* buf, int buf_size)
{
  return -1;
}

int64_t CDVDInputStreamMultiFiles::Seek(int64_t offset, int whence)
{
  return -1;
}

int64_t CDVDInputStreamMultiFiles::GetLength()
{
  if (m_pInputStream)
    return m_pInputStream->GetLength();
  else
    return 0;
}

bool CDVDInputStreamMultiFiles::GetCacheStatus(XFILE::SCacheStatus *status)
{
    return false;
}

BitstreamStats CDVDInputStreamMultiFiles::GetBitstreamStats() const
{
    return m_stats;
}

int CDVDInputStreamMultiFiles::GetBlockSize()
{
    return 0;
}

void CDVDInputStreamMultiFiles::SetReadRate(unsigned rate)
{
  for (std::map<int, InputStreamPtr>::iterator iter = m_pInputStreams.begin(); iter != m_pInputStreams.end(); ++iter)
    iter->second->SetReadRate(rate);
}
