/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2017 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "General.h"
#include "Filesystem.h"
#include "Network.h"

#include "addons/kodi-addon-dev-kit/include/kodi/General.h"

#include "Application.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonDll.h"
#include "addons/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace kodi; // addon-dev-kit namespace

namespace ADDON
{

void Interface_General::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi.kodi = (AddonToKodiFuncTable_kodi*)malloc(sizeof(AddonToKodiFuncTable_kodi));
  addonInterface->toKodi.kodi->open_settings_dialog = open_settings_dialog;
  addonInterface->toKodi.kodi->get_localized_string = get_localized_string;
  addonInterface->toKodi.kodi->unknown_to_utf8 = unknown_to_utf8;
  addonInterface->toKodi.kodi->queue_notification = queue_notification;
  addonInterface->toKodi.kodi->queue_notification_from_type = queue_notification_from_type;
  addonInterface->toKodi.kodi->queue_notification_with_image = queue_notification_with_image;
  addonInterface->toKodi.kodi->get_dvd_menu_language = get_dvd_menu_language;

  Interface_Filesystem::Init(addonInterface);
  Interface_Network::Init(addonInterface);
}

void Interface_General::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi.kodi)
  {
    free(addonInterface->toKodi.kodi);
    addonInterface->toKodi.kodi = nullptr;
  }
}

bool Interface_General::get_setting(void* kodiBase, const char* settingName, void* settingValue)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || settingName == nullptr || settingValue == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', settingName='%p', settingValue='%p')",
                                        __FUNCTION__, addon, settingName, settingValue);

    return false;
  }

  CLog::Log(LOGDEBUG, "kodi::General::%s - add-on '%s' requests setting '%s'",
                        __FUNCTION__, addon->Name().c_str(), settingName);

  if (strcasecmp(settingName, "__addonpath__") == 0)
  {
    strcpy((char*) settingValue, addon->Path().c_str());
    return true;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "kodi::General::%s - could't get settings for add-on '%s'", __FUNCTION__, addon->Name().c_str());
    return false;
  }

  const TiXmlElement *category = addon->GetSettingsXML()->FirstChildElement("category");
  if (!category) // add a default one...
    category = addon->GetSettingsXML();

  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const std::string   id = XMLUtils::GetAttribute(setting, "id");
      const std::string type = XMLUtils::GetAttribute(setting, "type");

      if (id == settingName && !type.empty())
      {
        if (type == "text"     || type == "ipaddress" ||
            type == "folder"   || type == "action"    ||
            type == "music"    || type == "pictures"  ||
            type == "programs" || type == "fileenum"  ||
            type == "file"     || type == "labelenum")
        {
          strcpy((char*) settingValue, addon->GetSetting(id).c_str());
          return true;
        }
        else if (type == "number" || type == "enum")
        {
          *(int*) settingValue = (int) atoi(addon->GetSetting(id).c_str());
          return true;
        }
        else if (type == "bool")
        {
          *(bool*) settingValue = (bool) (addon->GetSetting(id) == "true" ? true : false);
          return true;
        }
        else if (type == "slider")
        {
          const char *option = setting->Attribute("option");
          if (option && strcmpi(option, "int") == 0)
          {
            *(int*) settingValue = (int) atoi(addon->GetSetting(id).c_str());
            return true;
          }
          else
          {
            *(float*) settingValue = (float) atof(addon->GetSetting(id).c_str());
            return true;
          }
        }
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  CLog::Log(LOGERROR, "kodi::General::%s - can't find setting '%s' in '%s'", __FUNCTION__, settingName, addon->Name().c_str());

  return false;
}

bool Interface_General::set_setting(void* kodiBase, const char* settingName, const char* settingValue)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || settingName == nullptr || settingValue == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', settingName='%p', settingValue='%p')",
                                        __FUNCTION__, addon, settingName, settingValue);

    return false;
  }

  bool save = true;
  if (g_windowManager.IsWindowActive(WINDOW_DIALOG_ADDON_SETTINGS))
  {
    CGUIDialogAddonSettings* dialog = dynamic_cast<CGUIDialogAddonSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS));
    if (dialog->GetCurrentID() == addon->ID())
    {
      CGUIMessage message(GUI_MSG_SETTING_UPDATED, 0, 0);
      std::vector<std::string> params;
      params.push_back(settingName);
      params.push_back(settingValue);
      message.SetStringParams(params);
      g_windowManager.SendThreadMessage(message, WINDOW_DIALOG_ADDON_SETTINGS);
      save=false;
    }
  }
  if (save)
  {
    addon->UpdateSetting(settingName, settingValue);
    addon->SaveSettings();
  }

  return true;
}

void Interface_General::open_settings_dialog(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - called with a null pointer", __FUNCTION__);
    return;
  }

  // show settings dialog
  AddonInfoPtr addonInfo = CAddonMgr::GetInstance().GetInstalledAddonInfo(addon->ID());
  CGUIDialogAddonSettings::ShowAndGetInput(addonInfo);
}

char* Interface_General::get_localized_string(void* kodiBase, long dwCode)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p')", __FUNCTION__, addon);
    return nullptr;
  }

  if (g_application.m_bStop)
    return nullptr;

  std::string string;
  if ((dwCode >= 30000 && dwCode <= 30999) || (dwCode >= 32000 && dwCode <= 32999))
    string = g_localizeStrings.GetAddonString(addon->ID(), dwCode).c_str();
  else
    string = g_localizeStrings.Get(dwCode).c_str();

  char* buffer = strdup(string.c_str());
  return buffer;
}

char* Interface_General::unknown_to_utf8(void* kodiBase, const char* source, bool& ret, bool failOnBadChar)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon || !source)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', source='%p')", __FUNCTION__, addon, source);
    return nullptr;
  }
   
  std::string string;
  ret = g_charsetConverter.unknownToUTF8(source, string, failOnBadChar);
  char* buffer = strdup(string.c_str());
  return buffer;
}

void Interface_General::get_dvd_menu_language(void* kodiBase, char& language, unsigned int& iMaxStringSize)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p')", __FUNCTION__, addon);
    return;
  }

  std::string str = g_langInfo.GetDVDMenuLanguage();
  strncpy(&language, str.c_str(), iMaxStringSize);
  iMaxStringSize = str.length();
}

void Interface_General::queue_notification(
        void*                     kodiBase,
        const int                 type,
        const char*               message)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || message == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - called with a null pointer", __FUNCTION__);
    return;
  }

  switch (type)
  {
    case QUEUE_WARNING:
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, addon->Name(), message, 3000, true);
      CLog::Log(LOGDEBUG, "Interface_General - %s - %s - Warning Message: '%s'", __FUNCTION__, addon->Name().c_str(), message);
      break;
  
    case QUEUE_ERROR:
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, addon->Name(), message, 3000, true);
      CLog::Log(LOGDEBUG, "Interface_General - %s - %s - Error Message : '%s'", __FUNCTION__, addon->Name().c_str(), message);
      break;
  
    case QUEUE_INFO:
    default:
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, addon->Name(), message, 3000, false);
      CLog::Log(LOGDEBUG, "Interface_General - %s - %s - Info Message : '%s'", __FUNCTION__, addon->Name().c_str(), message);
      break;
  }
}

void Interface_General::queue_notification_from_type(
        void*                     kodiBase,
        const int                 type,
        const char*               aCaption,
        const char*               aDescription,
        unsigned int              displayTime,
        bool                      withSound,
        unsigned int              messageTime)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || aCaption == nullptr || aDescription == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - called with a null pointer", __FUNCTION__);
    return;
  }
  
  CGUIDialogKaiToast::eMessageType usedType;
  switch (type)
  {
  case QUEUE_WARNING:
    usedType = CGUIDialogKaiToast::Warning;
    break;
  case QUEUE_ERROR:
    usedType = CGUIDialogKaiToast::Error;
    break;
  case QUEUE_INFO:
  default:
    usedType = CGUIDialogKaiToast::Info;
    break;
  }
  CGUIDialogKaiToast::QueueNotification(usedType, aCaption, aDescription, displayTime, withSound, messageTime);
}

void Interface_General::queue_notification_with_image(
        void*                     kodiBase,
        const char*               aImageFile,
        const char*               aCaption,
        const char*               aDescription,
        unsigned int              displayTime,
        bool                      withSound,
        unsigned int              messageTime)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || aImageFile == nullptr || aCaption == nullptr  || aDescription == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - called with a null pointer", __FUNCTION__);
    return;
  }

  CGUIDialogKaiToast::QueueNotification(aImageFile, aCaption, aDescription, displayTime, withSound, messageTime);
}

} /* namespace ADDON */
