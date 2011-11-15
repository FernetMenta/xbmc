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
#include <dlfcn.h>
#include "XVBA.h"
#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"

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

CXVBAContext::CXVBAContext()
{
  m_context = 0;
  m_dlHandle = 0;
  m_xvbaContext = 0;
  m_refCount = 0;
  m_ctxId = 0;
  m_displayState = XVBA_RESET;
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
  if (m_dlHandle)
  {
    dlclose(m_dlHandle);
    m_dlHandle = 0;
  }
  g_Windowing.Unregister(m_context);
}

bool CXVBAContext::EnsureContext(CXVBAContext **ctx, int *ctxId)
{
  CSingleLock lock(m_section);

  if (!m_context)
  {
    m_context = new CXVBAContext();
    g_Windowing.Register(m_context);
  }

  EDisplayState state;
  { CSharedLock l(*m_context);
    state = m_context->m_displayState;
  }
  if (state == XVBA_LOST)
  {
    CLog::Log(LOGNOTICE,"XVBA::EnsureContextCVDPAU::Check waiting for display reset event");
    if (!m_context->m_displayEvent.WaitMSec(2000))
    {
      CLog::Log(LOGERROR, "XVBA::EnsureContext - device didn't reset in reasonable time");
      return false;
    }
    { CSharedLock l(*m_context);
      state = m_context->m_displayState;
    }
  }
  if (state == XVBA_RESET)
  {
    CSingleLock gLock(g_graphicsContext);
    CExclusiveLock l(*m_context);
    if (!m_context->LoadSymbols() || !m_context->CreateContext())
      return false;
    m_context->m_displayState = XVBA_OPEN;
  }

  if (*ctx == 0)
    m_context->m_refCount++;

  *ctx = m_context;
  *ctxId = m_context->m_ctxId;
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

  Display *disp;
  Drawable window;
  { CSingleLock lock(g_graphicsContext);
    disp = g_Windowing.GetDisplay();
    window = DefaultRootWindow(disp);
  }

  int version;
  if (!g_XVBA_vtable.QueryExtension(disp, &version))
    return false;
  CLog::Log(LOGNOTICE,"XVBA::CreateContext - opening xvba version: %i", version);

  // create XVBA Context
  XVBA_Create_Context_Input contextInput;
  XVBA_Create_Context_Output contextOutput;
  contextInput.size = sizeof(contextInput);
  contextInput.display = disp;
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
  m_ctxId++;
}

void CXVBAContext::OnLostDevice()
{
  CLog::Log(LOGNOTICE,"CXVBAContext::OnLostDevice event");

  CExclusiveLock lock(*this);
  DestroyContext();
  m_displayState = XVBA_LOST;
  m_displayEvent.Reset();
}

void CXVBAContext::OnResetDevice()
{
  CLog::Log(LOGNOTICE,"CXVBAContext::OnResetDevice event");

  CExclusiveLock lock(*this);
  if (m_displayState == XVBA_LOST)
  {
    m_displayState = XVBA_RESET;
    m_displayEvent.Set();
  }
}

bool CXVBAContext::IsValid(int ctxId)
{
  CSharedLock lock(*this);
  if (m_displayState == XVBA_OPEN && ctxId == m_ctxId)
    return true;
  else
    return false;
}

void *CXVBAContext::GetContext()
{
  return m_xvbaContext;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CDecoder::CDecoder()
{
  m_context = 0;
  m_xvbaSession = 0;
  m_flipBuffer = 0;
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

bool CDecoder::Open(AVCodecContext* avctx, const enum PixelFormat fmt, unsigned int surfaces)
{
  CLog::Log(LOGNOTICE,"(XVBA::Open) opening dxva decoder");

  if(avctx->coded_width  == 0
  || avctx->coded_height == 0)
  {
    CLog::Log(LOGWARNING,"(XVBA) no width/height available, can't init");
    return false;
  }

  if (!m_dllAvUtil.Load())
    return false;

  if (!CXVBAContext::EnsureContext(&m_context, &m_ctxId))
    return false;

  CSharedLock lock(*m_context);

  // xvba get session info
  XVBA_GetSessionInfo_Input sessionInput;
  XVBA_GetSessionInfo_Output sessionOutput;
  sessionInput.size = sizeof(sessionInput);
  sessionInput.context = m_context->GetContext();
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
  capInput.context = m_context->GetContext();
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
    for (int i = 0; i < capOutput->num_of_decodecaps; ++i)
    {
      if (capOutput->decode_caps_list[i].capability_id == XVBA_H264 &&
          capOutput->decode_caps_list[i].flags == XVBA_H264_HIGH)
      {
        match = i;
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
    for (int i = 0; i < capOutput->num_of_decodecaps; ++i)
    {
      if (capOutput->decode_caps_list[i].capability_id == XVBA_VC1 &&
          capOutput->decode_caps_list[i].flags == XVBA_VC1_ADVANCED)
      {
        match = i;
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
    for (int i = 0; i < capOutput->num_of_decodecaps; ++i)
    {
      if (capOutput->decode_caps_list[i].capability_id == XVBA_MPEG2_VLD)
      {
        // XXX: uncomment when implemented
//        match = i;
//        break;
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
    for (int i = 0; i < capOutput->num_of_decodecaps; ++i)
    {
      if (capOutput->decode_caps_list[i].capability_id == XVBA_VC1 &&
          capOutput->decode_caps_list[i].flags == XVBA_VC1_MAIN)
      {
        match = i;
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

  m_decoderCap = capOutput->decode_caps_list[match];

  free(capOutput);

  // set some varables
  m_xvbaSession = 0;
  m_xvbaBufferPool.data_buffer = 0;
  m_xvbaBufferPool.iq_matrix_buffer = 0;
  m_xvbaBufferPool.picture_descriptor_buffer = 0;
  picAge.b_age = picAge.ip_age[0] = picAge.ip_age[1] = 256*256*256*64;
  m_surfaceWidth = 0;
  m_surfaceHeight = 0;
  m_presentPicture = 0;
  m_numRenderBuffers = surfaces;
  m_flipBuffer = new RenderPicture[m_numRenderBuffers];
  for (unsigned int i = 0; i < m_numRenderBuffers; ++i)
  {
    m_flipBuffer[i].outPic = 0;
    m_flipBuffer[i].glSurface[0] =
    m_flipBuffer[i].glSurface[1] =
    m_flipBuffer[i].glSurface[2] = 0;
    m_flipBuffer[i].glTexture[0] =
    m_flipBuffer[i].glTexture[1] =
    m_flipBuffer[i].glTexture[2] = 0;
  }
  for (unsigned int j = 0; j < NUM_OUTPUT_PICS; ++j)
    m_freeOutPic.push_back(&m_allOutPic[j]);

  // setup ffmpeg
  avctx->thread_count    = 1;
  avctx->get_buffer      = CDecoder::FFGetBuffer;
  avctx->release_buffer  = CDecoder::FFReleaseBuffer;
  avctx->draw_horiz_band = CDecoder::FFDrawSlice;
  avctx->slice_flags     = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
  return true;
}

void CDecoder::Close()
{
  CLog::Log(LOGNOTICE, "XVBA::Close - closing decoder");

  if (!m_context)
    return;

  { CSharedLock lock(*m_context);
    DestroySession();
  }
  m_context->Release();
  if (m_flipBuffer)
    delete [] m_flipBuffer;
}

void CDecoder::ResetState()
{
  picAge.b_age = picAge.ip_age[0] = picAge.ip_age[1] = 256*256*256*64;
  m_presentPicture = 0;
  m_freeOutPic.clear();
  m_usedOutPic.clear();
  for (int j = 0; j < NUM_OUTPUT_PICS; ++j)
    m_freeOutPic.push_back(&m_allOutPic[j]);
}

int CDecoder::Check(AVCodecContext* avctx)
{
  if (m_context && m_context->IsValid(m_ctxId))
    return 0;

  CXVBAContext::EnsureContext(&m_context, &m_ctxId);

  CExclusiveLock lock(*m_context);
  m_xvbaSession = 0;
  DestroySession();
  ResetState();

  return VC_FLUSHED;
}

bool CDecoder::CreateSession(AVCodecContext* avctx)
{
  m_surfaceWidth = (avctx->coded_width+15) & ~15;
  m_surfaceHeight = (avctx->coded_height+15) & ~15;

  XVBA_Create_Decode_Session_Input sessionInput;
  XVBA_Create_Decode_Session_Output sessionOutput;

  sessionInput.size = sizeof(sessionInput);
  sessionInput.width = m_surfaceWidth;
  sessionInput.height = m_surfaceHeight;
  sessionInput.context = m_context->GetContext();
  sessionInput.decode_cap = &m_decoderCap;
  sessionOutput.size = sizeof(sessionOutput);

  if (Success != g_XVBA_vtable.CreateDecode(&sessionInput, &sessionOutput))
  {
    CLog::Log(LOGERROR,"(XVBA) failed to create decoder session");
    return false;
  }
  m_xvbaSession = sessionOutput.session;


  // create decode buffers
  XVBA_Create_DecodeBuff_Input bufferInput;
  XVBA_Create_DecodeBuff_Output bufferOutput;

  bufferInput.size = sizeof(bufferInput);
  bufferInput.session = m_xvbaSession;
  bufferInput.buffer_type = XVBA_PICTURE_DESCRIPTION_BUFFER;
  bufferInput.num_of_buffers = 1;
  bufferOutput.size = sizeof(bufferOutput);
  if (Success != g_XVBA_vtable.CreateDecodeBuffers(&bufferInput, &bufferOutput)
      || bufferOutput.num_of_buffers_in_list != 1)
  {
    CLog::Log(LOGERROR,"(XVBA) failed to create picture buffer");
    return false;
  }
  m_xvbaBufferPool.picture_descriptor_buffer = bufferOutput.buffer_list;

  // data buffer
  bufferInput.buffer_type = XVBA_DATA_BUFFER;
  if (Success != g_XVBA_vtable.CreateDecodeBuffers(&bufferInput, &bufferOutput)
      || bufferOutput.num_of_buffers_in_list != 1)
  {
    CLog::Log(LOGERROR,"(XVBA) failed to create data buffer");
    return false;
  }
  m_xvbaBufferPool.data_buffer = bufferOutput.buffer_list;

  // QO Buffer
  bufferInput.buffer_type = XVBA_QM_BUFFER;
  if (Success != g_XVBA_vtable.CreateDecodeBuffers(&bufferInput, &bufferOutput)
      || bufferOutput.num_of_buffers_in_list != 1)
  {
    CLog::Log(LOGERROR,"(XVBA) failed to create qm buffer");
    return false;
  }
  m_xvbaBufferPool.iq_matrix_buffer = bufferOutput.buffer_list;

  return true;
}

void CDecoder::DestroySession()
{
  XVBA_Destroy_Decode_Buffers_Input bufInput;
  bufInput.size = sizeof(bufInput);
  bufInput.num_of_buffers_in_list = 1;
  if (m_xvbaSession)
  {
    for (unsigned int i=0; i<m_xvbaBufferPool.data_control_buffers.size() ; ++i)
    {
      bufInput.buffer_list = m_xvbaBufferPool.data_control_buffers[i];
      g_XVBA_vtable.DestroyDecodeBuffers(&bufInput);
    }

    if (m_xvbaBufferPool.picture_descriptor_buffer)
    {
      bufInput.buffer_list = m_xvbaBufferPool.picture_descriptor_buffer;
      g_XVBA_vtable.DestroyDecodeBuffers(&bufInput);
    }
    if (m_xvbaBufferPool.iq_matrix_buffer)
    {
      bufInput.buffer_list = m_xvbaBufferPool.iq_matrix_buffer;
      g_XVBA_vtable.DestroyDecodeBuffers(&bufInput);
    }
  }

  m_xvbaBufferPool.data_control_buffers.clear();
  m_xvbaBufferPool.picture_descriptor_buffer = 0;
  m_xvbaBufferPool.iq_matrix_buffer = 0;
  m_xvbaBufferPool.data_buffer = 0;

  while (!m_videoSurfaces.empty())
  {
    xvba_render_state *render = m_videoSurfaces.back();
    if(render->buffers_alllocated > 0)
    	m_dllAvUtil.av_free(render->buffers);

    m_videoSurfaces.pop_back();
    if (m_xvbaSession)
      g_XVBA_vtable.DestroySurface(render->surface);
    free(render);
  }

  if (m_xvbaSession)
    g_XVBA_vtable.DestroyDecode(m_xvbaSession);
  m_xvbaSession = 0;
}

bool CDecoder::EnsureDataControlBuffers(unsigned int num)
{
  if (m_xvbaBufferPool.data_control_buffers.size() >= num)
    return true;

  unsigned int missing = num - m_xvbaBufferPool.data_control_buffers.size();

  XVBA_Create_DecodeBuff_Input bufferInput;
  XVBA_Create_DecodeBuff_Output bufferOutput;
  bufferInput.size = sizeof(bufferInput);
  bufferInput.session = m_xvbaSession;
  bufferInput.buffer_type = XVBA_DATA_CTRL_BUFFER;
  bufferInput.num_of_buffers = 1;
  bufferOutput.size = sizeof(bufferOutput);

  for (unsigned int i=0; i<missing; ++i)
  {
    if (Success != g_XVBA_vtable.CreateDecodeBuffers(&bufferInput, &bufferOutput)
        || bufferOutput.num_of_buffers_in_list != 1)
    {
      CLog::Log(LOGERROR,"(XVBA) failed to create data control buffer");
      return false;
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

  CSharedLock lock(*xvba->m_context);

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
  bool found(false);
  for(unsigned int i = 0; i < xvba->m_videoSurfaces.size(); ++i)
  {
    if(xvba->m_videoSurfaces[i] == render)
    {
      found = true;
      break;
    }
  }
  if (!found)
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

  CSharedLock lock(*xvba->m_context);

  if (!xvba->m_context->IsValid(xvba->m_ctxId))
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
  bool found(false);
  for(unsigned int i = 0; i < xvba->m_videoSurfaces.size(); ++i)
  {
    if(xvba->m_videoSurfaces[i] == render)
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    CLog::Log(LOGWARNING, "XVBA::FFDrawSlice - ignoring invalid buffer");
    return;
  }

  // decoding
  XVBA_Decode_Picture_Start_Input startInput;
  startInput.size = sizeof(startInput);
  startInput.session = xvba->m_xvbaSession;
  startInput.target_surface = render->surface;
  if (Success != g_XVBA_vtable.StartDecodePicture(&startInput))
  {
    CLog::Log(LOGERROR,"(XVBA) failed to start decoding");
    return;
  }
  XVBA_Decode_Picture_Input picInput;
  picInput.size = sizeof(picInput);
  picInput.session = xvba->m_xvbaSession;
  XVBABufferDescriptor *list[2];
  picInput.buffer_list = list;
  list[0] = xvba->m_xvbaBufferPool.picture_descriptor_buffer;
  picInput.num_of_buffers_in_list = 1;
  if (avctx->codec_id == CODEC_ID_H264)
  {
    list[1] = xvba->m_xvbaBufferPool.iq_matrix_buffer;
    picInput.num_of_buffers_in_list = 2;
  }

  XVBAPictureDescriptor *desc = (XVBAPictureDescriptor*)list[0]->bufferXVBA;

  if (Success != g_XVBA_vtable.DecodePicture(&picInput))
  {
    CLog::Log(LOGERROR,"(XVBA) failed to decode picture 1");
    return;
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
      memcpy((uint8_t*)xvba->m_xvbaBufferPool.data_buffer->bufferXVBA+location,
          startCode, 3);
      memcpy((uint8_t*)xvba->m_xvbaBufferPool.data_buffer->bufferXVBA+location+3,
          &sdf, 1);
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
    if (Success != g_XVBA_vtable.DecodePicture(&picInput))
    {
      CLog::Log(LOGERROR,"(XVBA) failed to decode picture 2");
      return;
    }
  }
  XVBA_Decode_Picture_End_Input endInput;
  endInput.size = sizeof(endInput);
  endInput.session = xvba->m_xvbaSession;
  if (Success != g_XVBA_vtable.EndDecodePicture(&endInput))
  {
    CLog::Log(LOGERROR,"(XVBA) failed to decode picture 3");
    return;
  }

  // decode sync and error
  XVBA_Surface_Sync_Input syncInput;
  XVBA_Surface_Sync_Output syncOutput;
  syncInput.size = sizeof(syncInput);
  syncInput.session = xvba->m_xvbaSession;
  syncInput.surface = render->surface;
  syncInput.query_status = XVBA_GET_SURFACE_STATUS;
  syncOutput.size = sizeof(syncOutput);
  while (1)
  {
    if (Success != g_XVBA_vtable.SyncSurface(&syncInput, &syncOutput))
    {
      CLog::Log(LOGERROR,"(XVBA) failed sync 1");
      return;
    }
    if (!(syncOutput.status_flags & XVBA_STILL_PENDING))
      break;
    usleep(100);
  }
}

int CDecoder::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CDVDVideoCodecFFmpeg* ctx   = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDecoder*             xvba  = (CDecoder*)ctx->GetHardware();
  struct pictureAge*    pA    = &xvba->picAge;

  CSharedLock lock(*xvba->m_context);

  if (!xvba->m_context->IsValid(xvba->m_ctxId))
    return -1;

  if (xvba->m_xvbaSession == 0)
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

  // create a new surface
  if (render == NULL)
  {
    render = (xvba_render_state*)calloc(sizeof(xvba_render_state), 1);
    if (render == NULL)
    {
      CLog::Log(LOGERROR, "XVBA::FFGetBuffer - calloc failed");
      return -1;
    }
    XVBA_Create_Surface_Input surfaceInput;
    XVBA_Create_Surface_Output surfaceOutput;
    surfaceInput.size = sizeof(surfaceInput);
    surfaceInput.surface_type = xvba->m_decoderCap.surface_type;
    surfaceInput.width = xvba->m_surfaceWidth;
    surfaceInput.height = xvba->m_surfaceHeight;
    surfaceInput.session = xvba->m_xvbaSession;
    surfaceOutput.size = sizeof(surfaceOutput);
    if (Success != g_XVBA_vtable.CreateSurface(&surfaceInput, &surfaceOutput))
    {
      CLog::Log(LOGERROR,"(XVBA) failed to create video surface");
      return -1;
    }
    CSingleLock lock(xvba->m_videoSurfaceSec);
    render->surface = surfaceOutput.surface;
    render->buffers_alllocated = 0;
    render->picture_descriptor = (XVBAPictureDescriptor *)xvba->m_xvbaBufferPool.picture_descriptor_buffer->bufferXVBA;
    render->iq_matrix = (XVBAQuantMatrixAvc *)xvba->m_xvbaBufferPool.iq_matrix_buffer->bufferXVBA;
    xvba->m_videoSurfaces.push_back(render);
  }

  if (render == NULL)
    return -1;

  pic->data[0] = (uint8_t*)render;
  pic->data[1] =
  pic->data[2] =
  pic->data[3] = 0;

  pic->linesize[0] =
  pic->linesize[1] =
  pic->linesize[2] =
  pic->linesize[3] = 0;

  if(pic->reference)
  {
    pic->age = pA->ip_age[0];
    pA->ip_age[0]= pA->ip_age[1]+1;
    pA->ip_age[1]= 1;
    pA->b_age++;
  }
  else
  {
    pic->age = pA->b_age;
    pA->ip_age[0]++;
    pA->ip_age[1]++;
    pA->b_age = 1;
  }
  pic->type= FF_BUFFER_TYPE_USER;

  render->state |= FF_XVBA_STATE_USED_FOR_REFERENCE;
  pic->reordered_opaque= avctx->reordered_opaque;

  return 0;
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  CSharedLock lock(*m_context);

  int result = Check(avctx);
  if (result)
    return result;

  int iReturn(0);
  if(frame)
  { // we have a new frame from decoder

    xvba_render_state * render = (xvba_render_state*)frame->data[0];
    if(!render)
      return VC_ERROR;

    // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
    bool found(false);
    for(unsigned int i = 0; i < m_videoSurfaces.size(); ++i)
    {
      if(m_videoSurfaces[i] == render)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      CLog::Log(LOGWARNING, "XVBA::Decode - ignoring invalid buffer");
      return VC_BUFFER;
    }

    render->state |= FF_XVBA_STATE_USED_FOR_RENDER;

    CSingleLock lock(m_outPicSec);
    if (m_freeOutPic.empty())
    {
      return VC_ERROR;
    }
    OutputPicture *outPic = m_freeOutPic.front();
    m_freeOutPic.pop_front();
    memset(&outPic->dvdPic, 0, sizeof(DVDVideoPicture));
    ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(&outPic->dvdPic);
    outPic->render = render;

    outPic->dvdPic.format = DVDVideoPicture::FMT_XVBA;
    outPic->dvdPic.iWidth = m_surfaceWidth;
    outPic->dvdPic.iHeight = m_surfaceHeight;
    outPic->dvdPic.xvba = this;

    m_usedOutPic.push_back(outPic);
    lock.Leave();

    iReturn |= VC_PICTURE;
  }

  { CSingleLock lock(m_outPicSec);
    if (!m_freeOutPic.empty())
      iReturn |= VC_BUFFER;
  }

  return iReturn;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  CSharedLock lock(*m_context);

  if (!m_context->IsValid(m_ctxId))
    return false;

  { CSingleLock lock(m_outPicSec);

    if (DiscardPresentPicture())
      CLog::Log(LOGWARNING,"XVBA::GetPicture: old presentPicture was still valid - now discarded");
    if (m_usedOutPic.size() > 0)
    {
      m_presentPicture = m_usedOutPic.front();
      m_usedOutPic.pop_front();
      *picture = m_presentPicture->dvdPic;
    }
    else
    {
      CLog::Log(LOGERROR,"XVBA::GetPicture: no picture");
      return false;
    }
  }
  return true;
}

bool CDecoder::DiscardPresentPicture()
{
  CSingleLock lock(m_outPicSec);
  if (m_presentPicture)
  {
    if (m_presentPicture->render)
      m_presentPicture->render->state &= ~FF_XVBA_STATE_USED_FOR_RENDER;
    m_presentPicture->render = NULL;
    m_freeOutPic.push_back(m_presentPicture);
    m_presentPicture = NULL;
    return true;
  }
  return false;
}

void CDecoder::Present(int index)
{
  CSharedLock lock(*m_context);

  if (!m_context->IsValid(m_ctxId))
    return;

  if (!m_presentPicture)
  {
    CLog::Log(LOGWARNING, "XVBA::Present: present picture is NULL");
    return;
  }

  if (m_flipBuffer[index].outPic)
  {
    if (m_flipBuffer[index].outPic->render)
    {
      CSingleLock lock(m_videoSurfaceSec);
      m_flipBuffer[index].outPic->render->state &= ~FF_XVBA_STATE_USED_FOR_RENDER;
      m_flipBuffer[index].outPic->render = NULL;
    }
    CSingleLock lock(m_outPicSec);
    m_freeOutPic.push_back(m_flipBuffer[index].outPic);
    m_flipBuffer[index].outPic = NULL;
  }

  m_flipBuffer[index].outPic = m_presentPicture;
  m_presentPicture = NULL;
}

void CDecoder::CopyYV12(uint8_t *dest)
{
  CSharedLock lock(*m_context);

  if (!m_context->IsValid(m_ctxId))
    return;

  if (!m_presentPicture)
  {
    CLog::Log(LOGWARNING, "XVBA::Present: present picture is NULL");
    return;
  }

  XVBA_GetSurface_Target target;
  target.size = sizeof(target);
  target.surfaceType = XVBA_YV12;
  target.flag = XVBA_FRAME;

  XVBA_Get_Surface_Input input;
  input.size = sizeof(input);
  input.session = m_xvbaSession;
  input.src_surface = m_presentPicture->render->surface;
  input.target_buffer = dest;
  input.target_pitch = m_surfaceWidth;
  input.target_width = m_surfaceWidth;
  input.target_height = m_surfaceHeight;
  input.target_parameter = target;
  if (Success != g_XVBA_vtable.GetSurface(&input))
  {
    CLog::Log(LOGERROR,"(XVBA::CopyYV12) failed to get  surface");
  }

  if (m_presentPicture->render)
  {
    CSingleLock lock(m_videoSurfaceSec);
    m_presentPicture->render->state &= ~FF_XVBA_STATE_USED_FOR_RENDER;
    m_presentPicture->render = NULL;
  }
  {
    CSingleLock lock(m_outPicSec);
    m_freeOutPic.push_back(m_presentPicture);
    m_presentPicture = NULL;
  }
}

void CDecoder::Reset()
{

}

int CDecoder::UploadTexture(int index, XVBA_SURFACE_FLAG field, GLenum textureTarget)
{
  CSharedLock lock(*m_context);

  if (!m_flipBuffer[index].outPic)
    return -1;

  int i = field;
//  unsigned int first, last;
//  first = last = 0;
//  if (field != XVBA_FRAME)
//  {
//    first = 1;
//    last = 2;
//  }
//  for (unsigned int i = first; i <= last; ++i)
  {
    XVBA_SURFACE_FLAG field;
    if (i==0) field = XVBA_FRAME;
    else if (i==1) field = XVBA_TOP_FIELD;
    else field = XVBA_BOTTOM_FIELD;

    if (!glIsTexture(m_flipBuffer[index].glTexture[i]))
    {
      glEnable(textureTarget);
      glGenTextures(1, &m_flipBuffer[index].glTexture[i]);
      glBindTexture(textureTarget, m_flipBuffer[index].glTexture[i]);
      glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_surfaceWidth, m_surfaceHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

      XVBA_Create_GLShared_Surface_Input surfInput;
      XVBA_Create_GLShared_Surface_Output surfOutput;
      surfInput.size = sizeof(surfInput);
      surfInput.session = m_xvbaSession;
      surfInput.gltexture = m_flipBuffer[index].glTexture[i];
      surfInput.glcontext = glXGetCurrentContext();
      surfOutput.size = sizeof(surfOutput);
      surfOutput.surface = 0;
      if (Success != g_XVBA_vtable.CreateGLSharedSurface(&surfInput, &surfOutput))
      {
        CLog::Log(LOGERROR,"(XVBA) failed to create shared surface");
        return -1;
      }
      m_flipBuffer[index].glSurface[i] = surfOutput.surface;
    }

    XVBA_Transfer_Surface_Input transInput;
    transInput.size = sizeof(transInput);
    transInput.session = m_xvbaSession;
    transInput.src_surface = m_flipBuffer[index].outPic->render->surface;
    transInput.target_surface = m_flipBuffer[index].glSurface[i];
    transInput.flag = field;
    if (Success != g_XVBA_vtable.TransferSurface(&transInput))
    {
      CLog::Log(LOGERROR,"(XVBA) failed to transfer surface");
      return -1;
    }
  }

//  { CSingleLock lock(m_videoSurfaceSec);
//    m_flipBuffer[index].outPic->render->state &= ~FF_XVBA_STATE_USED_FOR_RENDER;
//    m_flipBuffer[index].outPic->render = NULL;
//  }
//  {
//    CSingleLock lock(m_outPicSec);
//    m_freeOutPic.push_back(m_flipBuffer[index].outPic);
//    m_flipBuffer[index].outPic = NULL;
//  }

  return 1;
}

GLuint CDecoder::GetTexture(int index, XVBA_SURFACE_FLAG field)
{
  return m_flipBuffer[index].glTexture[field];
}

void CDecoder::FinishGL()
{
  CLog::Log(LOGNOTICE, "XVBA::FinishGL - clearing down gl resources");

  CSharedLock lock(*m_context);

  for (unsigned int i=0; i<m_numRenderBuffers;++i)
  {
    if (m_flipBuffer[i].outPic)
    {
      { CSingleLock lock(m_videoSurfaceSec);
        m_flipBuffer[i].outPic->render->state &= ~FF_XVBA_STATE_USED_FOR_RENDER;
        m_flipBuffer[i].outPic->render = NULL;
      }
      { CSingleLock lock(m_outPicSec);
        m_freeOutPic.push_back(m_flipBuffer[i].outPic);
        m_flipBuffer[i].outPic = 0;
      }
    }

    for (int j=0; j<3; ++j)
    {
      if (glIsTexture(m_flipBuffer[i].glTexture[j]))
      {
        glDeleteTextures(1, &m_flipBuffer[i].glTexture[j]);
        m_flipBuffer[i].glTexture[j] = 0;
      }
      if (m_flipBuffer[i].glSurface[j])
      {
        g_XVBA_vtable.DestroySurface(m_flipBuffer[i].glSurface[j]);
        m_flipBuffer[i].glSurface[j] = 0;
      }
    }
  }
}

#endif
