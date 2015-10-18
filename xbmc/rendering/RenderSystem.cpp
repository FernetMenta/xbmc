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

#include "RenderSystem.h"
#include "guilib/GUIImage.h"
#include "filesystem/File.h"

CRenderSystemBase::CRenderSystemBase()
  : CThread("Render")
  , m_stereoView(RENDER_STEREO_VIEW_OFF)
  , m_stereoMode(RENDER_STEREO_MODE_OFF)
{
  m_bRenderCreated = false;
  m_bVSync = true;
  m_maxTextureSize = 2048;
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;
  m_renderCaps = 0;
  m_renderQuirks = 0;
  m_minDXTPitch = 0;
}

CRenderSystemBase::~CRenderSystemBase()
{

}

void CRenderSystemBase::GetRenderVersion(unsigned int& major, unsigned int& minor) const
{
  major = m_RenderVersionMajor;
  minor = m_RenderVersionMinor;
}

bool CRenderSystemBase::SupportsNPOT(bool dxt) const
{
  if (dxt)
    return (m_renderCaps & RENDER_CAPS_DXT_NPOT) == RENDER_CAPS_DXT_NPOT;
  return (m_renderCaps & RENDER_CAPS_NPOT) == RENDER_CAPS_NPOT;
}

bool CRenderSystemBase::SupportsDXT() const
{
  return (m_renderCaps & RENDER_CAPS_DXT) == RENDER_CAPS_DXT;
}

bool CRenderSystemBase::SupportsBGRA() const
{
  return (m_renderCaps & RENDER_CAPS_BGRA) == RENDER_CAPS_BGRA;
}

bool CRenderSystemBase::SupportsBGRAApple() const
{
  return (m_renderCaps & RENDER_CAPS_BGRA_APPLE) == RENDER_CAPS_BGRA_APPLE;
}

bool CRenderSystemBase::SupportsStereo(RENDER_STEREO_MODE mode) const
{
  switch(mode)
  {
    case RENDER_STEREO_MODE_OFF:
    case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
    case RENDER_STEREO_MODE_SPLIT_VERTICAL:
    case RENDER_STEREO_MODE_MONO:
      return true;
    default:
      return false;
  }
}

void CRenderSystemBase::ShowSplash(std::string splashImage)
{
  CGUIImage *image;
  if (!XFILE::CFile::Exists(splashImage))
    splashImage = "special://xbmc/media/Splash.png";

  image = new CGUIImage(0, 0, 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), CTextureInfo(splashImage));
  image->SetAspectRatio(CAspectRatio::AR_SCALE);

  g_graphicsContext.Lock();
  g_graphicsContext.Clear();

  RESOLUTION_INFO res = g_graphicsContext.GetResInfo();
  g_graphicsContext.SetRenderingResolution(res, true);

  //render splash image
  BeginRender();

  image->AllocResources();
  image->Render();
  image->FreeResources();

  //show it on screen
  EndRender();
  CDirtyRegionList dirty;
  g_graphicsContext.Flip(dirty);
  g_graphicsContext.Unlock();

  delete image;
}

void CRenderSystemBase::Process()
{

}

void CRenderSystemBase::FrameMove()
{
//  CWinEvents::MessagePump();
//
//  if (m_renderGUI)
//  {
//    m_skipGuiRender = false;
//    int fps = 0;
//
//#if defined(TARGET_RASPBERRY_PI) || defined(HAS_IMXVPU)
//    // This code reduces rendering fps of the GUI layer when playing videos in fullscreen mode
//    // it makes only sense on architectures with multiple layers
//    if (g_graphicsContext.IsFullScreenVideo() && !m_pPlayer->IsPausedPlayback() && m_pPlayer->IsRenderingVideoLayer())
//      fps = CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_LIMITGUIUPDATE);
//#endif
//
//    unsigned int now = XbmcThreads::SystemClockMillis();
//    unsigned int frameTime = now - m_lastRenderTime;
//    if (fps > 0 && frameTime * fps < 1000)
//      m_skipGuiRender = true;
//
//    if (!m_bStop)
//    {
//      if (!m_skipGuiRender)
//        g_windowManager.Process(CTimeUtils::GetFrameTime());
//    }
//
//    g_windowManager.FrameMove();
//  }
//
//  m_pPlayer->FrameMove();
}

void CRenderSystemBase::Render()
{
//  int vsync_mode = CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOSCREEN_VSYNC);
//
//  bool hasRendered = false;
//  bool limitFrames = false;
//  unsigned int singleFrameTime = 10; // default limit 100 fps
//  bool vsync = true;
//
//  {
//    // Less fps in DPMS
//    bool lowfps = g_Windowing.EnableFrameLimiter();
//
//    m_bPresentFrame = false;
//    if (g_graphicsContext.IsFullScreenVideo() && !m_pPlayer->IsPausedPlayback())
//    {
//      m_bPresentFrame = true; //m_pPlayer->HasFrame();
//      if (vsync_mode == VSYNC_DISABLED)
//        vsync = false;
//    }
//    else
//    {
//      // engage the frame limiter as needed
//      limitFrames = lowfps || extPlayerActive;
//      // DXMERGE - we checked for g_videoConfig.GetVSyncMode() before this
//      //           perhaps allowing it to be set differently than the UI option??
//      if (vsync_mode == VSYNC_DISABLED || vsync_mode == VSYNC_VIDEO)
//      {
//        limitFrames = true; // not using vsync.
//        vsync = false;
//      }
//      else if ((g_infoManager.GetFPS() > g_graphicsContext.GetFPS() + 10) && g_infoManager.GetFPS() > 1000.0f / singleFrameTime)
//      {
//        limitFrames = true; // using vsync, but it isn't working.
//        vsync = false;
//      }
//
//      if (limitFrames)
//      {
//        if (0) //extPlayerActive)
//        {
//          ResetScreenSaver();  // Prevent screensaver dimming the screen
//          singleFrameTime = 1000;  // 1 fps, high wakeup latency but v.low CPU usage
//        }
//        else if (lowfps)
//          singleFrameTime = 200;  // 5 fps, <=200 ms latency to wake up
//      }
//
//    }
//  }
//
//  CSingleLock lock(g_graphicsContext);
//
//  if (g_graphicsContext.IsFullScreenVideo() && m_pPlayer->IsPlaying() && vsync_mode == VSYNC_VIDEO)
//    g_Windowing.SetVSync(true);
//  else if (vsync_mode == VSYNC_ALWAYS)
//    g_Windowing.SetVSync(true);
//  else if (vsync_mode != VSYNC_DRIVER)
//    g_Windowing.SetVSync(false);
//
//  if (m_bPresentFrame && m_pPlayer->IsPlaying() && !m_pPlayer->IsPaused())
//    ResetScreenSaver();
//
//  if(!g_Windowing.BeginRender())
//    return;
//
//  CDirtyRegionList dirtyRegions;
//
//  // render gui layer
//  if (!m_skipGuiRender)
//  {
//    dirtyRegions = g_windowManager.GetDirty();
//    if (g_graphicsContext.GetStereoMode())
//    {
//      g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_LEFT);
//      if (RenderNoPresent())
//        hasRendered = true;
//
//      if (g_graphicsContext.GetStereoMode() != RENDER_STEREO_MODE_MONO)
//      {
//        g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_RIGHT);
//        if (RenderNoPresent())
//          hasRendered = true;
//      }
//      g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_OFF);
//    }
//    else
//    {
//      if (RenderNoPresent())
//        hasRendered = true;
//    }
//    // execute post rendering actions (finalize window closing)
//    g_windowManager.AfterRender();
//  }
//
//  // render video layer
//  g_windowManager.RenderEx();
//
//  m_pPlayer->AfterRender();
//
//  g_Windowing.EndRender();
//
//  // reset our info cache - we do this at the end of Render so that it is
//  // fresh for the next process(), or after a windowclose animation (where process()
//  // isn't called)
//  g_infoManager.ResetCache();
//
//
//  unsigned int now = XbmcThreads::SystemClockMillis();
//  if (hasRendered)
//  {
//    g_infoManager.UpdateFPS();
//    m_lastRenderTime = now;
//  }
//
//  lock.Leave();
//
//  //when nothing has been rendered for m_guiDirtyRegionNoFlipTimeout milliseconds,
//  //we don't call g_graphicsContext.Flip() anymore, this saves gpu and cpu usage
//  bool flip;
//  if (g_advancedSettings.m_guiDirtyRegionNoFlipTimeout >= 0)
//    flip = hasRendered || (now - m_lastRenderTime) < (unsigned int)g_advancedSettings.m_guiDirtyRegionNoFlipTimeout;
//  else
//    flip = true;
//
//  //fps limiter, make sure each frame lasts at least singleFrameTime milliseconds
//  if (limitFrames || !(flip || m_bPresentFrame))
//  {
//    if (!limitFrames)
//      singleFrameTime = 40; //if not flipping, loop at 25 fps
//
//    unsigned int frameTime = now - m_lastFrameTime;
//    if (frameTime < singleFrameTime)
//      Sleep(singleFrameTime - frameTime);
//  }
//
//  if (flip)
//    PresentRender(dirtyRegions);
//
//  if (!extPlayerActive && g_graphicsContext.IsFullScreenVideo() && !m_pPlayer->IsPausedPlayback())
//  {
//    m_pPlayer->FrameWait(100);
//  }
//
//  m_lastFrameTime = XbmcThreads::SystemClockMillis();
//  CTimeUtils::UpdateFrameTime(flip, vsync);
}
