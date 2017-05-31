#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#include "AddonDll.h"

namespace ADDON
{

//   class IAddonInstanceHandler
//   {
//   public:
//     IAddonInstanceHandler(TYPE type, const std::string& instanceID = "");
//     IAddonInstanceHandler(TYPE type, const AddonInfoPtr& addonInfo, kodi::addon::IAddonInstance* parentInstance = nullptr, const std::string& instanceID = "");
//     virtual ~IAddonInstanceHandler();
// 
//     const TYPE UsedType() const { return m_type; }
//     const CAddonType* Type(TYPE type) const { return m_addon->Type(type); }
//     const std::string& InstanceID() { return m_instanceId; }
// 
//     std::string ID() const { return m_addon->ID(); }
//     std::string Name() const { return m_addon->Name(); }
//     std::string Author() const { return m_addon->Author(); }
//     std::string Icon() const { return m_addon->Icon(); }
//     std::string Path() const { return m_addon->Path(); }
//     std::string Profile() const { return m_addon->Profile(); }
//     AddonVersion Version() const { return m_addon ? m_addon->Version() : AddonVersion("0.0.0"); }
// 
//     const AddonInfoPtr& AddonInfo() { return m_addonInfo; }
// 
//     bool CreateInstance(ADDON_TYPE instanceType, KODI_HANDLE instance);
//     void DestroyInstance();
//     const AddonDllPtr& Addon() { return m_addon; }
// 
//   private:
//     TYPE m_type;
//     std::string m_instanceId;
//     kodi::addon::IAddonInstance* m_parentInstance;
//     AddonInfoPtr m_addonInfo;
//     AddonDllPtr m_addon;
//   };

  class IAddonInstanceHandler
  {
  public:
    IAddonInstanceHandler(ADDON_TYPE type, const std::string& instanceID = "");
    IAddonInstanceHandler(ADDON_TYPE type, const AddonInfoPtr& addonInfo, KODI_HANDLE parentInstance = nullptr, const std::string& instanceID = "");
    virtual ~IAddonInstanceHandler();

    const ADDON_TYPE UsedType() const { return m_type; }
    const CAddonType* Type(TYPE type) const { return m_addon->Type(type); }
    const std::string& InstanceID() { return m_instanceId; }

    std::string ID() const { return m_addon ? m_addon->ID() : ""; }
    std::string Name() const { return m_addon ? m_addon->Name() : ""; }
    std::string Author() const { return m_addon ? m_addon->Author() : ""; }
    std::string Icon() const { return m_addon ? m_addon->Icon() : ""; }
    std::string Path() const { return m_addon ? m_addon->Path() : ""; }
    std::string Profile() const { return m_addon ? m_addon->Profile() : ""; }
    AddonVersion Version() const { return m_addon ? m_addon->Version() : AddonVersion("0.0.0"); }

    const AddonInfoPtr& AddonInfo() { return m_addonInfo; }

    bool CreateInstance(KODI_HANDLE instance);
    void DestroyInstance();
    const AddonDllPtr& Addon() { return m_addon; }

  private:
    ADDON_TYPE m_type;
    std::string m_instanceId;
    KODI_HANDLE m_addonInstance;
    KODI_HANDLE m_parentInstance;
    AddonInfoPtr m_addonInfo;
    AddonDllPtr m_addon;
  };

} /* namespace ADDON */

