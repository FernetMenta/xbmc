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
#include <map>

typedef DllAddon<InputStream, INPUTSTREAM_PROPS> DllInputStream;
namespace ADDON
{
  typedef CAddonDll<DllInputStream, InputStream, INPUTSTREAM_PROPS> InputStreamDll;

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

  protected:
    std::map<std::string, std::string> m_fileItemProps;
  };

} /*namespace ADDON*/
