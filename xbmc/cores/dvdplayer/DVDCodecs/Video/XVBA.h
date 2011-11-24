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
#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"
#include "threads/Event.h"
#include "guilib/DispResource.h"
#include "libavcodec/xvba.h"
#include <vector>
#include <deque>

#define NUM_OUTPUT_PICS  7

namespace XVBA
{

struct pictureAge
{
  int b_age;
  int ip_age[2];
};

enum EDisplayState
{ XVBA_OPEN
, XVBA_RESET
, XVBA_LOST
};

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
  void *m_dlHandle;
  void *m_xvbaContext;
};

class CDecoder : public CDVDVideoCodecFFmpeg::IHardwareDecoder,
                 public IDispResource
{
public:
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
  virtual const std::string Name() { return "xvba"; }

  void Present(int index);
  void CopyYV12(uint8_t *dest);
  int UploadTexture(int index, XVBA_SURFACE_FLAG field, GLenum textureTarget);
  GLuint GetTexture(int index, XVBA_SURFACE_FLAG field);
  void FinishGL();

protected:
  bool CreateSession(AVCodecContext* avctx);
  void DestroySession();
  bool EnsureDataControlBuffers(unsigned int num);
  bool DiscardPresentPicture();
  void ResetState();
  void SetError(const char* function, const char* msg, int line);

  // callbacks for ffmpeg
  static void  FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic);
  static void  FFDrawSlice(struct AVCodecContext *avctx,
                               const AVFrame *src, int offset[4],
                               int y, int type, int height);
  static int   FFGetBuffer(AVCodecContext *avctx, AVFrame *pic);

  DllAvUtil m_dllAvUtil;
  CXVBAContext *m_context;
  CSharedSection m_displaySection, m_decoderSection;
  CEvent m_displayEvent;
  EDisplayState m_displayState;

  unsigned int m_surfaceWidth, m_surfaceHeight;
  unsigned int m_numRenderBuffers;

  XVBADecodeCap m_decoderCap;
  void *m_xvbaSession;
  struct XVBABufferPool
  {
    XVBABufferDescriptor *picture_descriptor_buffer;
    XVBABufferDescriptor *iq_matrix_buffer;
    XVBABufferDescriptor *data_buffer;
    std::vector<XVBABufferDescriptor*> data_control_buffers;
  };
  XVBABufferPool m_xvbaBufferPool;

  std::vector<xvba_render_state*> m_videoSurfaces;
  pictureAge picAge;

  struct OutputPicture
  {
    DVDVideoPicture dvdPic;
    xvba_render_state *render;
    void *glSurface;
  };
  struct RenderPicture
  {
    OutputPicture *outPic;
    void *glSurface[3];
    GLuint glTexture[3];
  };
  CCriticalSection m_outPicSec, m_videoSurfaceSec;
  OutputPicture m_allOutPic[NUM_OUTPUT_PICS];
  std::deque<OutputPicture*> m_freeOutPic;
  std::deque<OutputPicture*> m_usedOutPic;
  OutputPicture *m_presentPicture;
  RenderPicture *m_flipBuffer;
};

}
