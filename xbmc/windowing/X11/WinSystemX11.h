#ifndef WINDOW_SYSTEM_X11_H
#define WINDOW_SYSTEM_X11_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system_gl.h"
#include <GL/glx.h>

#include "windowing/WinSystem.h"
#include "utils/Stopwatch.h"
#include "threads/CriticalSection.h"

class IDispResource;

class CWinSystemX11 : public CWinSystemBase
{
public:
  CWinSystemX11();
  virtual ~CWinSystemX11();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();
  virtual int  GetNumScreens() { return 1; }
  virtual int  GetCurrentScreen() { return m_nScreen; }
  virtual void ShowOSMouse(bool show);
  virtual void ResetOSScreensaver();
  virtual void EnableSystemScreenSaver(bool bEnable);

  virtual void NotifyAppActiveChange(bool bActivated);
  virtual void NotifyAppFocusChange(bool bGaining);

  virtual bool Minimize();
  virtual bool Restore() ;
  virtual bool Hide();
  virtual bool Show(bool raise = true);
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);
  virtual bool HasCalibration(const RESOLUTION_INFO &resInfo);

  // Local to WinSystemX11 only
  Display*  GetDisplay() { return m_dpy; }
  GLXWindow GetWindow() { return m_glWindow; }
  GLXContext GetGlxContext() { return m_glContext; }
  void NotifyXRREvent();
  void GetConnectedOutputs(std::vector<CStdString> *outputs);
  bool IsCurrentOutput(CStdString output);
  void NotifyMouseCoverage(bool covered);

protected:
  bool RefreshGlxContext();
#if defined(HAS_SDL_VIDEO_X11)
  void CheckDisplayEvents();
#endif
  void OnLostDevice();
  bool SetWindow(int width, int height, bool fullscreen, const CStdString &output);

#if defined(HAS_SDL_VIDEO_X11)
  SDL_Surface* m_SDLSurface;
  GLXWindow    m_glWindow;
#else
  Window       m_glWindow;
#endif
  Window       m_wmWindow;
  GLXContext   m_glContext;
  Display*     m_dpy;
  Cursor       m_invisibleCursor;
  Pixmap       m_icon;
  bool         m_bIsRotated;
  bool         m_bWasFullScreenBeforeMinimize;
  bool         m_bIgnoreNextFocusMessage;
  int          m_RREventBase;
  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  uint64_t                     m_dpyLostTime;
  CStdString                   m_currentOutput;
  bool                         m_initDone;
  bool                         m_windowDirty;
  bool                         m_bIsInternalXrr;
  bool                         m_newGlContext;

private:
  bool IsSuitableVisual(XVisualInfo *vInfo);
  static int XErrorHandler(Display* dpy, XErrorEvent* error);
  bool CreateIconPixmap();

  CStopWatch m_screensaverReset;
};

#endif // WINDOW_SYSTEM_H

