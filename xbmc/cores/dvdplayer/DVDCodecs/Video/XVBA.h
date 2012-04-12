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
#pragma once

#include "X11/Xlib.h"
#include "amd/amdxvba.h"
#include "DllAvCodec.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"
#include "threads/Event.h"
#include "guilib/DispResource.h"
#include "guilib/Geometry.h"
#include "libavcodec/xvba.h"
#include "utils/ActorProtocol.h"
#include "settings/VideoSettings.h"
#include <GL/glx.h>
#include <vector>
#include <deque>

using namespace Actor;


namespace XVBA
{

//-----------------------------------------------------------------------------
// XVBA data structs
//-----------------------------------------------------------------------------

class CDecoder;
class CXVBAContext;
class COutput;

#define NUM_RENDER_PICS 9

/**
 * Buffer statistics used to control number of frames in queue
 */

class CXvbaBufferStats
{
public:
  uint16_t decodedPics;
  uint16_t processedPics;
  uint16_t renderPics;
  uint64_t latency;         // time decoder has waited for a frame, ideally there is no latency
  int playSpeed;
  bool canSkipDeint;
  int processCmd;

  void IncDecoded() { CSingleLock l(m_sec); decodedPics++;}
  void DecDecoded() { CSingleLock l(m_sec); decodedPics--;}
  void IncProcessed() { CSingleLock l(m_sec); processedPics++;}
  void DecProcessed() { CSingleLock l(m_sec); processedPics--;}
  void IncRender() { CSingleLock l(m_sec); renderPics++;}
  void DecRender() { CSingleLock l(m_sec); renderPics--;}
  void Reset() { CSingleLock l(m_sec); decodedPics=0; processedPics=0;renderPics=0;latency=0;}
  void Get(uint16_t &decoded, uint16_t &processed, uint16_t &render) {CSingleLock l(m_sec); decoded = decodedPics, processed=processedPics, render=renderPics;}
  void SetParams(uint64_t time, int speed) { CSingleLock l(m_sec); latency = time; playSpeed = speed; }
  void GetParams(uint64_t &lat, int &speed) { CSingleLock l(m_sec); lat = latency; speed = playSpeed; }
  void SetCmd(int cmd) { CSingleLock l(m_sec); processCmd = cmd; }
  void GetCmd(int &cmd) { CSingleLock l(m_sec); cmd = processCmd; processCmd = 0; }
  void SetCanSkipDeint(bool canSkip) { CSingleLock l(m_sec); canSkipDeint = canSkip; }
  bool CanSkipDeint() { CSingleLock l(m_sec); if (canSkipDeint) return true; else return false;}
private:
  CCriticalSection m_sec;
};

/**
 *  CXvbaConfig holds all configuration parameters needed by vdpau
 *  The structure is sent to the internal classes CMixer and COutput
 *  for init.
 */

struct CXvbaConfig
{
  int surfaceWidth;
  int surfaceHeight;
  int vidWidth;
  int vidHeight;
  int outWidth;
  int outHeight;
  bool useSharedSurfaces;

  CXVBAContext *context;
  XVBADecodeCap decoderCap;
  void *xvbaSession;
  std::vector<xvba_render_state*> *videoSurfaces;
  CCriticalSection *videoSurfaceSec;
  CCriticalSection *apiSec;

  CXvbaBufferStats *stats;
  int numRenderBuffers;
  uint32_t maxReferences;
};

/**
 * Holds a decoded frame
 * Input to COutput for further processing
 */
struct CXvbaDecodedPicture
{
  DVDVideoPicture DVDPic;
  xvba_render_state *render;
};

/**
 * Ready to render textures
 * Sent from COutput back to CDecoder
 * Objects are referenced by DVDVideoPicture and are sent
 * to renderer
 */
class CXvbaRenderPicture
{
  friend class CDecoder;
  friend class COutput;
public:
  DVDVideoPicture DVDPic;
  int texWidth, texHeight;
  CRect crop;
  GLuint texture;
  uint32_t sourceIdx;
  bool valid;
  CDecoder *xvba;
  CXvbaRenderPicture* Acquire();
  long Release();
private:
  void ReturnUnused();
  int refCount;
  CCriticalSection *renderPicSection;
  COutput *xvbaOutput;
};

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------

/**
 * Buffer pool holds allocated xvba and gl resources
 * Embedded in COutput
 */
struct XvbaBufferPool
{
  struct GLVideoSurface
  {
    unsigned int id;
    bool used;
    bool transferred;
    GLuint texture;
    void *glSurface;
    xvba_render_state *render;
    XVBA_SURFACE_FLAG field;
  };
  std::vector<GLVideoSurface> glSurfaces;
  std::vector<CXvbaRenderPicture> allRenderPics;
  std::deque<CXvbaRenderPicture*> usedRenderPics;
  std::deque<CXvbaRenderPicture*> freeRenderPics;
  CCriticalSection renderPicSec;
};

class COutputControlProtocol : public Protocol
{
public:
  COutputControlProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
  enum OutSignal
  {
    INIT,
    FLUSH,
    PRECLEANUP,
    TIMEOUT,
  };
  enum InSignal
  {
    ACC,
    ERROR,
    STATS,
  };
};

class COutputDataProtocol : public Protocol
{
public:
  COutputDataProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
  enum OutSignal
  {
    NEWFRAME = 0,
    RETURNPIC,
  };
  enum InSignal
  {
    PICTURE,
  };
};

/**
 * COutput is embedded in CDecoder and embeds CMixer
 * The class has its own OpenGl context which is shared with render thread
 * COuput generated ready to render textures and passes them back to
 * CDecoder
 */
class COutput : private CThread
{
public:
  COutput(CEvent *inMsgEvent);
  virtual ~COutput();
  void Start();
  void Dispose();
  COutputControlProtocol m_controlPort;
  COutputDataProtocol m_dataPort;
protected:
  void OnStartup();
  void OnExit();
  void Process();
  void StateMachine(int signal, Protocol *port, Message *msg);
  bool HasWork();
  bool IsDecodingFinished();
  CXvbaRenderPicture* ProcessPicture();
  void ProcessReturnPicture(CXvbaRenderPicture *pic);
  int FindFreeSurface();
  void InitCycle();
  void FiniCycle();
  bool Init();
  bool Uninit();
  void Flush();
  bool CreateGlxContext();
  bool DestroyGlxContext();
  bool EnsureBufferPool();
  void ReleaseBufferPool(bool precleanup = false);
  void PreReleaseBufferPool();
  CEvent m_outMsgEvent;
  CEvent *m_inMsgEvent;
  int m_state;
  bool m_bStateMachineSelfTrigger;

  // extended state variables for state machine
  int m_extTimeout;
  bool m_xvbaError;
  CXvbaConfig m_config;
  XvbaBufferPool m_bufferPool;
  Display *m_Display;
  Window m_Window;
  GLXContext m_glContext;
  GLXWindow m_glWindow;
  Pixmap    m_pixmap;
  GLXPixmap m_glPixmap;
  GLsync m_fence;
  std::queue<CXvbaDecodedPicture> m_decodedPics;
  CXvbaDecodedPicture m_processPicture;
  XVBA_SURFACE_FLAG m_field;
  bool m_deinterlacing;
  int m_deintStep;
  bool m_deintSkip;
};

//-----------------------------------------------------------------------------
// XVBA decoder
//-----------------------------------------------------------------------------

class CXVBAContext
{
public:
  static bool EnsureContext(CXVBAContext **ctx);
  void *GetContext();
  void Release();
private:
  CXVBAContext();
  void Close();
  bool LoadSymbols();
  bool CreateContext();
  void DestroyContext();
  static CXVBAContext *m_context;
  static CCriticalSection m_section;
  static Display *m_display;
  int m_refCount;
  static void *m_dlHandle;
  void *m_xvbaContext;
};

class CDecoder : public CDVDVideoCodecFFmpeg::IHardwareDecoder,
                 public IDispResource
{
  friend class CXvbaRenderPicture;

public:

  struct pictureAge
  {
    int b_age;
    int ip_age[2];
  };

  enum EDisplayState
  { XVBA_OPEN
  , XVBA_RESET
  , XVBA_LOST
  , XVBA_ERROR
  };

  CDecoder();
  virtual ~CDecoder();
  virtual void OnLostDevice();
  virtual void OnResetDevice();

  virtual bool Open(AVCodecContext* avctx, const enum PixelFormat fmt, unsigned int surfaces = 0);
  virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture);
  virtual void Reset();
  virtual void Close();
  virtual int  Check(AVCodecContext* avctx);
  virtual long Release();
  virtual const std::string Name() { return "xvba"; }
  virtual bool CanSkipDeint();
  virtual void SetSpeed(int speed);

  bool Supports(EINTERLACEMETHOD method);
  long ReleasePicReference();

protected:
  bool CreateSession(AVCodecContext* avctx);
  void DestroySession(bool precleanup = false);
  bool EnsureDataControlBuffers(unsigned int num);
  void ResetState();
  void SetError(const char* function, const char* msg, int line);
  bool IsSurfaceValid(xvba_render_state *render);
  void ReturnRenderPicture(CXvbaRenderPicture *renderPic);

  // callbacks for ffmpeg
  static void  FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic);
  static void  FFDrawSlice(struct AVCodecContext *avctx,
                               const AVFrame *src, int offset[4],
                               int y, int type, int height);
  static int   FFGetBuffer(AVCodecContext *avctx, AVFrame *pic);

  DllAvUtil m_dllAvUtil;
  CCriticalSection m_decoderSection;
  CEvent m_displayEvent;
  EDisplayState m_displayState;
  CXvbaConfig  m_xvbaConfig;
  std::vector<xvba_render_state*> m_videoSurfaces;
  CCriticalSection m_apiSec, m_videoSurfaceSec;
  ThreadIdentifier m_decoderThread;

  unsigned int m_decoderId;
  struct XVBABufferPool
  {
    XVBABufferDescriptor *picture_descriptor_buffer;
    XVBABufferDescriptor *iq_matrix_buffer;
    XVBABufferDescriptor *data_buffer;
    std::vector<XVBABufferDescriptor*> data_control_buffers;
  };
  XVBABufferPool m_xvbaBufferPool;

  COutput       m_xvbaOutput;
  CXvbaBufferStats m_bufferStats;
  CEvent        m_inMsgEvent;
  CXvbaRenderPicture *m_presentPicture;

  int m_speed;
  int m_codecControl;
};

}
