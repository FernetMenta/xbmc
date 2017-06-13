#pragma once
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

#include <memory>
#include <vector>

#include "addons/AddonInfo.h"
#include "addons/settings/AddonSettings.h"
#include "addons/AddonVersion.h"
#include "utils/XBMCTinyXML.h"
#include "guilib/LocalizeStrings.h"
#include "utils/ISerializable.h"


class TiXmlElement;
class CAddonCallbacksAddon;
class CVariant;
class CAddonSettings;

typedef struct cp_plugin_info_t cp_plugin_info_t;
typedef struct cp_extension_t cp_extension_t;

namespace ADDON
{
  class CAddon;
  typedef std::shared_ptr<CAddon> AddonPtr;
  class CSkinInfo;
  typedef std::shared_ptr<CSkinInfo> SkinPtr;

  typedef std::vector<AddonPtr> VECADDONS;
  typedef std::vector<AddonPtr>::iterator IVECADDONS;

  const char* const ORIGIN_SYSTEM = "b6a50484-93a0-4afb-a01c-8d17e059feda";

void OnEnabled(const std::string& id);
void OnDisabled(const std::string& id);
void OnPreInstall(const AddonPtr& addon);
void OnPostInstall(const AddonPtr& addon, bool update, bool modal);
void OnPreUnInstall(const AddonPtr& addon);
void OnPostUnInstall(const AddonPtr& addon);

class CAddon : public std::enable_shared_from_this<CAddon>
{
public:
  explicit CAddon(AddonInfoPtr addonInfo);
  virtual ~CAddon() {}

  virtual TYPE MainType() const { return m_addonInfo->MainType(); }
  virtual std::string MainLibPath() const { return m_addonInfo->MainLibPath(); }

  virtual bool IsType(TYPE type) const { return m_addonInfo->IsType(type); }
  virtual const CAddonType* Type(TYPE type) const { return m_addonInfo->Type(type); }

  virtual std::string ID() const { return m_addonInfo->ID(); }
  virtual std::string Name() const { return m_addonInfo->Name(); }
  virtual bool IsInUse() const { return false; };
  virtual AddonVersion Version() const { return m_addonInfo->Version(); }
  virtual AddonVersion MinVersion() const { return m_addonInfo->MinVersion(); }
  virtual std::string Summary() const { return m_addonInfo->Summary(); }
  virtual std::string Description() const { return m_addonInfo->Description(); }
  virtual std::string Path() const { return m_addonInfo->Path(); }
  virtual std::string Profile() const { return m_profilePath; }
  virtual std::string Author() const { return m_addonInfo->Author(); }
  virtual std::string ChangeLog() const { return m_addonInfo->ChangeLog(); }
  virtual std::string Icon() const { return m_addonInfo->Icon(); };
  virtual ArtMap Art() const { return m_addonInfo->Art(); };
  virtual std::string FanArt() const { return m_addonInfo->FanArt(); }
  virtual std::vector<std::string> Screenshots() const { return m_addonInfo->Screenshots(); };
  virtual std::string Disclaimer() const { return m_addonInfo->Disclaimer(); }
  virtual std::string Broken() const { return m_addonInfo->Broken(); }
  virtual CDateTime InstallDate() const { return m_addonInfo->InstallDate(); }
  virtual CDateTime LastUpdated() const { return m_addonInfo->LastUpdated(); }
  virtual CDateTime LastUsed() const { return m_addonInfo->LastUsed(); }
  virtual std::string Origin() const { return m_addonInfo->Origin(); }
  virtual uint64_t PackageSize() const { return m_addonInfo->PackageSize(); }
  virtual const ADDONDEPS& GetDeps() const { return m_addonInfo->GetDeps(); }

  /*! \brief Check whether the this addon can be configured or not
   \return true if the addon has settings, false otherwise
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual bool HasSettings();

  /*! \brief Check whether the user has configured this addon or not
   \return true if previously saved settings are found, false otherwise
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, GetSetting, UpdateSetting
   */
  virtual bool HasUserSettings();

  /*! \brief Save any user configured settings
   \sa LoadSettings, LoadUserSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual void SaveSettings();

  /*! \brief Update a user-configured setting with a new value
   \param key the id of the setting to update
   \param value the value that the setting should take
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  virtual void UpdateSetting(const std::string& key, const std::string& value);

  /*! \brief Update a user-configured setting with a new boolean value
  \param key the id of the setting to update
  \param value the value that the setting should take
  \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
  */
  virtual bool UpdateSettingBool(const std::string& key, bool value);

  /*! \brief Update a user-configured setting with a new integer value
  \param key the id of the setting to update
  \param value the value that the setting should take
  \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
  */
  virtual bool UpdateSettingInt(const std::string& key, int value);

  /*! \brief Update a user-configured setting with a new number value
  \param key the id of the setting to update
  \param value the value that the setting should take
  \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
  */
  virtual bool UpdateSettingNumber(const std::string& key, double value);

  /*! \brief Update a user-configured setting with a new string value
  \param key the id of the setting to update
  \param value the value that the setting should take
  \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
  */
  virtual bool UpdateSettingString(const std::string& key, const std::string& value);

  /*! \brief Retrieve a particular settings value
   If a previously configured user setting is available, we return it's value, else we return the default (if available)
   \param key the id of the setting to retrieve
   \return the current value of the setting, or the default if the setting has yet to be configured.
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  virtual std::string GetSetting(const std::string& key);

  /*! \brief Retrieve a particular settings value as boolean
  If a previously configured user setting is available, we return it's value, else we return the default (if available)
  \param key the id of the setting to retrieve
  \param value the current value of the setting, or the default if the setting has yet to be configured
  \return true if the setting's value was retrieved, false otherwise.
  \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
  */
  virtual bool GetSettingBool(const std::string& key, bool& value);

  /*! \brief Retrieve a particular settings value as integer
  If a previously configured user setting is available, we return it's value, else we return the default (if available)
  \param key the id of the setting to retrieve
  \param value the current value of the setting, or the default if the setting has yet to be configured
  \return true if the setting's value was retrieved, false otherwise.
  \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
  */
  virtual bool GetSettingInt(const std::string& key, int& value);

  /*! \brief Retrieve a particular settings value as number
  If a previously configured user setting is available, we return it's value, else we return the default (if available)
  \param key the id of the setting to retrieve
  \param value the current value of the setting, or the default if the setting has yet to be configured
  \return true if the setting's value was retrieved, false otherwise.
  \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
  */
  virtual bool GetSettingNumber(const std::string& key, double& value);

  /*! \brief Retrieve a particular settings value as string
  If a previously configured user setting is available, we return it's value, else we return the default (if available)
  \param key the id of the setting to retrieve
  \param value the current value of the setting, or the default if the setting has yet to be configured
  \return true if the setting's value was retrieved, false otherwise.
  \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
  */
  virtual bool GetSettingString(const std::string& key, std::string& value);

  virtual CAddonSettings* GetSettings() const;

  /*! \brief get the required version of a dependency.
   \param dependencyID the addon ID of the dependency.
   \return the version this addon requires.
   */
  virtual AddonVersion GetDependencyVersion(const std::string &dependencyID) const;

  /*! \brief return whether or not this addon satisfies the given version requirements
   \param version the version to meet.
   \return true if  min_version <= version <= current_version, false otherwise.
   */
  virtual bool MeetsVersion(const AddonVersion &version) const { return m_addonInfo->MeetsVersion(version); }
  virtual bool ReloadSettings();

  /*! \brief callback for when this add-on is disabled.
   Use to perform any needed actions (e.g. stop a service)
   */
  virtual void OnDisabled() {};

  /*! \brief callback for when this add-on is enabled.
   Use to perform any needed actions (e.g. start a service)
   */
  virtual void OnEnabled() {};

  /*! \brief retrieve the running instance of an add-on if it persists while running.
   */
  virtual AddonPtr GetRunningInstance() const { return AddonPtr(); }

  virtual void OnPreInstall() {};
  virtual void OnPostInstall(bool update, bool modal) {};
  virtual void OnPreUnInstall() {};
  virtual void OnPostUnInstall() {};

  const AddonInfoPtr AddonInfo() const { return m_addonInfo; }

protected:
  /*! \brief Whether or not the settings have been initialized. */
  virtual bool SettingsInitialized() const;

  /*! \brief Whether or not the settings have been loaded. */
  virtual bool SettingsLoaded() const;

  /*! \brief Load the default settings and override these with any previously configured user settings
   \param bForce force the load of settings even if they are already loaded (reload)
   \return true if settings exist, false otherwise
   \sa LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  bool LoadSettings(bool bForce);

  /*! \brief Load the user settings
   \return true if user settings exist, false otherwise
   \sa LoadSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual bool LoadUserSettings();

  /* \brief Whether there are settings to be saved
   \sa SaveSettings
   */
  virtual bool HasSettingsToSave() const;

  /*! \brief Parse settings from an XML document
   \param doc XML document to parse for settings
   \param loadDefaults if true, the default attribute is used and settings are reset prior to parsing, else the value attribute is used.
   \return true if settings are loaded, false otherwise
   \sa SettingsToXML
   */
  virtual bool SettingsFromXML(const CXBMCTinyXML &doc, bool loadDefaults = false);

  /*! \brief Write settings into an XML document
   \param doc XML document to receive the settings
   \return true if settings are saved, false otherwise
   \sa SettingsFromXML
   */
  virtual bool SettingsToXML(CXBMCTinyXML &doc) const;

  const AddonInfoPtr m_addonInfo;

private:
  bool m_loadSettingsFailed;
  bool m_hasUserSettings;

  std::string m_profilePath;
  std::string m_userSettingsPath;
  mutable std::shared_ptr<CAddonSettings> m_settings;
};

}; /* namespace ADDON */
