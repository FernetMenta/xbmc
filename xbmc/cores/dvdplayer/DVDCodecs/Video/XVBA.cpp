/*
 *      Copyright (C) 2005-2011 Team XBMC
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
#ifdef HAVE_LIBXVBA

#include "system_gl.h"
#include <dlfcn.h>
#include <string>
#include "XVBA.h"
#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "utils/TimeUtils.h"
#include "cores/dvdplayer/DVDClock.h"

using namespace XVBA;

// XVBA interface

#define XVBA_LIBRARY    "libXvBAW.so.1"

typedef Bool        (*XVBAQueryExtensionProc)       (Display *dpy, int *vers);
typedef Status      (*XVBACreateContextProc)        (void *input, void *output);
typedef Status      (*XVBADestroyContextProc)       (void *context);
typedef Bool        (*XVBAGetSessionInfoProc)       (void *input, void *output);
typedef Status      (*XVBACreateSurfaceProc)        (void *input, void *output);
typedef Status      (*XVBACreateGLSharedSurfaceProc)(void *input, void *output);
typedef Status      (*XVBADestroySurfaceProc)       (void *surface);
typedef Status      (*XVBACreateDecodeBuffersProc)  (void *input, void *output);
typedef Status      (*XVBADestroyDecodeBuffersProc) (void *input);
typedef Status      (*XVBAGetCapDecodeProc)         (void *input, void *output);
typedef Status      (*XVBACreateDecodeProc)         (void *input, void *output);
typedef Status      (*XVBADestroyDecodeProc)        (void *session);
typedef Status      (*XVBAStartDecodePictureProc)   (void *input);
typedef Status      (*XVBADecodePictureProc)        (void *input);
typedef Status      (*XVBAEndDecodePictureProc)     (void *input);
typedef Status      (*XVBASyncSurfaceProc)          (void *input, void *output);
typedef Status      (*XVBAGetSurfaceProc)           (void *input);
typedef Status      (*XVBATransferSurfaceProc)      (void *input);

static struct
{
  XVBAQueryExtensionProc              QueryExtension;
  XVBACreateContextProc               CreateContext;
  XVBADestroyContextProc              DestroyContext;
  XVBAGetSessionInfoProc              GetSessionInfo;
  XVBACreateSurfaceProc               CreateSurface;
  XVBACreateGLSharedSurfaceProc       CreateGLSharedSurface;
  XVBADestroySurfaceProc              DestroySurface;
  XVBACreateDecodeBuffersProc         CreateDecodeBuffers;
  XVBADestroyDecodeBuffersProc        DestroyDecodeBuffers;
  XVBAGetCapDecodeProc                GetCapDecode;
  XVBACreateDecodeProc                CreateDecode;
  XVBADestroyDecodeProc               DestroyDecode;
  XVBAStartDecodePictureProc          StartDecodePicture;
  XVBADecodePictureProc               DecodePicture;
  XVBAEndDecodePictureProc            EndDecodePicture;
  XVBASyncSurfaceProc                 SyncSurface;
  XVBAGetSurfaceProc                  GetSurface;
  XVBATransferSurfaceProc             TransferSurface;
}g_XVBA_vtable;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CXVBAContext *CXVBAContext::m_context = 0;
CCriticalSection CXVBAContext::m_section;
Display *CXVBAContext::m_display = 0;
void *CXVBAContext::m_dlHandle = 0;

CXVBAContext::CXVBAContext()
{
  m_context = 0;
//  m_dlHandle = 0;
  m_xvbaContext = 0;
  m_refCount = 0;
}

void CXVBAContext::Release()
{
  CSingleLock lock(m_section);

  m_refCount--;
  if (m_refCount <= 0)
  {
    Close();
    delete this;
    m_context = 0;
  }
}

void CXVBAContext::Close()
{
  CLog::Log(LOGNOTICE, "XVBA::Close - closing decoder context");

  DestroyContext();
//  if (m_dlHandle)
//  {
//    dlclose(m_dlHandle);
//    m_dlHandle = 0;
//  }
}

bool CXVBAContext::EnsureContext(CXVBAContext **ctx)
{
  CSingleLock lock(m_section);

  if (m_context)
  {
    m_context->m_refCount++;
    *ctx = m_context;
    return true;
  }

  m_context = new CXVBAContext();
  *ctx = m_context;
  {
    CSingleLock gLock(g_graphicsContext);
    if (!m_context->LoadSymbols() || !m_context->CreateContext())
    {
      delete m_context;
      m_context = 0;
      return false;
    }
  }

  m_context->m_refCount++;

  *ctx = m_context;
  return true;
}

bool CXVBAContext::LoadSymbols()
{
  if (!m_dlHandle)
  {
    m_dlHandle  = dlopen(XVBA_LIBRARY, RTLD_LAZY);
    if (!m_dlHandle)
    {
      const char* error = dlerror();
      if (!error)
        error = "dlerror() returned NULL";

      CLog::Log(LOGERROR,"XVBA::LoadSymbols: Unable to get handle to lib: %s", error);
      return false;
    }
  }
  else
    return true;

#define INIT_PROC(PREFIX, PROC) do {                            \
        g_##PREFIX##_vtable.PROC = (PREFIX##PROC##Proc)         \
            dlsym(m_dlHandle, #PREFIX #PROC);                   \
    } while (0)

#define INIT_PROC_CHECK(PREFIX, PROC) do {                      \
        dlerror();                                              \
        INIT_PROC(PREFIX, PROC);                                \
        if (dlerror()) {                                        \
            dlclose(m_dlHandle);                                \
            m_dlHandle = NULL;                                  \
            return false;                                       \
        }                                                       \
    } while (0)

#define XVBA_INIT_PROC(PROC) INIT_PROC_CHECK(XVBA, PROC)

  XVBA_INIT_PROC(QueryExtension);
  XVBA_INIT_PROC(CreateContext);
  XVBA_INIT_PROC(DestroyContext);
  XVBA_INIT_PROC(GetSessionInfo);
  XVBA_INIT_PROC(CreateSurface);
  XVBA_INIT_PROC(CreateGLSharedSurface);
  XVBA_INIT_PROC(DestroySurface);
  XVBA_INIT_PROC(CreateDecodeBuffers);
  XVBA_INIT_PROC(DestroyDecodeBuffers);
  XVBA_INIT_PROC(GetCapDecode);
  XVBA_INIT_PROC(CreateDecode);
  XVBA_INIT_PROC(DestroyDecode);
  XVBA_INIT_PROC(StartDecodePicture);
  XVBA_INIT_PROC(DecodePicture);
  XVBA_INIT_PROC(EndDecodePicture);
  XVBA_INIT_PROC(SyncSurface);
  XVBA_INIT_PROC(GetSurface);
  XVBA_INIT_PROC(TransferSurface);

#undef XVBA_INIT_PROC
#undef INIT_PROC

  return true;
}

bool CXVBAContext::CreateContext()
{
  if (m_xvbaContext)
    return true;

  CLog::Log(LOGNOTICE,"XVBA::CreateContext - creating decoder context");

  Drawable window;
  { CSingleLock lock(g_graphicsContext);
    if (!m_display)
      m_display = XOpenDisplay(NULL);
    window = 0;
  }

  int version;
  if (!g_XVBA_vtable.QueryExtension(m_display, &version))
    return false;
  CLog::Log(LOGNOTICE,"XVBA::CreateContext - opening xvba version: %i", version);

  // create XVBA Context
  XVBA_Create_Context_Input contextInput;
  XVBA_Create_Context_Output contextOutput;
  contextInput.size = sizeof(contextInput);
  contextInput.display = m_display;
  contextInput.draw = window;
  contextOutput.size = sizeof(contextOutput);
  if(Success != g_XVBA_vtable.CreateContext(&contextInput, &contextOutput))
  {
    CLog::Log(LOGERROR,"XVBA::CreateContext - failed to create context");
    return false;
  }
  m_xvbaContext = contextOutput.context;

  return true;
}

void CXVBAContext::DestroyContext()
{
  if (!m_xvbaContext)
    return;

  g_XVBA_vtable.DestroyContext(m_xvbaContext);
  m_xvbaContext = 0;
//  XCloseDisplay(m_display);
}

void *CXVBAContext::GetContext()
{
  return m_xvbaContext;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static unsigned int decoderId = 0;

CDecoder::CDecoder() : m_xvbaOutput(&m_inMsgEvent)
{
  m_xvbaConfig.context = 0;
  m_xvbaConfig.xvbaSession = 0;
  m_xvbaConfig.videoSurfaces = &m_videoSurfaces;
  m_xvbaConfig.videoSurfaceSec = &m_videoSurfaceSec;
  m_xvbaConfig.apiSec = &m_apiSec;

  m_displayState = XVBA_OPEN;
}

CDecoder::~CDecoder()
{
  Close();
}

typedef struct {
    unsigned int  size;
    unsigned int  num_of_decodecaps;
    XVBADecodeCap decode_caps_list[];
} XVBA_GetCapDecode_Output_Base;

void CDecoder::OnLostDevice()
{
  CLog::Log(LOGNOTICE,"XVBA::OnLostDevice event");

  CSingleLock lock(m_decoderSection);
  DestroySession();
  if (m_xvbaConfig.context)
    m_xvbaConfig.context->Release();
  m_xvbaConfig.context = 0;

  m_displayState = XVBA_LOST;
  m_displayEvent.Reset();
}

void CDecoder::OnResetDevice()
{
  CLog::Log(LOGNOTICE,"XVBA::OnResetDevice event");

  CSingleLock lock(m_decoderSection);
  if (m_displayState == XVBA_LOST)
  {
    m_displayState = XVBA_RESET;
    lock.Leave();
    m_displayEvent.Set();
  }
}

bool CDecoder::Open(AVCodecContext* avctx, const enum PixelFormat fmt, unsigned int surfaces)
{
  std::string Vendor = g_Windowing.GetRenderVendor();
  std::transform(Vendor.begin(), Vendor.end(), Vendor.begin(), ::tolower);
  if (Vendor.compare(0, 3, "ati") != 0)
  {
    return false;
  }

  m_decoderId = decoderId++;

  CLog::Log(LOGNOTICE,"(XVBA::Open) opening xvba decoder, id: %d", m_decoderId);

  if(avctx->coded_width  == 0
  || avctx->coded_height == 0)
  {
    CLog::Log(LOGWARNING,"(XVBA) no width/height available, can't init");
    return false;
  }

  // Fixme: Revisit with new SDK
  // Workaround for 0.74.01-AES-2 that does not signal if surfaces are too large
  // it seems that xvba does not support anything > 2k
  // return false, for files that are larger
  // if you are unlucky, this would kill your decoder
  // we limit to 2048x1152 now - as this was reported by ckoenig the UVD OSS 
  // engineer to be the maximum technically possible for all hardware with UVD.
  // Some chips can do a bit more, but we use the safe default.
  int surfaceWidth = (avctx->coded_width+15) & ~15;
  int surfaceHeight = (avctx->coded_height+15) & ~15;
  if(surfaceHeight > 1152 || surfaceWidth > 2048)
  {
    CLog::Log(LOGERROR, "Surface too large, decoder skipped: surfaceWidth %u, surfaceHeight %u",
                        surfaceWidth, surfaceHeight);
    return false;
  }

  if (!m_dllAvUtil.Load())
    return false;

  if (!CXVBAContext::EnsureContext(&m_xvbaConfig.context))
    return false;

  // xvba get session info
  XVBA_GetSessionInfo_Input sessionInput;
  XVBA_GetSessionInfo_Output sessionOutput;
  sessionInput.size = sizeof(sessionInput);
  sessionInput.context = m_xvbaConfig.context->GetContext();
  sessionOutput.size = sizeof(sessionOutput);
  if (Success != g_XVBA_vtable.GetSessionInfo(&sessionInput, &sessionOutput))
  {
    CLog::Log(LOGERROR,"(XVBA) can't get session info");
    return false;
  }
  if (sessionOutput.getcapdecode_output_size == 0)
  {
    CLog::Log(LOGERROR,"(XVBA) session decode not supported");
    return false;
  }

  // get decoder capabilities
  XVBA_GetCapDecode_Input capInput;
  XVBA_GetCapDecode_Output *capOutput;
  capInput.size = sizeof(capInput);
  capInput.context = m_xvbaConfig.context->GetContext();
  capOutput = (XVBA_GetCapDecode_Output *)calloc(sessionOutput.getcapdecode_output_size, 1);
  capOutput->size = sessionOutput.getcapdecode_output_size;
  if (Success != g_XVBA_vtable.GetCapDecode(&capInput, capOutput))
  {
    CLog::Log(LOGERROR,"(XVBA) can't get decode capabilities");
    return false;
  }

  int match = -1;
  if (avctx->codec_id == CODEC_ID_H264)
  {
    // search for profile high
    for (unsigned int i = 0; i < capOutput->num_of_decodecaps; ++i)
    {
      if (capOutput->decode_caps_list[i].capability_id == XVBA_H264 &&
          capOutput->decode_caps_list[i].flags == XVBA_H264_HIGH)
      {
        match = (int) i;
        break;
      }
    }
    if (match < 0)
    {
      CLog::Log(LOGNOTICE, "(XVBA::Open) - profile XVBA_H264_HIGH not found");
    }
  }
  else if (avctx->codec_id == CODEC_ID_VC1)
  {
    // search for profile advanced
    for (unsigned int i = 0; i < capOutput->num_of_decodecaps; ++i)
    {
      if (capOutput->decode_caps_list[i].capability_id == XVBA_VC1 &&
          capOutput->decode_caps_list[i].flags == XVBA_VC1_ADVANCED)
      {
        match = (int) i;
        break;
      }
    }
    if (match < 0)
    {
      CLog::Log(LOGNOTICE, "(XVBA::Open) - profile XVBA_VC1_ADVANCED not found");
    }
  }
  else if (avctx->codec_id == CODEC_ID_MPEG2VIDEO)
  {
    // search for profile high
    for (unsigned int i = 0; i < capOutput->num_of_decodecaps; ++i)
    {
      if (capOutput->decode_caps_list[i].capability_id == XVBA_MPEG2_VLD)
      {
        // XXX: uncomment when implemented
        // match = (int) i;
        // break;
      }
    }
    if (match < 0)
    {
      CLog::Log(LOGNOTICE, "(XVBA::Open) - profile XVBA_MPEG2_VLD not found");
    }
  }
  else if (avctx->codec_id == CODEC_ID_WMV3)
  {
    // search for profile high
    for (unsigned int i = 0; i < capOutput->num_of_decodecaps; ++i)
    {
      if (capOutput->decode_caps_list[i].capability_id == XVBA_VC1 &&
          capOutput->decode_caps_list[i].flags == XVBA_VC1_MAIN)
      {
        match = (int) i;
        break;
      }
    }
    if (match < 0)
    {
      CLog::Log(LOGNOTICE, "(XVBA::Open) - profile XVBA_VC1_MAIN not found");
    }
  }

  if (match < 0)
  {
    free(capOutput);
    return false;
  }

  CLog::Log(LOGNOTICE,"(XVBA) using decoder capability id: %i flags: %i",
                          capOutput->decode_caps_list[match].capability_id,
                          capOutput->decode_caps_list[match].flags);
  CLog::Log(LOGNOTICE,"(XVBA) using surface type: %x",
                          capOutput->decode_caps_list[match].surface_type);

  m_xvbaConfig.decoderCap = capOutput->decode_caps_list[match];

  free(capOutput);

  // set some varables
  m_xvbaConfig.xvbaSession = 0;
  m_xvbaBufferPool.data_buffer = 0;
  m_xvbaBufferPool.iq_matrix_buffer = 0;
  m_xvbaBufferPool.picture_descriptor_buffer = 0;
  m_presentPicture = 0;
  m_xvbaConfig.numRenderBuffers = surfaces;
  m_decoderThread = CThread::GetCurrentThreadId();
  m_speed = DVD_PLAYSPEED_NORMAL;

  if (1) //g_guiSettings.GetBool("videoplayer.usexvbasharedsurface"))
    m_xvbaConfig.useSharedSurfaces = true;
  else
    m_xvbaConfig.useSharedSurfaces = false;

  m_displayState = XVBA_OPEN;

  // setup ffmpeg
  avctx->thread_count    = 1;
  avctx->get_buffer      = CDecoder::FFGetBuffer;
  avctx->release_buffer  = CDecoder::FFReleaseBuffer;
  avctx->draw_horiz_band = CDecoder::FFDrawSlice;
  avctx->slice_flags     = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;

  g_Windowing.Register(this);
  return true;
}

void CDecoder::Close()
{
  CLog::Log(LOGNOTICE, "XVBA::Close - closing decoder, id: %d", m_decoderId);

  if (!m_xvbaConfig.context)
    return;

  DestroySession();
  if (m_xvbaConfig.context)
    m_xvbaConfig.context->Release();
  m_xvbaConfig.context = 0;

  while (!m_videoSurfaces.empty())
  {
    xvba_render_state *render = m_videoSurfaces.back();
    if(render->buffers_alllocated > 0)
      m_dllAvUtil.av_free(render->buffers);
    m_videoSurfaces.pop_back();
    free(render);
  }

  g_Windowing.Unregister(this);
  m_dllAvUtil.Unload();
}

long CDecoder::Release()
{
  // check if we should do some pre-cleanup here
  // a second decoder might need resources
  if (m_xvbaConfig.xvbaSession)
  {
    CSingleLock lock(m_decoderSection);
    CLog::Log(LOGNOTICE,"XVBA::Release pre-cleanup");
    DestroySession(true);
  }
  return IHardwareDecoder::Release();
}

long CDecoder::ReleasePicReference()
{
  return IHardwareDecoder::Release();
}

bool CDecoder::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_AUTO)
    return true;

  if (1) //g_guiSettings.GetBool("videoplayer.usexvbasharedsurface"))
  {
    if (method == VS_INTERLACEMETHOD_XVBA)
      return true;
  }

  return false;
}

void CDecoder::ResetState()
{
  m_displayState = XVBA_OPEN;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  EDisplayState state;

  { CSingleLock lock(m_decoderSection);
    state = m_displayState;
  }

  if (state == XVBA_LOST)
  {
    CLog::Log(LOGNOTICE,"XVBA::Check waiting for display reset event");
    if (!m_displayEvent.WaitMSec(2000))
    {
      CLog::Log(LOGERROR, "XVBA::Check - device didn't reset in reasonable time");
      state = XVBA_RESET;;
    }
    else
    { CSingleLock lock(m_decoderSection);
      state = m_displayState;
    }
  }
  if (state == XVBA_RESET || state == XVBA_ERROR)
  {
    CLog::Log(LOGNOTICE,"XVBA::Check - Attempting recovery");

    CSingleLock gLock(g_graphicsContext);
    CSingleLock lock(m_decoderSection);

    DestroySession();
    ResetState();
    CXVBAContext::EnsureContext(&m_xvbaConfig.context);

    if (state == XVBA_RESET)
      return VC_FLUSHED;
    else
      return VC_ERROR;
  }
  return 0;
}

void CDecoder::SetError(const char* function, const char* msg, int line)
{
  CLog::Log(LOGERROR, "XVBA::%s - %s, line %d", function, msg, line);
  CSingleLock lock(m_decoderSection);
  m_displayState = XVBA_ERROR;
}

bool CDecoder::CreateSession(AVCodecContext* avctx)
{
  m_xvbaConfig.surfaceWidth = (avctx->coded_width+15) & ~15;
  m_xvbaConfig.surfaceHeight = (avctx->coded_height+15) & ~15;

  m_xvbaConfig.vidWidth = avctx->width;
  m_xvbaConfig.vidHeight = avctx->height;

  XVBA_Create_Decode_Session_Input sessionInput;
  XVBA_Create_Decode_Session_Output sessionOutput;

  sessionInput.size = sizeof(sessionInput);
  sessionInput.width = m_xvbaConfig.surfaceWidth;
  sessionInput.height = m_xvbaConfig.surfaceHeight;
  sessionInput.context = m_xvbaConfig.context->GetContext();
  sessionInput.decode_cap = &m_xvbaConfig.decoderCap;
  sessionOutput.size = sizeof(sessionOutput);

  if (Success != g_XVBA_vtable.CreateDecode(&sessionInput, &sessionOutput))
  {
    SetError(__FUNCTION__, "failed to create decoder session", __LINE__);
    CLog::Log(LOGERROR, "Decoder failed with following stats: m_surfaceWidth %u, m_surfaceHeight %u,"
                        " m_vidWidth %u, m_vidHeight %u, coded_width %d, coded_height %d",
                        m_xvbaConfig.surfaceWidth,
                        m_xvbaConfig.surfaceHeight,
                        m_xvbaConfig.vidWidth,
                        m_xvbaConfig.vidHeight,
                        avctx->coded_width,
                        avctx->coded_height);
    return false;
  }
  m_xvbaConfig.xvbaSession = sessionOutput.session;

  // create decode buffers
  XVBA_Create_DecodeBuff_Input bufferInput;
  XVBA_Create_DecodeBuff_Output bufferOutput;

  bufferInput.size = sizeof(bufferInput);
  bufferInput.session = m_xvbaConfig.xvbaSession;
  bufferInput.buffer_type = XVBA_PICTURE_DESCRIPTION_BUFFER;
  bufferInput.num_of_buffers = 1;
  bufferOutput.size = sizeof(bufferOutput);
  if (Success != g_XVBA_vtable.CreateDecodeBuffers(&bufferInput, &bufferOutput)
      || bufferOutput.num_of_buffers_in_list != 1)
  {
    SetError(__FUNCTION__, "failed to create picture buffer", __LINE__);
    return false;
  }
  m_xvbaBufferPool.picture_descriptor_buffer = bufferOutput.buffer_list;

  // data buffer
  bufferInput.buffer_type = XVBA_DATA_BUFFER;
  if (Success != g_XVBA_vtable.CreateDecodeBuffers(&bufferInput, &bufferOutput)
      || bufferOutput.num_of_buffers_in_list != 1)
  {
    SetError(__FUNCTION__, "failed to create data buffer", __LINE__);
    return false;
  }
  m_xvbaBufferPool.data_buffer = bufferOutput.buffer_list;

  // QO Buffer
  bufferInput.buffer_type = XVBA_QM_BUFFER;
  if (Success != g_XVBA_vtable.CreateDecodeBuffers(&bufferInput, &bufferOutput)
      || bufferOutput.num_of_buffers_in_list != 1)
  {
    SetError(__FUNCTION__, "failed to create qm buffer", __LINE__);
    return false;
  }
  m_xvbaBufferPool.iq_matrix_buffer = bufferOutput.buffer_list;


  // initialize output
  CSingleLock lock(g_graphicsContext);
  m_xvbaConfig.stats = &m_bufferStats;
  m_bufferStats.Reset();
  m_xvbaOutput.Start();
  Message *reply;
  if (m_xvbaOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::INIT,
                                                 &reply,
                                                 2000,
                                                 &m_xvbaConfig,
                                                 sizeof(m_xvbaConfig)))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "XVBA::%s - vdpau output returned error", __FUNCTION__);
      m_xvbaOutput.Dispose();
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "XVBA::%s - failed to init output", __FUNCTION__);
    m_xvbaOutput.Dispose();
    return false;
  }
  m_inMsgEvent.Reset();

  return true;
}

void CDecoder::DestroySession(bool precleanup /*= false*/)
{
  // wait for unfinished decoding jobs
  XbmcThreads::EndTime timer;
  if (m_xvbaConfig.xvbaSession)
  {
    for (unsigned int i = 0; i < m_videoSurfaces.size(); ++i)
    {
      xvba_render_state *render = m_videoSurfaces[i];
      if (render->surface)
      {
        XVBA_Surface_Sync_Input syncInput;
        XVBA_Surface_Sync_Output syncOutput;
        syncInput.size = sizeof(syncInput);
        syncInput.session = m_xvbaConfig.xvbaSession;
        syncInput.surface = render->surface;
        syncInput.query_status = XVBA_GET_SURFACE_STATUS;
        syncOutput.size = sizeof(syncOutput);
        timer.Set(1000);
        while(!timer.IsTimePast())
        {
          if (Success != g_XVBA_vtable.SyncSurface(&syncInput, &syncOutput))
          {
            CLog::Log(LOGERROR,"XVBA::DestroySession - failed sync surface");
            break;
          }
          if (!(syncOutput.status_flags & XVBA_STILL_PENDING))
            break;
          Sleep(10);
        }
        if (timer.IsTimePast())
          CLog::Log(LOGERROR,"XVBA::DestroySession - unfinished decoding job");
      }
    }
  }

  if (precleanup)
  {
    Message *reply;
    if (m_xvbaOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::PRECLEANUP,
                                                   &reply,
                                                   2000))
    {
      bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
      reply->Release();
      if (!success)
      {
        CLog::Log(LOGERROR, "XVBA::%s - pre-cleanup returned error", __FUNCTION__);
        m_displayState = XVBA_ERROR;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "XVBA::%s - pre-cleanup timed out", __FUNCTION__);
      m_displayState = XVBA_ERROR;
    }
  }
  else
    m_xvbaOutput.Dispose();

  XVBA_Destroy_Decode_Buffers_Input bufInput;
  bufInput.size = sizeof(bufInput);
  bufInput.num_of_buffers_in_list = 1;
  bufInput.session = m_xvbaConfig.xvbaSession;

  for (unsigned int i=0; i<m_xvbaBufferPool.data_control_buffers.size() ; ++i)
  {
    bufInput.buffer_list = m_xvbaBufferPool.data_control_buffers[i];
    g_XVBA_vtable.DestroyDecodeBuffers(&bufInput);
  }
  m_xvbaBufferPool.data_control_buffers.clear();

  if (m_xvbaConfig.xvbaSession && m_xvbaBufferPool.data_buffer)
  {
    bufInput.buffer_list = m_xvbaBufferPool.data_buffer;
    g_XVBA_vtable.DestroyDecodeBuffers(&bufInput);
  }
  m_xvbaBufferPool.data_buffer = 0;

  if (m_xvbaConfig.xvbaSession && m_xvbaBufferPool.picture_descriptor_buffer)
  {
    bufInput.buffer_list = m_xvbaBufferPool.picture_descriptor_buffer;
    g_XVBA_vtable.DestroyDecodeBuffers(&bufInput);
  }
  m_xvbaBufferPool.picture_descriptor_buffer = 0;

  if (m_xvbaConfig.xvbaSession && m_xvbaBufferPool.iq_matrix_buffer)
  {
    bufInput.buffer_list = m_xvbaBufferPool.iq_matrix_buffer;
    g_XVBA_vtable.DestroyDecodeBuffers(&bufInput);
  }
  m_xvbaBufferPool.iq_matrix_buffer = 0;

  for (unsigned int i = 0; i < m_videoSurfaces.size(); ++i)
  {
    xvba_render_state *render = m_videoSurfaces[i];
    if (m_xvbaConfig.xvbaSession && render->surface)
    {
      g_XVBA_vtable.DestroySurface(render->surface);
      render->surface = 0;
      render->state = 0;
      render->picture_descriptor = 0;
      render->iq_matrix = 0;
    }
  }

  if (m_xvbaConfig.xvbaSession)
    g_XVBA_vtable.DestroyDecode(m_xvbaConfig.xvbaSession);
  m_xvbaConfig.xvbaSession = 0;
}

bool CDecoder::IsSurfaceValid(xvba_render_state *render)
{
  // find render state in queue
  bool found(false);
  unsigned int i;
  for(i = 0; i < m_videoSurfaces.size(); ++i)
  {
    if(m_videoSurfaces[i] == render)
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    CLog::Log(LOGERROR,"%s - video surface not found", __FUNCTION__);
    return false;
  }
  if (m_videoSurfaces[i]->surface == 0)
  {
    m_videoSurfaces[i]->state = 0;
    return false;
  }

  return true;
}

bool CDecoder::EnsureDataControlBuffers(unsigned int num)
{
  if (m_xvbaBufferPool.data_control_buffers.size() >= num)
    return true;

  unsigned int missing = num - m_xvbaBufferPool.data_control_buffers.size();

  XVBA_Create_DecodeBuff_Input bufferInput;
  XVBA_Create_DecodeBuff_Output bufferOutput;
  bufferInput.size = sizeof(bufferInput);
  bufferInput.session = m_xvbaConfig.xvbaSession;
  bufferInput.buffer_type = XVBA_DATA_CTRL_BUFFER;
  bufferInput.num_of_buffers = 1;
  bufferOutput.size = sizeof(bufferOutput);

  for (unsigned int i=0; i<missing; ++i)
  {
    { CSingleLock lock(m_apiSec);
      if (Success != g_XVBA_vtable.CreateDecodeBuffers(&bufferInput, &bufferOutput)
          || bufferOutput.num_of_buffers_in_list != 1)
      {
        SetError(__FUNCTION__, "failed to create data control buffer", __LINE__);
        return false;
      }
    }
    m_xvbaBufferPool.data_control_buffers.push_back(bufferOutput.buffer_list);
  }

  return true;
}

void CDecoder::FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CDVDVideoCodecFFmpeg* ctx   = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDecoder*             xvba  = (CDecoder*)ctx->GetHardware();
  unsigned int i;

  CSingleLock lock(xvba->m_decoderSection);

  xvba_render_state * render = NULL;
  render = (xvba_render_state*)pic->data[0];
  if(!render)
  {
    CLog::Log(LOGERROR, "XVBA::FFReleaseBuffer - invalid context handle provided");
    return;
  }

  for(i=0; i<4; i++)
    pic->data[i]= NULL;

  // find render state in queue
  if (!xvba->IsSurfaceValid(render))
  {
    CLog::Log(LOGDEBUG, "XVBA::FFReleaseBuffer - ignoring invalid buffer");
    return;
  }

  render->state &= ~FF_XVBA_STATE_USED_FOR_REFERENCE;
}

void CDecoder::FFDrawSlice(struct AVCodecContext *avctx,
                             const AVFrame *src, int offset[4],
                             int y, int type, int height)
{
  CDVDVideoCodecFFmpeg* ctx   = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDecoder*             xvba  = (CDecoder*)ctx->GetHardware();

  CSingleLock lock(xvba->m_decoderSection);

  if(xvba->m_displayState != XVBA_OPEN)
    return;

  if(src->linesize[0] || src->linesize[1] || src->linesize[2]
    || offset[0] || offset[1] || offset[2])
  {
    CLog::Log(LOGERROR, "XVBA::FFDrawSlice - invalid linesizes or offsets provided");
    return;
  }

  xvba_render_state * render;

  render = (xvba_render_state*)src->data[0];
  if(!render)
  {
    CLog::Log(LOGERROR, "XVBA::FFDrawSlice - invalid context handle provided");
    return;
  }

  // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
  if (!xvba->IsSurfaceValid(render))
  {
    CLog::Log(LOGWARNING, "XVBA::FFDrawSlice - ignoring invalid buffer");
    return;
  }

  // decoding
  XVBA_Decode_Picture_Start_Input startInput;
  startInput.size = sizeof(startInput);
  startInput.session = xvba->m_xvbaConfig.xvbaSession;
  startInput.target_surface = render->surface;
  { CSingleLock lock(xvba->m_apiSec);
    if (Success != g_XVBA_vtable.StartDecodePicture(&startInput))
    {
      xvba->SetError(__FUNCTION__, "failed to start decoding", __LINE__);
      return;
    }
  }

  XVBA_Decode_Picture_Input picInput;
  picInput.size = sizeof(picInput);
  picInput.session = xvba->m_xvbaConfig.xvbaSession;
  XVBABufferDescriptor *list[2];
  picInput.buffer_list = list;
  list[0] = xvba->m_xvbaBufferPool.picture_descriptor_buffer;
  picInput.num_of_buffers_in_list = 1;
  if (avctx->codec_id == CODEC_ID_H264)
  {
    list[1] = xvba->m_xvbaBufferPool.iq_matrix_buffer;
    picInput.num_of_buffers_in_list = 2;
  }

  { CSingleLock lock(xvba->m_apiSec);
    if (Success != g_XVBA_vtable.DecodePicture(&picInput))
    {
      xvba->SetError(__FUNCTION__, "failed to decode picture 1", __LINE__);
      return;
    }
  }

  if (!xvba->EnsureDataControlBuffers(render->num_slices))
    return;

  XVBADataCtrl *dataControl;
  int location = 0;
  xvba->m_xvbaBufferPool.data_buffer->data_size_in_buffer = 0;
  for (unsigned int j = 0; j < render->num_slices; ++j)
  {
    int startCodeSize = 0;
    uint8_t startCode[] = {0x00,0x00,0x01};
    if (avctx->codec_id == CODEC_ID_H264)
    {
      startCodeSize = 3;
      memcpy((uint8_t*)xvba->m_xvbaBufferPool.data_buffer->bufferXVBA+location,
          startCode, 3);
    }
    else if (avctx->codec_id == CODEC_ID_VC1 &&
        (memcmp(render->buffers[j].buffer, startCode, 3) != 0))
    {
      startCodeSize = 4;
      uint8_t sdf = 0x0d;
      if (render->picture_descriptor->sps_info.vc1.second_field)
        sdf = 0x0c;

      memcpy((uint8_t*)xvba->m_xvbaBufferPool.data_buffer->bufferXVBA+location,
          startCode, 3);
      memcpy((uint8_t*)xvba->m_xvbaBufferPool.data_buffer->bufferXVBA+location+3,
          &sdf, 1);
    }
    // check for potential buffer overwrite
    unsigned int bytesToCopy = render->buffers[j].size;
    unsigned int freeBufferSize = xvba->m_xvbaBufferPool.data_buffer->buffer_size -
        xvba->m_xvbaBufferPool.data_buffer->data_size_in_buffer;
    if (bytesToCopy >= freeBufferSize)
    {
      xvba->SetError(__FUNCTION__, "bitstream buffer too large, maybe corrupted packet", __LINE__);
      return;
    }
    memcpy((uint8_t*)xvba->m_xvbaBufferPool.data_buffer->bufferXVBA+location+startCodeSize,
        render->buffers[j].buffer,
        render->buffers[j].size);
    dataControl = (XVBADataCtrl*)xvba->m_xvbaBufferPool.data_control_buffers[j]->bufferXVBA;
    dataControl->SliceDataLocation = location;
    dataControl->SliceBytesInBuffer = render->buffers[j].size+startCodeSize;
    dataControl->SliceBitsInBuffer = dataControl->SliceBytesInBuffer * 8;
    xvba->m_xvbaBufferPool.data_buffer->data_size_in_buffer += dataControl->SliceBytesInBuffer;
    location += dataControl->SliceBytesInBuffer;
  }

  int bufSize = xvba->m_xvbaBufferPool.data_buffer->data_size_in_buffer;
  int padding = bufSize % 128;
  if (padding)
  {
    padding = 128 - padding;
    xvba->m_xvbaBufferPool.data_buffer->data_size_in_buffer += padding;
    memset((uint8_t*)xvba->m_xvbaBufferPool.data_buffer->bufferXVBA+bufSize,0,padding);
  }

  picInput.num_of_buffers_in_list = 2;
  for (unsigned int i = 0; i < render->num_slices; ++i)
  {
    list[0] = xvba->m_xvbaBufferPool.data_buffer;
    list[0]->data_offset = 0;
    list[1] = xvba->m_xvbaBufferPool.data_control_buffers[i];
    list[1]->data_size_in_buffer = sizeof(*dataControl);
    { CSingleLock lock(xvba->m_apiSec);
      if (Success != g_XVBA_vtable.DecodePicture(&picInput))
      {
        xvba->SetError(__FUNCTION__, "failed to decode picture 2", __LINE__);
        return;
      }
    }
  }
  XVBA_Decode_Picture_End_Input endInput;
  endInput.size = sizeof(endInput);
  endInput.session = xvba->m_xvbaConfig.xvbaSession;
  { CSingleLock lock(xvba->m_apiSec);
    if (Success != g_XVBA_vtable.EndDecodePicture(&endInput))
    {
      xvba->SetError(__FUNCTION__, "failed to decode picture 3", __LINE__);
      return;
    }
  }

  // decode sync and error
  XVBA_Surface_Sync_Input syncInput;
  XVBA_Surface_Sync_Output syncOutput;
  syncInput.size = sizeof(syncInput);
  syncInput.session = xvba->m_xvbaConfig.xvbaSession;
  syncInput.surface = render->surface;
  syncInput.query_status = XVBA_GET_SURFACE_STATUS;
  syncOutput.size = sizeof(syncOutput);
  int64_t start = CurrentHostCounter();
  while (1)
  {
    { CSingleLock lock(xvba->m_apiSec);
      if (Success != g_XVBA_vtable.SyncSurface(&syncInput, &syncOutput))
      {
        xvba->SetError(__FUNCTION__, "failed sync surface 1", __LINE__);
        return;
      }
    }
    if (!(syncOutput.status_flags & XVBA_STILL_PENDING))
      break;
    if (CurrentHostCounter() - start > CurrentHostFrequency())
    {
      xvba->SetError(__FUNCTION__, "timed out waiting for surface", __LINE__);
      break;
    }
    usleep(100);
  }
  render->state |= FF_XVBA_STATE_DECODED;
}

int CDecoder::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CDVDVideoCodecFFmpeg* ctx   = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDecoder*             xvba  = (CDecoder*)ctx->GetHardware();

  pic->data[0] =
  pic->data[1] =
  pic->data[2] =
  pic->data[3] = 0;

  pic->linesize[0] =
  pic->linesize[1] =
  pic->linesize[2] =
  pic->linesize[3] = 0;

  CSingleLock lock(xvba->m_decoderSection);

  if(xvba->m_displayState != XVBA_OPEN)
    return -1;

  if (xvba->m_xvbaConfig.xvbaSession == 0)
  {
    if (!xvba->CreateSession(avctx))
      return -1;
  }

  xvba_render_state * render = NULL;
  // find unused surface
  { CSingleLock lock(xvba->m_videoSurfaceSec);
    for(unsigned int i = 0; i < xvba->m_videoSurfaces.size(); ++i)
    {
      if(!(xvba->m_videoSurfaces[i]->state & (FF_XVBA_STATE_USED_FOR_REFERENCE | FF_XVBA_STATE_USED_FOR_RENDER)))
      {
        render = xvba->m_videoSurfaces[i];
        render->state = 0;
        break;
      }
    }
  }

  // create a new render state
  if (render == NULL)
  {
    render = (xvba_render_state*)calloc(sizeof(xvba_render_state), 1);
    if (render == NULL)
    {
      CLog::Log(LOGERROR, "XVBA::FFGetBuffer - calloc failed");
      return -1;
    }
    render->surface = 0;
    render->buffers_alllocated = 0;
    CSingleLock lock(xvba->m_videoSurfaceSec);
    xvba->m_videoSurfaces.push_back(render);
  }

  // create a new surface
  if (render->surface == 0)
  {
    XVBA_Create_Surface_Input surfaceInput;
    XVBA_Create_Surface_Output surfaceOutput;
    surfaceInput.size = sizeof(surfaceInput);
    surfaceInput.surface_type = xvba->m_xvbaConfig.decoderCap.surface_type;
    surfaceInput.width = xvba->m_xvbaConfig.surfaceWidth;
    surfaceInput.height = xvba->m_xvbaConfig.surfaceHeight;
    surfaceInput.session = xvba->m_xvbaConfig.xvbaSession;
    surfaceOutput.size = sizeof(surfaceOutput);
    { CSingleLock lock(xvba->m_apiSec);
      if (Success != g_XVBA_vtable.CreateSurface(&surfaceInput, &surfaceOutput))
      {
        xvba->SetError(__FUNCTION__, "failed to create video surface", __LINE__);
        return -1;
      }
    }
    render->surface = surfaceOutput.surface;
    render->picture_descriptor = (XVBAPictureDescriptor *)xvba->m_xvbaBufferPool.picture_descriptor_buffer->bufferXVBA;
    render->iq_matrix = (XVBAQuantMatrixAvc *)xvba->m_xvbaBufferPool.iq_matrix_buffer->bufferXVBA;
    CLog::Log(LOGDEBUG, "XVBA::FFGetBuffer - created video surface");
  }

  if (render == NULL)
    return -1;

  pic->data[0] = (uint8_t*)render;

  pic->type= FF_BUFFER_TYPE_USER;

  render->state |= FF_XVBA_STATE_USED_FOR_REFERENCE;
  render->state &= ~FF_XVBA_STATE_DECODED;
  render->psf = 0;
  pic->reordered_opaque= avctx->reordered_opaque;

  return 0;
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  int result = Check(avctx);
  if (result)
    return result;

  CSingleLock lock(m_decoderSection);

  if(frame)
  { // we have a new frame from decoder

    xvba_render_state * render = (xvba_render_state*)frame->data[0];
    if(!render)
    {
      CLog::Log(LOGERROR, "XVBA::Decode - no render buffer");
      return VC_ERROR;
    }

    // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
    if (!IsSurfaceValid(render))
    {
      CLog::Log(LOGWARNING, "XVBA::Decode - ignoring invalid buffer");
      return VC_BUFFER;
    }
    if (!(render->state & FF_XVBA_STATE_DECODED))
    {
      CLog::Log(LOGDEBUG, "XVBA::Decode - ffmpeg failed");
      return VC_BUFFER;
    }

    CSingleLock lock(m_videoSurfaceSec);
    render->state |= FF_XVBA_STATE_USED_FOR_RENDER;
    lock.Leave();

    // send frame to output for processing
    CXvbaDecodedPicture pic;
    memset(&pic.DVDPic, 0, sizeof(pic.DVDPic));
    ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(&pic.DVDPic);
    pic.render = render;
    if (render->psf)
      pic.DVDPic.iFlags &= ~DVP_FLAG_INTERLACED;
    m_bufferStats.IncDecoded();
    m_xvbaOutput.m_dataPort.SendOutMessage(COutputDataProtocol::NEWFRAME, &pic, sizeof(pic));

    m_codecControl = pic.DVDPic.iFlags & (DVP_FLAG_DRAIN | DVP_FLAG_NO_POSTPROC);
  }

  int retval = 0;
  uint16_t decoded, processed, render;
  Message *msg;
  while (m_xvbaOutput.m_controlPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputControlProtocol::ERROR)
    {
      m_displayState = XVBA_ERROR;
      retval |= VC_ERROR;
    }
    msg->Release();
  }

  m_bufferStats.Get(decoded, processed, render);

  uint64_t startTime = CurrentHostCounter();
  while (!retval)
  {
    if (m_xvbaOutput.m_dataPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputDataProtocol::PICTURE)
      {
        if (m_presentPicture)
        {
          m_presentPicture->ReturnUnused();
          m_presentPicture = 0;
        }

        m_presentPicture = *(CXvbaRenderPicture**)msg->data;
        m_presentPicture->xvba = this;
        m_bufferStats.DecRender();
        m_bufferStats.Get(decoded, processed, render);
        retval |= VC_PICTURE;
      }
      msg->Release();
    }
    else if (m_xvbaOutput.m_controlPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputControlProtocol::STATS)
      {
        m_bufferStats.Get(decoded, processed, render);
      }
      else
      {
        m_displayState = XVBA_ERROR;
        retval |= VC_ERROR;
      }
      msg->Release();
    }

    if ((m_codecControl & DVP_FLAG_DRAIN))
    {
      if (decoded + processed + render < 2)
      {
        retval |= VC_BUFFER;
      }
    }
    else
    {
      if (decoded + processed + render < 4)
      {
        retval |= VC_BUFFER;
      }
    }

    if (!retval && !m_inMsgEvent.WaitMSec(2000))
      break;
  }
  uint64_t diff = CurrentHostCounter() - startTime;
  if (retval & VC_PICTURE)
  {
    m_bufferStats.SetParams(diff, m_speed);
    if (diff*1000/CurrentHostFrequency() > 50)
      CLog::Log(LOGDEBUG,"XVBA::Decode long wait: %d", (int)((diff*1000)/CurrentHostFrequency()));
  }

  if (!retval)
  {
    CLog::Log(LOGERROR, "XVBA::%s - timed out waiting for output message", __FUNCTION__);
    m_displayState = XVBA_ERROR;
    retval |= VC_ERROR;
  }

  return retval;

}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  CSingleLock lock(m_decoderSection);

  if (m_displayState != XVBA_OPEN)
    return false;

  *picture = m_presentPicture->DVDPic;
  picture->xvba = m_presentPicture;

  return true;
}

void CDecoder::ReturnRenderPicture(CXvbaRenderPicture *renderPic)
{
  m_xvbaOutput.m_dataPort.SendOutMessage(COutputDataProtocol::RETURNPIC, &renderPic, sizeof(renderPic));
}


//void CDecoder::CopyYV12(int index, uint8_t *dest)
//{
//  CSharedLock lock(m_decoderSection);
//
//  { CSharedLock dLock(m_displaySection);
//    if(m_displayState != XVBA_OPEN)
//      return;
//  }
//
//  if (!m_flipBuffer[index].outPic)
//  {
//    CLog::Log(LOGWARNING, "XVBA::Present: present picture is NULL");
//    return;
//  }
//
//  XVBA_GetSurface_Target target;
//  target.size = sizeof(target);
//  target.surfaceType = XVBA_YV12;
//  target.flag = XVBA_FRAME;
//
//  XVBA_Get_Surface_Input input;
//  input.size = sizeof(input);
//  input.session = m_xvbaSession;
//  input.src_surface = m_flipBuffer[index].outPic->render->surface;
//  input.target_buffer = dest;
//  input.target_pitch = m_surfaceWidth;
//  input.target_width = m_surfaceWidth;
//  input.target_height = m_surfaceHeight;
//  input.target_parameter = target;
//  { CSingleLock lock(m_apiSec);
//    if (Success != g_XVBA_vtable.GetSurface(&input))
//    {
//      CLog::Log(LOGERROR,"(XVBA::CopyYV12) failed to get  surface");
//    }
//  }
//}

void CDecoder::Reset()
{
  CSingleLock lock(m_decoderSection);

  if (!m_xvbaConfig.xvbaSession)
    return;

  Message *reply;
  if (m_xvbaOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::FLUSH,
                                                 &reply,
                                                 2000))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "XVBA::%s - flush returned error", __FUNCTION__);
      m_displayState = XVBA_ERROR;
    }
    else
      m_bufferStats.Reset();
  }
  else
  {
    CLog::Log(LOGERROR, "XVBA::%s - flush timed out", __FUNCTION__);
    m_displayState = XVBA_ERROR;
  }
}

bool CDecoder::CanSkipDeint()
{
  return m_bufferStats.CanSkipDeint();
}

void CDecoder::SetSpeed(int speed)
{
  m_speed = speed;
}

//-----------------------------------------------------------------------------
// RenderPicture
//-----------------------------------------------------------------------------

CXvbaRenderPicture* CXvbaRenderPicture::Acquire()
{
  CSingleLock lock(*renderPicSection);

  if (refCount == 0)
    xvba->Acquire();

  refCount++;
  return this;
}

long CXvbaRenderPicture::Release()
{
  CSingleLock lock(*renderPicSection);

  refCount--;
  if (refCount > 0)
    return refCount;

  lock.Leave();
  xvba->ReturnRenderPicture(this);
  xvba->ReleasePicReference();

  return refCount;
}

void CXvbaRenderPicture::ReturnUnused()
{
  { CSingleLock lock(*renderPicSection);
    if (refCount > 0)
      return;
  }
  if (xvba)
    xvba->ReturnRenderPicture(this);
}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------
COutput::COutput(CEvent *inMsgEvent) :
  CThread("XVBA Output Thread"),
  m_controlPort("OutputControlPort", inMsgEvent, &m_outMsgEvent),
  m_dataPort("OutputDataPort", inMsgEvent, &m_outMsgEvent)
{
  m_inMsgEvent = inMsgEvent;

  CXvbaRenderPicture pic;
  pic.renderPicSection = &m_bufferPool.renderPicSec;
  pic.refCount = 0;
  for (unsigned int i = 0; i < NUM_RENDER_PICS; i++)
  {
    m_bufferPool.allRenderPics.push_back(pic);
  }
  for (unsigned int i = 0; i < m_bufferPool.allRenderPics.size(); ++i)
  {
    m_bufferPool.freeRenderPics.push_back(&m_bufferPool.allRenderPics[i]);
  }
}

void COutput::Start()
{
  Create();
}

COutput::~COutput()
{
  Dispose();

  m_bufferPool.freeRenderPics.clear();
  m_bufferPool.usedRenderPics.clear();
  m_bufferPool.allRenderPics.clear();
}

void COutput::Dispose()
{
  CSingleLock lock(g_graphicsContext);
  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();
  m_controlPort.Purge();
  m_dataPort.Purge();
}

void COutput::OnStartup()
{
  CLog::Log(LOGNOTICE, "COutput::OnStartup: Output Thread created");
}

void COutput::OnExit()
{
  CLog::Log(LOGNOTICE, "COutput::OnExit: Output Thread terminated");
}

enum OUTPUT_STATES
{
  O_TOP = 0,                      // 0
  O_TOP_ERROR,                    // 1
  O_TOP_UNCONFIGURED,             // 2
  O_TOP_CONFIGURED,               // 3
  O_TOP_CONFIGURED_WAIT_RES1,     // 4
  O_TOP_CONFIGURED_WAIT_DEC,      // 5
  O_TOP_CONFIGURED_STEP1,         // 6
  O_TOP_CONFIGURED_WAIT_RES2,     // 7
  O_TOP_CONFIGURED_STEP2,         // 8
};

int OUTPUT_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    3, //TOP_CONFIGURED_WAIT_RES1
    3, //TOP_CONFIGURED_WAIT_DEC
    3, //TOP_CONFIGURED_STEP1
    3, //TOP_CONFIGURED_WAIT_RES2
    3, //TOP_CONFIGURED_STEP2
};

void COutput::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = OUTPUT_parentStates[state])
  {
    switch (state)
    {
    case O_TOP: // TOP
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::FLUSH:
          msg->Reply(COutputControlProtocol::ACC);
          return;
        case COutputControlProtocol::PRECLEANUP:
          msg->Reply(COutputControlProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case COutputDataProtocol::RETURNPIC:
          CXvbaRenderPicture *pic;
          pic = *((CXvbaRenderPicture**)msg->data);
          ProcessReturnPicture(pic);
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "COutput::%s - signal: %d form port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case O_TOP_ERROR:
      m_extTimeout = 1000;
      break;

    case O_TOP_UNCONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::INIT:
          CXvbaConfig *data;
          data = (CXvbaConfig*)msg->data;
          if (data)
          {
            m_config = *data;
          }
          Init();
          EnsureBufferPool();
          if (!m_xvbaError)
          {
            m_state = O_TOP_CONFIGURED_WAIT_RES1;
            msg->Reply(COutputControlProtocol::ACC);
          }
          else
          {
            m_state = O_TOP_ERROR;
            msg->Reply(COutputControlProtocol::ERROR);
          }
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::FLUSH:
          m_state = O_TOP_CONFIGURED_WAIT_RES1;
          Flush();
          msg->Reply(COutputControlProtocol::ACC);
          return;
        case COutputControlProtocol::PRECLEANUP:
          m_state = O_TOP_UNCONFIGURED;
          m_extTimeout = 10000;
          Flush();
          ReleaseBufferPool(true);
          msg->Reply(COutputControlProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case COutputDataProtocol::NEWFRAME:
          CXvbaDecodedPicture *frame;
          frame = (CXvbaDecodedPicture*)msg->data;
          if (frame)
          {
            m_decodedPics.push(*frame);
            m_extTimeout = 0;
          }
          return;
        case COutputDataProtocol::RETURNPIC:
          CXvbaRenderPicture *pic;
          pic = *((CXvbaRenderPicture**)msg->data);
          ProcessReturnPicture(pic);
          m_controlPort.SendInMessage(COutputControlProtocol::STATS);
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_WAIT_RES1:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
          if (!m_decodedPics.empty() && FindFreeSurface() >= 0 && !m_bufferPool.freeRenderPics.empty())
          {
            m_state = O_TOP_CONFIGURED_WAIT_DEC;
            m_bStateMachineSelfTrigger = true;
          }
          else
          {
            if (m_extTimeout != 0)
            {
              uint16_t decoded, processed, render;
              m_config.stats->Get(decoded, processed, render);
//              CLog::Log(LOGDEBUG, "CVDPAU::COutput - timeout idle: decoded: %d, proc: %d, render: %d", decoded, processed, render);
            }
            m_extTimeout = 100;
          }
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_WAIT_DEC:
       if (port == NULL) // timeout
       {
         switch (signal)
         {
         case COutputControlProtocol::TIMEOUT:
           if (IsDecodingFinished())
           {
             m_state = O_TOP_CONFIGURED_STEP1;
             m_bStateMachineSelfTrigger = true;
           }
           else
           {
             m_extTimeout = 1;
           }
           return;
         default:
           break;
         }
       }
       break;

    case O_TOP_CONFIGURED_STEP1:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
          m_processPicture = m_decodedPics.front();
          m_decodedPics.pop();
          InitCycle();
          CXvbaRenderPicture *pic;
          pic = ProcessPicture();
          if (pic)
          {
            m_config.stats->IncRender();
            m_dataPort.SendInMessage(COutputDataProtocol::PICTURE, &pic, sizeof(pic));
          }
          if (m_xvbaError)
          {
            m_state = O_TOP_ERROR;
            return;
          }
          if (m_deinterlacing && !m_deintSkip)
          {
            m_state = O_TOP_CONFIGURED_WAIT_RES2;
            m_extTimeout = 0;
          }
          else
          {
            FiniCycle();
            m_state = O_TOP_CONFIGURED_WAIT_RES1;
            m_extTimeout = 0;
          }
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_WAIT_RES2:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
          if (FindFreeSurface() >= 0 && !m_bufferPool.freeRenderPics.empty())
          {
            m_state = O_TOP_CONFIGURED_STEP2;
            m_bStateMachineSelfTrigger = true;
          }
          else
          {
            if (m_extTimeout != 0)
            {
              uint16_t decoded, processed, render;
              m_config.stats->Get(decoded, processed, render);
              CLog::Log(LOGDEBUG, "CVDPAU::COutput - timeout idle: decoded: %d, proc: %d, render: %d", decoded, processed, render);
            }
            m_extTimeout = 100;
          }
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_STEP2:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
          CXvbaRenderPicture *pic;
          m_deintStep = 1;
          pic = ProcessPicture();
          if (pic)
          {
            m_config.stats->IncRender();
            m_dataPort.SendInMessage(COutputDataProtocol::PICTURE, &pic, sizeof(pic));
          }
          if (m_xvbaError)
          {
            m_state = O_TOP_ERROR;
            return;
          }
          FiniCycle();
          m_state = O_TOP_CONFIGURED_WAIT_RES1;
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "COutput::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void COutput::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;

  m_state = O_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;

  while (!m_bStop)
  {
    gotMsg = false;

    if (m_bStateMachineSelfTrigger)
    {
      m_bStateMachineSelfTrigger = false;
      // self trigger state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }
    // check control port
    else if (m_controlPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_controlPort;
    }
    // check data port
    else if (m_dataPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_dataPort;
    }
    if (gotMsg)
    {
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }

    // wait for message
    else if (m_outMsgEvent.WaitMSec(m_extTimeout))
    {
      continue;
    }
    // time out
    else
    {
      msg = m_controlPort.GetMessage();
      msg->signal = COutputControlProtocol::TIMEOUT;
      port = 0;
      // signal timeout to state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
    }
  }
  Flush();
  Uninit();
}

bool COutput::Init()
{
  if (!CreateGlxContext())
    return false;

  m_xvbaError = false;
  m_processPicture.render = 0;
  m_fence = None;

  return true;
}

bool COutput::Uninit()
{
  ReleaseBufferPool();
  DestroyGlxContext();
  return true;
}

void COutput::Flush()
{
  while (!m_decodedPics.empty())
  {
    CXvbaDecodedPicture pic = m_decodedPics.front();
    m_decodedPics.pop();
    CSingleLock lock(*m_config.videoSurfaceSec);
    if (pic.render)
      pic.render->state &= ~(FF_XVBA_STATE_USED_FOR_RENDER | FF_XVBA_STATE_DECODED);
  }

  if (m_processPicture.render)
  {
    CSingleLock lock(*m_config.videoSurfaceSec);
    m_processPicture.render->state &= ~(FF_XVBA_STATE_USED_FOR_RENDER | FF_XVBA_STATE_DECODED);
    m_processPicture.render = 0;
  }

  Message *msg;
  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::NEWFRAME)
    {
      CXvbaDecodedPicture pic = *(CXvbaDecodedPicture*)msg->data;
      CSingleLock lock(*m_config.videoSurfaceSec);
      if (pic.render)
        pic.render->state &= ~(FF_XVBA_STATE_USED_FOR_RENDER | FF_XVBA_STATE_DECODED);
    }
    else if (msg->signal == COutputDataProtocol::RETURNPIC)
    {
      CXvbaRenderPicture *pic;
      pic = *((CXvbaRenderPicture**)msg->data);
      ProcessReturnPicture(pic);
    }
    msg->Release();
  }

  while (m_dataPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::PICTURE)
    {
      CXvbaRenderPicture *pic;
      pic = *((CXvbaRenderPicture**)msg->data);
      ProcessReturnPicture(pic);
    }
  }
}

bool COutput::IsDecodingFinished()
{
  // check for decoding to be finished
  CXvbaDecodedPicture decodedPic = m_decodedPics.front();

  XVBA_Surface_Sync_Input syncInput;
  XVBA_Surface_Sync_Output syncOutput;
  syncInput.size = sizeof(syncInput);
  syncInput.session = m_config.xvbaSession;
  syncInput.surface = decodedPic.render->surface;
  syncInput.query_status = XVBA_GET_SURFACE_STATUS;
  syncOutput.size = sizeof(syncOutput);
  { CSingleLock lock(*(m_config.apiSec));
    if (Success != g_XVBA_vtable.SyncSurface(&syncInput, &syncOutput))
    {
      CLog::Log(LOGERROR,"XVBA - failed sync surface");
      m_xvbaError = true;
      return false;
    }
  }
  if (!(syncOutput.status_flags & XVBA_STILL_PENDING))
    return true;

  return false;
}

CXvbaRenderPicture* COutput::ProcessPicture()
{
  CXvbaRenderPicture *retPic = 0;

  if (m_deintStep == 1)
  {
    if(m_field == XVBA_TOP_FIELD)
      m_field = XVBA_BOTTOM_FIELD;
    else
      m_field = XVBA_TOP_FIELD;
  }

  // find unused shared surface
  unsigned int idx = FindFreeSurface();
  XvbaBufferPool::GLVideoSurface *glSurface = &m_bufferPool.glSurfaces[idx];
  glSurface->used = true;
  glSurface->field = m_field;
  glSurface->render = m_processPicture.render;
  glSurface->transferred = false;

  int cmd = 0;
  m_config.stats->GetCmd(cmd);

//  if (m_fence)
//    glDeleteSync(m_fence);
//  m_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

  // transfer surface
  XVBA_Transfer_Surface_Input transInput;
  transInput.size = sizeof(transInput);
  transInput.session = m_config.xvbaSession;
  transInput.src_surface = m_processPicture.render->surface;
  transInput.target_surface = glSurface->glSurface;
  transInput.flag = m_field;
  { CSingleLock lock(*(m_config.apiSec));
    if (Success != g_XVBA_vtable.TransferSurface(&transInput))
    {
      CLog::Log(LOGERROR,"(XVBA) failed to transfer surface");
      m_xvbaError = true;
      return retPic;
    }
  }

  // prepare render pic
  retPic = m_bufferPool.freeRenderPics.front();
  m_bufferPool.freeRenderPics.pop_front();
  m_bufferPool.usedRenderPics.push_back(retPic);
  retPic->sourceIdx = glSurface->id;
  retPic->DVDPic = m_processPicture.DVDPic;
  retPic->valid = true;
  retPic->texture = glSurface->texture;
  retPic->crop = CRect(0,0,0,0);
  retPic->texWidth = m_config.surfaceWidth;
  retPic->texHeight = m_config.surfaceHeight;
  retPic->xvbaOutput = this;

  // set repeat pic for de-interlacing
  if (m_deinterlacing)
  {
    if (m_deintStep == 1)
    {
      retPic->DVDPic.pts = DVD_NOPTS_VALUE;
      retPic->DVDPic.dts = DVD_NOPTS_VALUE;
    }
    retPic->DVDPic.iRepeatPicture = 0.0;
  }

  return retPic;
}

void COutput::ProcessReturnPicture(CXvbaRenderPicture *pic)
{
  std::deque<CXvbaRenderPicture*>::iterator it;
  it = std::find(m_bufferPool.usedRenderPics.begin(), m_bufferPool.usedRenderPics.end(), pic);
  if (it == m_bufferPool.usedRenderPics.end())
  {
    CLog::Log(LOGWARNING, "COutput::ProcessReturnPicture - pic not found");
    return;
  }
  m_bufferPool.usedRenderPics.erase(it);
  m_bufferPool.freeRenderPics.push_back(pic);
  if (!pic->valid)
  {
    CLog::Log(LOGDEBUG, "COutput::%s - return of invalid render pic", __FUNCTION__);
    return;
  }

  xvba_render_state *render = m_bufferPool.glSurfaces[pic->sourceIdx].render;
  if (render)
  {
    // check if video surface is referenced by other glSurfaces
    bool referenced(false);
    for (unsigned int i=0; i<m_bufferPool.glSurfaces.size();++i)
    {
      if (i == pic->sourceIdx)
        continue;
      if (m_bufferPool.glSurfaces[i].render == render)
      {
        referenced = true;
        break;
      }
    }
    if (m_processPicture.render == render)
      referenced = true;

    // release video surface
    if (!referenced)
    {
      CSingleLock lock(*m_config.videoSurfaceSec);
      render->state &= ~(FF_XVBA_STATE_USED_FOR_RENDER | FF_XVBA_STATE_DECODED);
    }

    // unreference video surface
    m_bufferPool.glSurfaces[pic->sourceIdx].render = 0;

    m_bufferPool.glSurfaces[pic->sourceIdx].used = false;
    return;
  }
}

int COutput::FindFreeSurface()
{
  // find free shared surface
  unsigned int i;
  for (i = 0; i < m_bufferPool.glSurfaces.size(); ++i)
  {
    if (!m_bufferPool.glSurfaces[i].used)
      break;
  }
  if (i == m_bufferPool.glSurfaces.size())
    return -1;
  else
    return i;
}

void COutput::InitCycle()
{
  uint64_t latency;
  int speed;
  m_config.stats->GetParams(latency, speed);
  latency = (latency*1000)/CurrentHostFrequency();

  m_config.stats->SetCanSkipDeint(false);

  EDEINTERLACEMODE   mode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
  EINTERLACEMETHOD method = CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod;
  bool interlaced = m_processPicture.DVDPic.iFlags & DVP_FLAG_INTERLACED;

  if (mode == VS_DEINTERLACEMODE_FORCE ||
     (mode == VS_DEINTERLACEMODE_AUTO && interlaced))
  {
    if((method == VS_INTERLACEMETHOD_AUTO && interlaced)
      ||  method == VS_INTERLACEMETHOD_XVBA)
    {
      m_deinterlacing = true;
      m_deintSkip = false;
      m_config.stats->SetCanSkipDeint(true);

      if (m_processPicture.DVDPic.iFlags & DVP_FLAG_DROPDEINT)
      {
        m_deintSkip = true;
      }

      // do only half deinterlacing
      if (speed != DVD_PLAYSPEED_NORMAL || !g_graphicsContext.IsFullScreenVideo())
      {
        m_config.stats->SetCanSkipDeint(false);
        m_deintSkip = true;
      }

      if(m_processPicture.DVDPic.iFlags & DVP_FLAG_TOP_FIELD_FIRST)
        m_field = XVBA_TOP_FIELD;
      else
        m_field = XVBA_BOTTOM_FIELD;
    }
  }
  else
  {
    m_deinterlacing = false;
    m_field = XVBA_FRAME;
  }

  m_processPicture.DVDPic.format = RENDER_FMT_XVBA;
  m_processPicture.DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                                    DVP_FLAG_REPEAT_TOP_FIELD |
                                    DVP_FLAG_INTERLACED);
  m_processPicture.DVDPic.iWidth = m_config.vidWidth;
  m_processPicture.DVDPic.iHeight = m_config.vidHeight;

  m_deintStep = 0;
}

void COutput::FiniCycle()
{
//  { CSingleLock lock(*m_config.videoSurfaceSec);
//    m_processPicture.render->state &= ~FF_XVBA_STATE_USED_FOR_RENDER;
//  }
  m_processPicture.render = 0;
  m_config.stats->DecDecoded();
}

bool COutput::EnsureBufferPool()
{
  if (m_config.useSharedSurfaces && m_bufferPool.glSurfaces.empty())
  {
    GLenum textureTarget;
    if (!glewIsSupported("GL_ARB_texture_non_power_of_two") && glewIsSupported("GL_ARB_texture_rectangle"))
    {
      textureTarget = GL_TEXTURE_RECTANGLE_ARB;
    }
    else
      textureTarget = GL_TEXTURE_2D;

    // create shared surfaces
    XvbaBufferPool::GLVideoSurface surface;
    for (unsigned int i = 0; i < NUM_RENDER_PICS; ++i)
    {
      glEnable(textureTarget);
      glGenTextures(1, &surface.texture);
      glBindTexture(textureTarget, surface.texture);
      glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
      glTexImage2D(textureTarget, 0, GL_RGBA, m_config.surfaceWidth, m_config.surfaceHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

      XVBA_Create_GLShared_Surface_Input surfInput;
      XVBA_Create_GLShared_Surface_Output surfOutput;
      surfInput.size = sizeof(surfInput);
      surfInput.session = m_config.xvbaSession;
      surfInput.gltexture = surface.texture;
      surfInput.glcontext = m_glContext;
      surfOutput.size = sizeof(surfOutput);
      surfOutput.surface = 0;
      if (Success != g_XVBA_vtable.CreateGLSharedSurface(&surfInput, &surfOutput))
      {
        CLog::Log(LOGERROR,"(XVBA) failed to create shared surface");
        m_xvbaError = true;
        break;
      }
      CLog::Log(LOGDEBUG, "XVBA::GetTexture - created shared surface");

      surface.glSurface = surfOutput.surface;
      surface.id = i;
      surface.used = false;
      surface.render = 0;
      m_bufferPool.glSurfaces.push_back(surface);
    }
    glDisable(textureTarget);
  }

  return true;
}

void COutput::ReleaseBufferPool(bool precleanup /*= false*/)
{
//  if (m_fence)
//  {
//    uint64_t maxTimeout = 1000000000LL;
//    glClientWaitSync(m_fence, GL_SYNC_FLUSH_COMMANDS_BIT, maxTimeout);
//    glDeleteSync(m_fence);
//    m_fence = None;
//  }

  CSingleLock lock(m_bufferPool.renderPicSec);

  for (unsigned int i = 0; i < m_bufferPool.glSurfaces.size(); ++i)
  {
    if (m_bufferPool.glSurfaces[i].glSurface)
    {
      g_XVBA_vtable.DestroySurface(m_bufferPool.glSurfaces[i].glSurface);
      m_bufferPool.glSurfaces[i].glSurface = 0;
    }
    if (m_bufferPool.glSurfaces[i].texture && !precleanup)
    {
      glDeleteTextures(1, &m_bufferPool.glSurfaces[i].texture);
      m_bufferPool.glSurfaces[i].texture = 0;
    }
    m_bufferPool.glSurfaces[i].render = 0;
    m_bufferPool.glSurfaces[i].used = true;
  }

  if (!precleanup)
  {
    m_bufferPool.glSurfaces.clear();

    // invalidate all used render pictures
    for (unsigned int i = 0; i < m_bufferPool.usedRenderPics.size(); ++i)
    {
      m_bufferPool.usedRenderPics[i]->valid = false;
    }
  }
}

void COutput::PreReleaseBufferPool()
{
  CSingleLock lock(m_bufferPool.renderPicSec);

  if (m_config.useSharedSurfaces)
  {
    for (unsigned int i = 0; i < m_bufferPool.glSurfaces.size(); ++i)
    {
      if (!m_bufferPool.glSurfaces[i].used)
      {
        g_XVBA_vtable.DestroySurface(m_bufferPool.glSurfaces[i].glSurface);
        glDeleteTextures(1, &m_bufferPool.glSurfaces[i].texture);
        m_bufferPool.glSurfaces[i].glSurface = 0;
        m_bufferPool.glSurfaces[i].used = true;
      }
    }
  }
}

bool COutput::CreateGlxContext()
{
  GLXContext   glContext;

  m_Display = g_Windowing.GetDisplay();
  glContext = g_Windowing.GetGlxContext();
  m_Window = g_Windowing.GetWindow();

  // Get our window attribs.
  XWindowAttributes wndattribs;
  XGetWindowAttributes(m_Display, m_Window, &wndattribs);

  // Get visual Info
  XVisualInfo visInfo;
  visInfo.visualid = wndattribs.visual->visualid;
  int nvisuals = 0;
  XVisualInfo* visuals = XGetVisualInfo(m_Display, VisualIDMask, &visInfo, &nvisuals);
  if (nvisuals != 1)
  {
    CLog::Log(LOGERROR, "XVBA::COutput::CreateGlxContext - could not find visual");
    return false;
  }
  visInfo = visuals[0];
  XFree(visuals);

  m_pixmap = XCreatePixmap(m_Display,
                           m_Window,
                           192,
                           108,
                           visInfo.depth);
  if (!m_pixmap)
  {
    CLog::Log(LOGERROR, "XVBA::COutput::CreateGlxContext - Unable to create XPixmap");
    return false;
  }

  // create gl pixmap
  m_glPixmap = glXCreateGLXPixmap(m_Display, &visInfo, m_pixmap);

  if (!m_glPixmap)
  {
    CLog::Log(LOGINFO, "XVBA::COutput::CreateGlxContext - Could not create glPixmap");
    return false;
  }

  m_glContext = glXCreateContext(m_Display, &visInfo, glContext, True);

  if (!glXMakeCurrent(m_Display, m_glPixmap, m_glContext))
  {
    CLog::Log(LOGINFO, "XVBA::COutput::CreateGlxContext - Could not make Pixmap current");
    return false;
  }

  CLog::Log(LOGNOTICE, "XVBA::COutput::CreateGlxContext - created context");
  return true;
}

bool COutput::DestroyGlxContext()
{
  if (m_glContext)
  {
    glXMakeCurrent(m_Display, None, NULL);
    glXDestroyContext(m_Display, m_glContext);
  }
  m_glContext = 0;

  if (m_glPixmap)
    glXDestroyPixmap(m_Display, m_glPixmap);
  m_glPixmap = 0;

  if (m_pixmap)
    XFreePixmap(m_Display, m_pixmap);
  m_pixmap = 0;

  return true;
}

#endif
