/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "GLContextGLX.h"
#include "utils/log.h"

CGLContextGLX::CGLContextGLX(Display *dpy) : CGLContext(dpy)
{
  m_extPrefix = "GLX_";
}

bool CGLContextGLX::Refresh(bool force, int screen, Window glWindow, bool &newContext)
{
  // refresh context
  bool retval = false;

  return retval;
}

void CGLContextGLX::Destroy()
{

}

void CGLContextGLX::Detach()
{

}

bool CGLContextGLX::IsSuitableVisual(XVisualInfo *vInfo)
{

  return true;
}

void CGLContextGLX::SetVSync(bool enable)
{

}

bool CGLContextGLX::SwapBuffers(const CDirtyRegionList& dirty)
{

  return true;
}

void CGLContextGLX::QueryExtensions()
{

  CLog::Log(LOGDEBUG, "GLX_EXTENSIONS:%s", m_extensions.c_str());
}

bool CGLContextGLX::IsExtSupported(const char* extension)
{
  std::string name;

  name  = " ";
  name += extension;
  name += " ";

  return m_extensions.find(name) != std::string::npos;
}

#endif
