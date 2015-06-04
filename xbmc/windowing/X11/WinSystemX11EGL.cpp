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
#include "system.h"

#if defined(HAVE_X11)

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "WinSystemX11EGL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "Application.h"
#include "windowing/egl/EGLWrapper.h"

CWinSystemX11EGL::CWinSystemX11EGL()
{
}

CWinSystemX11EGL::~CWinSystemX11EGL()
{
}

bool CWinSystemX11EGL::PresentRenderImpl(const CDirtyRegionList& dirty)
{
  if ((m_eglDisplay == EGL_NO_DISPLAY) || (m_eglSurface == EGL_NO_SURFACE))
    return false;

  eglSwapBuffers(m_eglDisplay, m_eglSurface);

  return true;
}

void CWinSystemX11EGL::SetVSyncImpl(bool enable)
{
  eglSwapInterval(m_eglDisplay, enable ? 1 : 0);
}

bool CWinSystemX11EGL::IsExtSupported(const char* extension)
{
  if(strncmp(extension, "EGL_", 4) != 0)
    return CRenderSystemGL::IsExtSupported(extension);

  std::string name;

  name  = " ";
  name += extension;
  name += " ";

  return m_eglext.find(name) != std::string::npos;
}

bool CWinSystemX11EGL::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  if(!CWinSystemX11::CreateNewWindow(name, fullScreen, res, userFunction))
    return false;

  std::string extensions = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
  m_eglext = std::string(" ") + extensions + " ";

  CLog::Log(LOGDEBUG, "EGL_EXTENSIONS:%s", m_eglext.c_str());

  return true;
}

bool CWinSystemX11EGL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_newGlContext = false;
  CWinSystemX11::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight, false, 0);

  if (m_newGlContext)
    g_application.ReloadSkin();

  return true;
}

bool CWinSystemX11EGL::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  m_newGlContext = false;
  CWinSystemX11::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGL::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);

  if (m_newGlContext)
    g_application.ReloadSkin();

  return true;
}

#endif
