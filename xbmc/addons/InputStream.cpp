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
#include "InputStream.h"
#include "utils/StringUtils.h"


namespace ADDON
{

CInputStream::CInputStream(const cp_extension_t* ext) : InputStreamDll(ext)
{
  std::string props = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@listitemprops");
  m_fileItemProps = StringUtils::Tokenize(props, ",");
}

AddonPtr CInputStream::Clone() const
{
  // Copy constructor is generated by compiler and calls parent copy constructor
  return AddonPtr(new CInputStream(*this));
}

bool CInputStream::Supports(CFileItem &fileitem)
{
  return false;
}

} /*namespace ADDON*/

