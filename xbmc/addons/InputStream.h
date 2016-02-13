/*
 *      Copyright (C) 2016 Team Kodi
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
#pragma once

#include "AddonDll.h"
#include "include/kodi_inputstream_types.h"
#include "FileItem.h"
#include <vector>
#include <map>

typedef DllAddon<InputStreamAddonFunctions, INPUTSTREAM_PROPS> DllInputStream;

class CDemuxStream;

namespace ADDON
{
  typedef CAddonDll<DllInputStream, InputStreamAddonFunctions, INPUTSTREAM_PROPS> InputStreamDll;

  class CInputStream : public InputStreamDll
  {
  public:
    CInputStream(const AddonProps &props)
      : InputStreamDll(props)
    {};
    CInputStream(const cp_extension_t *ext);
    virtual ~CInputStream() {}
    virtual AddonPtr Clone() const;

    bool Supports(CFileItem &fileitem);
    bool Open(CFileItem &fileitem);
    void Close();

    bool HasDemux() { return m_caps.m_supportsIDemux; };
    bool HasSeekTime() { return m_caps.m_supportsISeekTime; };
    bool HasDisplayTime() { return m_caps.m_supportsIDisplayTime; };

    int GetNrOfStreams();
    CDemuxStream* GetStream(int iStreamId);
    DemuxPacket* ReadDemux();

  protected:
    void UpdateStreams();
    void DisposeStreams();

    std::vector<std::string> m_fileItemProps;
    std::vector<std::string> m_pathList;
    INPUTSTREAM_CAPABILITIES m_caps;
    std::map<int, CDemuxStream*> m_streams;
  };

} /*namespace ADDON*/
