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
#pragma once

#include "AddonDll.h"
#include "cores/AudioEngine/Interfaces/IAudioCallback.h"
#include "include/xbmc_vis_types.h"
#include "guilib/IRenderingCallback.h"
#include "utils/rfft.h"
#include "utils/Stopwatch.h"

#include <algorithm>
#include <map>
#include <list>
#include <memory>

#define AUDIO_BUFFER_SIZE 512 // MUST BE A POWER OF 2!!!
#define MAX_AUDIO_BUFFERS 16

class CCriticalSection;

typedef DllAddon<Visualisation, VIS_PROPS> DllVisualisation;

class CAudioBuffer
{
public:
  CAudioBuffer(int iSize);
  virtual ~CAudioBuffer();
  const float* Get() const;
  void Set(const float* psBuffer, int iSize);
private:
  CAudioBuffer();
  float* m_pBuffer;
  int m_iLen;
};

namespace ADDON
{
  class CVisualisation : public CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>
                       , public IAudioCallback
                       , public IRenderingCallback
  {
  public:
    CVisualisation(const ADDON::AddonProps &props) : CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>(props) {}
    CVisualisation(const cp_extension_t *ext) : CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>(ext) {}
    virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
    virtual void OnAudioData(const float* pAudioData, int iAudioDataLength);
    bool Create();
    virtual bool Create(int x, int y, int w, int h, float pixelRatio);
    void Start(int x, int y, int w, int h, float pixelRatio,
               int iChannels, int iSamplesPerSec, int iBitsPerSample, const std::string &strSongName);
    void AudioData(const float *pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
    void Render();
    void Stop();
    void GetInfo(VIS_INFO *info);
    bool OnAction(VIS_ACTION action, void *param = NULL);
    bool UpdateTrack();
    bool HasPresets() { return m_hasPresets; };
    bool HasSubModules() { return !m_submodules.empty(); }
    bool IsLocked();
    unsigned GetPreset();
    std::string GetPresetName();
    bool GetPresetList(std::vector<std::string>& vecpresets);
    bool GetSubModuleList(std::vector<std::string>& vecmodules);
    static std::string GetFriendlyName(const std::string& vis, const std::string& module);
    void Destroy();

  private:
    void CreateBuffers();
    void ClearBuffers();

    bool GetPresets();
    bool GetSubModules();

    // attributes of the viewport we render to
    int m_xPos;
    int m_yPos;
    int m_width;
    int m_height;

    // cached preset list
    std::vector<std::string> m_presets;
    // cached submodule list
    std::vector<std::string> m_submodules;
    int m_currentModule;

    // audio properties
    int m_iChannels;
    int m_iSamplesPerSec;
    int m_iBitsPerSample;
    std::list<CAudioBuffer*> m_vecBuffers;
    int m_iNumBuffers;        // Number of Audio buffers
    bool m_bWantsFreq;
    float m_fFreq[AUDIO_BUFFER_SIZE];         // Frequency data
    bool m_bCalculate_Freq;       // True if the vis wants freq data
    bool m_hasPresets;
    std::unique_ptr<RFFT> m_transform;

    // track information
    std::string m_AlbumThumb;
  };

  typedef std::shared_ptr<CVisualisation> VisualisationPtr;

  //! \brief Class managing visualisation instances.
  class CVisualisationManager
  {
  public:
    //! \brief Singleton accessor
    static CVisualisationManager& GetInstance();

    //! \brief Obtain an addon instance.
    //! \param id ID of the addon to obtain.
    //! \param slot Slot to put addon in.
    VisualisationPtr GetAddon(const std::string& id, int slot=0);

    //! \brief Release an addon.
    //! \param slot Slot to release.
    void Release(int slot);

    //! \brief Clear out instances which have been idle for more than 30 secs.
    void ClearOutIdle();
  protected:
    //! \brief Protected construction. Use singleton accessor.
    CVisualisationManager();

    //! \brief Mapsslot -> (addon, idle timer).
    std::map<int, std::pair<VisualisationPtr, CStopWatch>> m_addons;
  };
}
