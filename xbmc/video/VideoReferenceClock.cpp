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
#include "system.h"
#include <list>
#include "utils/StdString.h"
#include "VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "threads/SingleLock.h"

#if defined(HAS_GLX) && defined(HAS_XRANDR)
  #include <sstream>
  #include <X11/extensions/Xrandr.h>
  #include "windowing/WindowingFactory.h"
  #include "settings/Settings.h"
  #include "guilib/GraphicContext.h"
  #define NVSETTINGSCMD "nvidia-settings -nt -q RefreshRate3"
#elif defined(__APPLE__) && !defined(__arm__)
  #include <QuartzCore/CVDisplayLink.h>
  #include "CocoaInterface.h"
#elif defined(__APPLE__) && defined(__arm__)
  #include "WindowingFactory.h"
#elif defined(_WIN32) && defined(HAS_DX)
  #pragma comment (lib,"d3d9.lib")
  #if (D3DX_SDK_VERSION >= 42) //aug 2009 sdk and up there is no dxerr9 anymore
    #include <Dxerr.h>
    #pragma comment (lib,"DxErr.lib")
  #else
    #include <dxerr9.h>
    #define DXGetErrorString(hr)      DXGetErrorString9(hr)
    #define DXGetErrorDescription(hr) DXGetErrorDescription9(hr)
    #pragma comment (lib,"Dxerr9.lib")
  #endif
  #include "windowing/WindowingFactory.h"
  #include "settings/AdvancedSettings.h"
#endif

#if defined(HAS_GLX)
void CDisplayCallback::OnLostDevice()
{
  CSingleLock lock(m_DisplaySection);
  m_State = DISP_LOST;
  m_DisplayEvent.Reset();
}
void CDisplayCallback::OnResetDevice()
{
  CSingleLock lock(m_DisplaySection);
  m_State = DISP_RESET;
  m_DisplayEvent.Set();
}
void CDisplayCallback::Register()
{
  CSingleLock lock(m_DisplaySection);
  g_Windowing.Register(this);
  m_State = DISP_OPEN;
}
void CDisplayCallback::Unregister()
{
  g_Windowing.Unregister(this);
}
bool CDisplayCallback::IsReset()
{
  DispState state;
  { CSingleLock lock(m_DisplaySection);
    state = m_State;
  }
  if (state == DISP_LOST)
  {
    CLog::Log(LOGDEBUG,"VideoReferenceClock - wait for display reset signal");
    m_DisplayEvent.Wait();
    CSingleLock lock(m_DisplaySection);
    state = m_State;
    CLog::Log(LOGDEBUG,"VideoReferenceClock - got display reset signal");
  }
  if (state == DISP_RESET)
  {
    m_State = DISP_OPEN;
    return true;
  }
  return false;
}
#endif

using namespace std;

#if defined(_WIN32) && defined(HAS_DX)

  void CD3DCallback::Reset()
  {
    m_devicevalid = true;
    m_deviceused = false;
  }

  void CD3DCallback::OnDestroyDevice()
  {
    CSingleLock lock(m_critsection);
    m_devicevalid = false;
    while (m_deviceused)
    {
      lock.Leave();
      m_releaseevent.Wait();
      lock.Enter();
    }
  }

  void CD3DCallback::OnCreateDevice()
  {
    CSingleLock lock(m_critsection);
    m_devicevalid = true;
    m_createevent.Set();
  }

  void CD3DCallback::Aquire()
  {
    CSingleLock lock(m_critsection);
    while(!m_devicevalid)
    {
      lock.Leave();
      m_createevent.Wait();
      lock.Enter();
    }
    m_deviceused = true;
  }

  void CD3DCallback::Release()
  {
    CSingleLock lock(m_critsection);
    m_deviceused = false;
    m_releaseevent.Set();
  }

  bool CD3DCallback::IsValid()
  {
    return m_devicevalid;
  }

#endif

CVideoReferenceClock::CVideoReferenceClock() : CThread("CVideoReferenceClock")
{
  m_SystemFrequency = CurrentHostFrequency();
  m_ClockSpeed = 1.0;
  m_ClockOffset = 0;
  m_TotalMissedVblanks = 0;
  m_UseVblank = false;
  m_Started.Reset();

#if defined(HAS_GLX) && defined(HAS_XRANDR)
  m_Dpy = NULL;
#endif
}

void CVideoReferenceClock::Process()
{
  bool SetupSuccess = false;
  int64_t Now;

#if defined(_WIN32) && defined(HAS_DX)
  //register callback
  m_D3dCallback.Reset();
  g_Windowing.Register(&m_D3dCallback);
#endif

  while(!m_bStop)
  {
    //set up the vblank clock
#if defined(HAS_GLX) && defined(HAS_XRANDR)
    SetupSuccess = SetupGLX();
#elif defined(_WIN32) && defined(HAS_DX)
    SetupSuccess = SetupD3D();
#elif defined(__APPLE__)
    SetupSuccess = SetupCocoa();
#elif defined(HAS_GLX)
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: compiled without RandR support");
#elif defined(_WIN32)
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: only available on directx build");
#else
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: no implementation available");
#endif

    CSingleLock SingleLock(m_CritSection);
    Now = CurrentHostCounter();
    m_CurrTime = Now + m_ClockOffset; //add the clock offset from the previous time we stopped
    m_LastIntTime = m_CurrTime;
    m_CurrTimeFract = 0.0;
    m_ClockSpeed = 1.0;
    m_TotalMissedVblanks = 0;
    m_fineadjust = 1.0;
    m_RefreshChanged = 0;
    m_Started.Set();

    SetPriority(1);

    if (SetupSuccess)
    {
      m_UseVblank = true;          //tell other threads we're using vblank as clock
      m_VblankTime = Now;          //initialize the timestamp of the last vblank
      SingleLock.Leave();

      //run the clock
#if defined(HAS_GLX) && defined(HAS_XRANDR)
      RunGLX();
#elif defined(_WIN32) && defined(HAS_DX)
      RunD3D();
#elif defined(__APPLE__)
      RunCocoa();
#endif

    }
    else
    {
      SingleLock.Leave();
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setup failed, falling back to CurrentHostCounter()");
    }

    SingleLock.Enter();
    m_UseVblank = false;                       //we're back to using the systemclock
    Now = CurrentHostCounter();                //set the clockoffset between the vblank clock and systemclock
    m_ClockOffset = m_CurrTime - Now;
    SingleLock.Leave();

    //clean up the vblank clock
#if defined(HAS_GLX) && defined(HAS_XRANDR)
    CleanupGLX();
#elif defined(_WIN32) && defined(HAS_DX)
    CleanupD3D();
#elif defined(__APPLE__)
    CleanupCocoa();
#endif
    if (!SetupSuccess) break;
  }

#if defined(_WIN32) && defined(HAS_DX)
  g_Windowing.Unregister(&m_D3dCallback);
#endif
}

bool CVideoReferenceClock::WaitStarted(int MSecs)
{
  //not waiting here can cause issues with alsa
  return m_Started.WaitMSec(MSecs);
}

#if defined(HAS_GLX) && defined(HAS_XRANDR)
bool CVideoReferenceClock::SetupGLX()
{
  int singleBufferAttributes[] = {
    GLX_RGBA,
    GLX_RED_SIZE,      0,
    GLX_GREEN_SIZE,    0,
    GLX_BLUE_SIZE,     0,
    GLX_DOUBLEBUFFER,
    None
  };

  int ReturnV, SwaMask;
  unsigned int GlxTest;
  XSetWindowAttributes Swa;

  m_vInfo = NULL;
  m_Context = NULL;
  m_Window = 0;

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setting up GLX");

  if (!m_Dpy)
  {
    m_Dpy = XOpenDisplay(NULL);
    if (!m_Dpy)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Unable to open display");
      return false;
    }
  }

  if (!glXQueryExtension(m_Dpy, NULL, NULL))
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: X server does not support GLX");
    return false;
  }

  bool          ExtensionFound = false;
  istringstream Extensions(glXQueryExtensionsString(m_Dpy, DefaultScreen(m_Dpy)));
  string        ExtensionStr;

  while (!ExtensionFound)
  {
    Extensions >> ExtensionStr;
    if (Extensions.fail())
      break;

    if (ExtensionStr == "GLX_SGI_video_sync")
      ExtensionFound = true;
  }

  if (!ExtensionFound)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: X server does not support GLX_SGI_video_sync");
    return false;
  }

  m_vInfo = glXChooseVisual(m_Dpy, DefaultScreen(m_Dpy), singleBufferAttributes);
  if (!m_vInfo)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXChooseVisual returned NULL");
    return false;
  }

  Swa.border_pixel = 0;
  Swa.event_mask = StructureNotifyMask;
  Swa.colormap = XCreateColormap(m_Dpy, RootWindow(m_Dpy, m_vInfo->screen), m_vInfo->visual, AllocNone );
  SwaMask = CWBorderPixel | CWColormap | CWEventMask;

  m_Window = XCreateWindow(m_Dpy, RootWindow(m_Dpy, m_vInfo->screen), 0, 0, 256, 256, 0,
                           m_vInfo->depth, InputOutput, m_vInfo->visual, SwaMask, &Swa);

  m_Context = glXCreateContext(m_Dpy, m_vInfo, NULL, True);
  if (!m_Context)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXCreateContext returned NULL");
    return false;
  }

  ReturnV = glXMakeCurrent(m_Dpy, m_Window, m_Context);
  if (ReturnV != True)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
    return false;
  }

  m_glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
  if (!m_glXWaitVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI not found");
    return false;
  }

  ReturnV = m_glXWaitVideoSyncSGI(2, 0, &GlxTest);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI returned %i", ReturnV);
    return false;
  }

  m_glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  if (!m_glXGetVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI not found");
    return false;
  }

  ReturnV = m_glXGetVideoSyncSGI(&GlxTest);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI returned %i", ReturnV);
    return false;
  }

  m_glXSwapIntervalMESA = NULL;
  m_glXSwapIntervalMESA = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");

  m_DispCallback.Register();

  UpdateRefreshrate(true); //forced refreshrate update
  m_MissedVblanks = 0;

  return true;
}

void CVideoReferenceClock::CleanupGLX()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cleaning up GLX");

  m_DispCallback.Unregister();

  bool AtiWorkaround = false;
  const char* VendorPtr = (const char*)glGetString(GL_VENDOR);
  if (VendorPtr)
  {
    CStdString Vendor = VendorPtr;
    Vendor.ToLower();
    if (Vendor.compare(0, 3, "ati") == 0)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: GL_VENDOR: %s, using ati dpy workaround", VendorPtr);
      AtiWorkaround = true;
    }
  }

  if (m_vInfo)
  {
    XFree(m_vInfo);
    m_vInfo = NULL;
  }
  if (m_Context)
  {
    glXMakeCurrent(m_Dpy, None, NULL);
    glXDestroyContext(m_Dpy, m_Context);
    m_Context = NULL;
  }
  if (m_Window)
  {
    XDestroyWindow(m_Dpy, m_Window);
    m_Window = 0;
  }

  //ati saves the Display* in their libGL, if we close it here, we crash
  if (m_Dpy && !AtiWorkaround)
  {
    XCloseDisplay(m_Dpy);
    m_Dpy = NULL;
  }
}

void CVideoReferenceClock::RunGLX()
{
  unsigned int  PrevVblankCount;
  unsigned int  VblankCount;
  int           ReturnV;
  bool          IsReset = false;
  int64_t       Now;

  bool AtiWorkaround = false;
  const char* VendorPtr = (const char*)glGetString(GL_VENDOR);
  if (VendorPtr)
  {
    CStdString Vendor = VendorPtr;
    Vendor.ToLower();
    if ((Vendor.compare(0, 3, "ati") == 0) && m_glXSwapIntervalMESA)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: GL_VENDOR: %s, using ati workaround", VendorPtr);
      AtiWorkaround = true;
    }
  }

  CSingleLock SingleLock(m_CritSection);
  SingleLock.Leave();

  //get the current vblank counter
  m_glXGetVideoSyncSGI(&VblankCount);
  PrevVblankCount = VblankCount;

  int precision = 1;
  int proximity;
  uint64_t lastVblankTime = CurrentHostCounter();
  int sleepTime, correction;

  while(!m_bStop)
  {
    //wait for the next vblank
    if (!AtiWorkaround)
      ReturnV = m_glXWaitVideoSyncSGI(2, (VblankCount + 1) % 2, &VblankCount);
    else
    {
      proximity = 0;

      // calculate sleep time in micro secs
      // we start with 10% of interval multiplied with precision
      sleepTime = m_SystemFrequency / m_RefreshRate / 10000LL * precision;

      // correct sleepTime
      correction = (CurrentHostCounter() - lastVblankTime) / m_SystemFrequency * 1000000LL;
      if (sleepTime > correction)
        sleepTime -= correction;
      usleep(sleepTime);
      m_glXGetVideoSyncSGI(&VblankCount);
      if (VblankCount == PrevVblankCount)
      {
        usleep(sleepTime/2);
        m_glXGetVideoSyncSGI(&VblankCount);
        while (VblankCount == PrevVblankCount)
        {
          usleep(sleepTime/20);
          m_glXGetVideoSyncSGI(&VblankCount);
          proximity++;
        }
      }
      else if (precision > 1)
        precision--;

      if (proximity > 4 && precision < 6)
        precision++;

//      CLog::Log(LOGNOTICE, "----- diff: %ld, precision: %d, prox: %d, sleep: %d, correction: %d",
//          CurrentHostCounter()- lastVblankTime, precision, proximity, sleepTime, correction);

      lastVblankTime = CurrentHostCounter();

      ReturnV = 0;
    }

    m_glXGetVideoSyncSGI(&VblankCount); //the vblank count returned by glXWaitVideoSyncSGI is not always correct
    Now = CurrentHostCounter();         //get the timestamp of this vblank

    if(ReturnV)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI returned %i", ReturnV);
      return;
    }

    if (m_DispCallback.IsReset())
      UpdateRefreshrate(true);

    if (VblankCount > PrevVblankCount)
    {
      //update the vblank timestamp, update the clock and send a signal that we got a vblank
      SingleLock.Enter();
      m_VblankTime = Now;
      UpdateClock((int)(VblankCount - PrevVblankCount), true);
      SingleLock.Leave();
      SendVblankSignal();
      IsReset = false;
    }
    else
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Vblank counter has reset");

      precision = 1;

      //only try reattaching once
      if (IsReset)
        return;

      //because of a bug in the nvidia driver, glXWaitVideoSyncSGI breaks when the vblank counter resets
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detaching glX context");
      ReturnV = glXMakeCurrent(m_Dpy, None, NULL);
      if (ReturnV != True)
      {
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
        return;
      }

      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Attaching glX context");
      ReturnV = glXMakeCurrent(m_Dpy, m_Window, m_Context);
      if (ReturnV != True)
      {
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
        return;
      }

      m_glXGetVideoSyncSGI(&VblankCount);

      IsReset = true;
    }
    PrevVblankCount = VblankCount;
  }
}

#elif defined(_WIN32) && defined(HAS_DX)

void CVideoReferenceClock::RunD3D()
{
  D3DRASTER_STATUS RasterStatus;
  int64_t       Now;
  int64_t       LastVBlankTime;
  unsigned int  LastLine;
  int           NrVBlanks;
  double        VBlankTime;
  int           ReturnV;

  CSingleLock SingleLock(m_CritSection);
  SingleLock.Leave();

  //get the scanline we're currently at
  m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (RasterStatus.InVBlank) LastLine = 0;
  else LastLine = RasterStatus.ScanLine;

  //init the vblanktime
  Now = CurrentHostCounter();
  LastVBlankTime = Now;

  while(!m_bStop && m_D3dCallback.IsValid())
  {
    //get the scanline we're currently at
    ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned returned %s: %s",
                DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
      return;
    }

    //if InVBlank is set, or the current scanline is lower than the previous scanline, a vblank happened
    if ((RasterStatus.InVBlank && LastLine > 0) || (RasterStatus.ScanLine < LastLine))
    {
      //calculate how many vblanks happened
      Now = CurrentHostCounter();
      VBlankTime = (double)(Now - LastVBlankTime) / (double)m_SystemFrequency;
      NrVBlanks = MathUtils::round_int(VBlankTime * (double)m_RefreshRate);

      //update the vblank timestamp, update the clock and send a signal that we got a vblank
      SingleLock.Enter();
      m_VblankTime = Now;
      UpdateClock(NrVBlanks, true);
      SingleLock.Leave();
      SendVblankSignal();

      if (UpdateRefreshrate())
      {
        //we have to measure the refreshrate again
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: Displaymode changed");
        return;
      }

      //save the timestamp of this vblank so we can calculate how many vblanks happened next time
      LastVBlankTime = Now;

      //because we had a vblank, sleep until half the refreshrate period
      Now = CurrentHostCounter();
      int SleepTime = (int)((LastVBlankTime + (m_SystemFrequency / m_RefreshRate / 2) - Now) * 1000 / m_SystemFrequency);
      if (SleepTime > 100) SleepTime = 100; //failsafe
      if (SleepTime > 0) ::Sleep(SleepTime);
    }
    else
    {
      ::Sleep(1);
    }

    if (RasterStatus.InVBlank) LastLine = 0;
    else LastLine = RasterStatus.ScanLine;
  }
}

//how many times we measure the refreshrate
#define NRMEASURES 6
//how long to measure in milliseconds
#define MEASURETIME 250

bool CVideoReferenceClock::SetupD3D()
{
  int ReturnV;

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setting up Direct3d");

  m_D3dCallback.Aquire();

  //get d3d device
  m_D3dDev = g_Windowing.Get3DDevice();

  //we need a high priority thread to get accurate timing
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: SetThreadPriority failed");

  D3DCAPS9 DevCaps;
  ReturnV = m_D3dDev->GetDeviceCaps(&DevCaps);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetDeviceCaps returned %s: %s",
                         DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  if ((DevCaps.Caps & D3DCAPS_READ_SCANLINE) != D3DCAPS_READ_SCANLINE)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Hardware does not support GetRasterStatus");
    return false;
  }

  D3DRASTER_STATUS RasterStatus;
  ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned returned %s: %s",
              DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  D3DDISPLAYMODE DisplayMode;
  ReturnV = m_D3dDev->GetDisplayMode(0, &DisplayMode);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetDisplayMode returned returned %s: %s",
              DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  //forced update of windows refreshrate
  UpdateRefreshrate(true);

  if (g_advancedSettings.m_measureRefreshrate)
  {
    //measure the refreshrate a couple times
    list<double> Measures;
    for (int i = 0; i < NRMEASURES; i++)
      Measures.push_back(MeasureRefreshrate(MEASURETIME));

    //build up a string of measured rates
    CStdString StrRates;
    for (list<double>::iterator it = Measures.begin(); it != Measures.end(); it++)
      StrRates.AppendFormat("%.2f ", *it);

    //get the top half of the measured rates
    Measures.sort();
    double RefreshRate = 0.0;
    int    NrMeasurements = 0;
    while (NrMeasurements < NRMEASURES / 2 && !Measures.empty())
    {
      if (Measures.back() > 0.0)
      {
        RefreshRate += Measures.back();
        NrMeasurements++;
      }
      Measures.pop_back();
    }

    if (NrMeasurements < NRMEASURES / 2)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: refreshrate measurements: %s, unable to get a good measurement",
        StrRates.c_str(), m_RefreshRate);
      return false;
    }

    RefreshRate /= NrMeasurements;
    m_RefreshRate = MathUtils::round_int(RefreshRate);

    CLog::Log(LOGDEBUG, "CVideoReferenceClock: refreshrate measurements: %s, assuming %i hertz", StrRates.c_str(), m_RefreshRate);
  }
  else
  {
    m_RefreshRate = m_PrevRefreshRate;
    if (m_RefreshRate == 23 || m_RefreshRate == 29 || m_RefreshRate == 59)
      m_RefreshRate++;

    if (m_Interlaced)
    {
      m_RefreshRate *= 2;
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: display is interlaced");
    }

    CLog::Log(LOGDEBUG, "CVideoReferenceClock: detected refreshrate: %i hertz, assuming %i hertz", m_PrevRefreshRate, (int)m_RefreshRate);
  }

  m_MissedVblanks = 0;

  return true;
}

double CVideoReferenceClock::MeasureRefreshrate(int MSecs)
{
  D3DRASTER_STATUS RasterStatus;
  int64_t          Now;
  int64_t          Target;
  int64_t          Prev;
  int64_t          AvgInterval;
  int64_t          MeasureCount;
  unsigned int     LastLine;
  int              ReturnV;

  Now = CurrentHostCounter();
  Target = Now + (m_SystemFrequency * MSecs / 1000);
  Prev = -1;
  AvgInterval = 0;
  MeasureCount = 0;

  //start measuring vblanks
  LastLine = 0;
  while(Now <= Target)
  {
    ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
    Now = CurrentHostCounter();
    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned returned %s: %s",
                DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
      return -1.0;
    }

    if ((RasterStatus.InVBlank && LastLine != 0) || (!RasterStatus.InVBlank && RasterStatus.ScanLine < LastLine))
    { //we got a vblank
      if (Prev != -1) //need two for a measurement
      {
        AvgInterval += Now - Prev; //save how long this vblank lasted
        MeasureCount++;
      }
      Prev = Now; //save this time for the next measurement
    }

    //save the current scanline
    if (RasterStatus.InVBlank)
      LastLine = 0;
    else
      LastLine = RasterStatus.ScanLine;

    ::Sleep(1);
  }

  if (MeasureCount < 1)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Didn't measure any vblanks");
    return -1.0;
  }

  double fRefreshRate = 1.0 / ((double)AvgInterval / (double)MeasureCount / (double)m_SystemFrequency);

  return fRefreshRate;
}

void CVideoReferenceClock::CleanupD3D()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cleaning up Direct3d");
  m_D3dCallback.Release();
}

#elif defined(__APPLE__)
#if !defined(__arm__)
// Called by the Core Video Display Link whenever it's appropriate to render a frame.
static CVReturn DisplayLinkCallBack(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
  double fps = 60.0;

  if (inNow->videoRefreshPeriod > 0)
    fps = (double)inOutputTime->videoTimeScale / (double)inOutputTime->videoRefreshPeriod;

  // Create an autorelease pool (necessary to call into non-Obj-C code from Obj-C code)
  void* pool = Cocoa_Create_AutoReleasePool();

  CVideoReferenceClock *VideoReferenceClock = reinterpret_cast<CVideoReferenceClock*>(displayLinkContext);
  VideoReferenceClock->VblankHandler(inNow->hostTime, fps);

  // Destroy the autorelease pool
  Cocoa_Destroy_AutoReleasePool(pool);

  return kCVReturnSuccess;
}
#endif
bool CVideoReferenceClock::SetupCocoa()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: setting up Cocoa");

  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  m_MissedVblanks = 0;
  m_RefreshRate = 60;              //init the refreshrate so we don't get any division by 0 errors

  #if defined(__arm__)
  {
    g_Windowing.InitDisplayLink();
  }
  #else
  if (!Cocoa_CVDisplayLinkCreate((void*)DisplayLinkCallBack, reinterpret_cast<void*>(this)))
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cocoa_CVDisplayLinkCreate failed");
    return false;
  }
  else
  #endif
  {
    UpdateRefreshrate(true);
    return true;
  }
}

void CVideoReferenceClock::RunCocoa()
{
  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  while(!m_bStop)
  {
    Sleep(1000);
  }
}

void CVideoReferenceClock::CleanupCocoa()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: cleaning up Cocoa");
  #if defined(__arm__)
    g_Windowing.DeinitDisplayLink();
  #else
    Cocoa_CVDisplayLinkRelease();
  #endif
}

void CVideoReferenceClock::VblankHandler(int64_t nowtime, double fps)
{
  int           NrVBlanks;
  double        VBlankTime;
  int           RefreshRate = MathUtils::round_int(fps);

  CSingleLock SingleLock(m_CritSection);

  if (RefreshRate != m_RefreshRate)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %f hertz, rounding to %i hertz", fps, RefreshRate);
    m_RefreshRate = RefreshRate;
  }
  m_LastRefreshTime = m_CurrTime;

  //calculate how many vblanks happened
  VBlankTime = (double)(nowtime - m_LastVBlankTime) / (double)m_SystemFrequency;
  NrVBlanks = MathUtils::round_int(VBlankTime * (double)m_RefreshRate);

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;

  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  m_VblankTime = nowtime;
  UpdateClock(NrVBlanks, true);

  SingleLock.Leave();

  SendVblankSignal();
  UpdateRefreshrate();
}
#endif

//this is called from the vblank run function and from CVideoReferenceClock::Wait in case of a late update
void CVideoReferenceClock::UpdateClock(int NrVBlanks, bool CheckMissed)
{
  if (CheckMissed) //set to true from the vblank run function, set to false from Wait and GetTime
  {
    if (NrVBlanks < m_MissedVblanks) //if this is true the vblank detection in the run function is wrong
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: detected %i vblanks, missed %i, refreshrate might have changed",
                NrVBlanks, m_MissedVblanks);

    NrVBlanks -= m_MissedVblanks; //subtract the vblanks we missed
    m_MissedVblanks = 0;
  }
  else
  {
    m_MissedVblanks += NrVBlanks;      //tell the vblank clock how many vblanks it missed
    m_TotalMissedVblanks += NrVBlanks; //for the codec information screen
    m_VblankTime += m_SystemFrequency * (int64_t)NrVBlanks / m_RefreshRate; //set the vblank time forward
  }

  if (NrVBlanks > 0) //update the clock with the adjusted frequency if we have any vblanks
  {
    double increment = UpdateInterval() * NrVBlanks;
    double integer   = floor(increment);
    m_CurrTime      += (int64_t)(integer + 0.5); //make sure it gets correctly converted to int

    //accumulate what we lost due to rounding in m_CurrTimeFract, then add the integer part of that to m_CurrTime
    m_CurrTimeFract += increment - integer;
    integer          = floor(m_CurrTimeFract);
    m_CurrTime      += (int64_t)(integer + 0.5);
    m_CurrTimeFract -= integer;
  }
}

double CVideoReferenceClock::UpdateInterval()
{
  return m_ClockSpeed * m_fineadjust / (double)m_RefreshRate * (double)m_SystemFrequency;
}

//called from dvdclock to get the time
int64_t CVideoReferenceClock::GetTime(bool interpolated /* = true*/)
{
  CSingleLock SingleLock(m_CritSection);

  //when using vblank, get the time from that, otherwise use the systemclock
  if (m_UseVblank)
  {
    int64_t  NextVblank;
    int64_t  Now;

    Now = CurrentHostCounter();        //get current system time
    NextVblank = TimeOfNextVblank();   //get time when the next vblank should happen

    while(Now >= NextVblank)  //keep looping until the next vblank is in the future
    {
      UpdateClock(1, false);           //update clock when next vblank should have happened already
      NextVblank = TimeOfNextVblank(); //get time when the next vblank should happen
    }

    if (interpolated)
    {
      //interpolate from the last time the clock was updated
      double elapsed = (double)(Now - m_VblankTime) * m_ClockSpeed * m_fineadjust;
      //don't interpolate more than 2 vblank periods
      elapsed = min(elapsed, UpdateInterval() * 2.0);

      //make sure the clock doesn't go backwards
      int64_t intTime = m_CurrTime + (int64_t)elapsed;
      if (intTime > m_LastIntTime)
        m_LastIntTime = intTime;

      return m_LastIntTime;
    }
    else
    {
      return m_CurrTime;
    }
  }
  else
  {
    return CurrentHostCounter() + m_ClockOffset;
  }
}

//called from dvdclock to get the clock frequency
int64_t CVideoReferenceClock::GetFrequency()
{
  return m_SystemFrequency;
}

void CVideoReferenceClock::SetSpeed(double Speed)
{
  CSingleLock SingleLock(m_CritSection);
  //dvdplayer can change the speed to fit the rereshrate
  if (m_UseVblank)
  {
    if (Speed != m_ClockSpeed)
    {
      m_ClockSpeed = Speed;
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Clock speed %f%%", GetSpeed() * 100.0);
    }
  }
}

double CVideoReferenceClock::GetSpeed()
{
  CSingleLock SingleLock(m_CritSection);

  //dvdplayer needs to know the speed for the resampler
  if (m_UseVblank)
    return m_ClockSpeed;
  else
    return 1.0;
}

bool CVideoReferenceClock::UpdateRefreshrate(bool Forced /*= false*/)
{
  //if the graphicscontext signaled that the refreshrate changed, we check it about one second later
  if (m_RefreshChanged == 1 && !Forced)
  {
    m_LastRefreshTime = m_CurrTime;
    m_RefreshChanged = 2;
    return false;
  }

  //update the refreshrate about once a second, or update immediately if a forced update is required
  if (m_CurrTime - m_LastRefreshTime < m_SystemFrequency && !Forced)
    return false;

  if (Forced)
    m_LastRefreshTime = 0;
  else
    m_LastRefreshTime = m_CurrTime;

#if defined(HAS_GLX) && defined(HAS_XRANDR)

  // the correct refresh rate is always stores in g_settings
  bool   bUpdate = Forced || m_RefreshChanged == 2;

  if (!Forced)
    m_RefreshChanged = 0;

  if (!bUpdate) //refreshrate did not change
    return false;

  CSingleLock SingleLock(m_CritSection);
  m_RefreshRate = MathUtils::round_int(g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fRefreshRate);

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", (int)m_RefreshRate);

  return true;

#elif defined(_WIN32) && defined(HAS_DX)

  D3DDISPLAYMODE DisplayMode;
  m_D3dDev->GetDisplayMode(0, &DisplayMode);

  //0 indicates adapter default
  if (DisplayMode.RefreshRate == 0)
    DisplayMode.RefreshRate = 60;

  if (m_PrevRefreshRate != DisplayMode.RefreshRate || m_Width != DisplayMode.Width || m_Height != DisplayMode.Height ||
      m_Interlaced != g_Windowing.Interlaced() || Forced )
  {
    m_PrevRefreshRate = DisplayMode.RefreshRate;
    m_Width = DisplayMode.Width;
    m_Height = DisplayMode.Height;
    m_Interlaced = g_Windowing.Interlaced();
    return true;
  }

  return false;

#elif defined(__APPLE__)
  #if defined(__arm__)
    int RefreshRate = round(g_Windowing.GetDisplayLinkFPS() + 0.5);
  #else
    int RefreshRate = MathUtils::round_int(Cocoa_GetCVDisplayLinkRefreshPeriod());
  #endif

  if (RefreshRate != m_RefreshRate || Forced)
  {
    CSingleLock SingleLock(m_CritSection);
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", RefreshRate);
    m_RefreshRate = RefreshRate;
    return true;
  }
  return false;
#endif

  return false;
}

//dvdplayer needs to know the refreshrate for matching the fps of the video playing to it
int CVideoReferenceClock::GetRefreshRate(double* interval /*= NULL*/)
{
  CSingleLock SingleLock(m_CritSection);

  if (m_UseVblank)
  {
    if (interval)
      *interval = m_ClockSpeed / m_RefreshRate;

    return (int)m_RefreshRate;
  }
  else
    return -1;
}


//this is called from CDVDClock::WaitAbsoluteClock, which is called from CXBMCRenderManager::WaitPresentTime
//it waits until a certain timestamp has passed, used for displaying videoframes at the correct moment
int64_t CVideoReferenceClock::Wait(int64_t Target)
{
  int64_t       Now;
  int           SleepTime;

  CSingleLock SingleLock(m_CritSection);

  if (m_UseVblank) //when true the vblank is used as clock source
  {
    while (m_CurrTime < Target)
    {
      //calculate how long to sleep before we should have gotten a signal that a vblank happened
      Now = CurrentHostCounter();
      int64_t NextVblank = TimeOfNextVblank();
      SleepTime = (int)((NextVblank - Now) * 1000 / m_SystemFrequency);

      int64_t CurrTime = m_CurrTime; //save current value of the clock

      bool Late = false;
      if (SleepTime <= 0) //if sleeptime is 0 or lower, the vblank clock is already late in updating
      {
        Late = true;
      }
      else
      {
        m_VblankEvent.Reset();
        SingleLock.Leave();
        if (!m_VblankEvent.WaitMSec(SleepTime)) //if this returns false, it means the vblank event was not set within
          Late = true;                          //the required time
        SingleLock.Enter();
      }

      //if the vblank clock was late with its update, we update the clock ourselves
      if (Late && CurrTime == m_CurrTime)
        UpdateClock(1, false); //update the clock by 1 vblank

    }
    return m_CurrTime;
  }
  else
  {
    int64_t ClockOffset = m_ClockOffset;
    SingleLock.Leave();
    Now = CurrentHostCounter();
    //sleep until the timestamp has passed
    SleepTime = (int)((Target - (Now + ClockOffset)) * 1000 / m_SystemFrequency);
    if (SleepTime > 0)
      ::Sleep(SleepTime);

    Now = CurrentHostCounter();
    return Now + ClockOffset;
  }
}


void CVideoReferenceClock::SendVblankSignal()
{
  m_VblankEvent.Set();
}

#define MAXVBLANKDELAY 13LL
//guess when the next vblank should happen,
//based on the refreshrate and when the previous one happened
//increase that by 30% to allow for errors
int64_t CVideoReferenceClock::TimeOfNextVblank()
{
  return m_VblankTime + (m_SystemFrequency / m_RefreshRate * MAXVBLANKDELAY / 10LL);
}

//for the codec information screen
bool CVideoReferenceClock::GetClockInfo(int& MissedVblanks, double& ClockSpeed, int& RefreshRate)
{
  if (m_UseVblank)
  {
    MissedVblanks = m_TotalMissedVblanks;
    ClockSpeed = m_ClockSpeed * 100.0;
    RefreshRate = (int)m_RefreshRate;
    return true;
  }
  return false;
}

void CVideoReferenceClock::SetFineAdjust(double fineadjust)
{
  CSingleLock SingleLock(m_CritSection);
  m_fineadjust = fineadjust;
}

CVideoReferenceClock g_VideoReferenceClock;
