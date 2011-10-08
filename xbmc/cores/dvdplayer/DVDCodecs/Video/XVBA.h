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

class CXVBAContext : public CSharedSection,
                     public IDispResource
{
public:
  virtual void OnLostDevice();
  virtual void OnResetDevice();
  static bool EnsureContext(CXVBAContext **ctx, int *ctxId);
  bool IsValid(int ctxId);
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
  int m_refCount;
  int m_ctxId;
  void *m_dlHandle;
  void *m_xvbaContext;
  EDisplayState m_displayState;
  CEvent m_displayEvent;
};

class CDecoder : public CDVDVideoCodecFFmpeg::IHardwareDecoder
{
public:
  CDecoder();
  virtual ~CDecoder();
  virtual bool Open(AVCodecContext* avctx, const enum PixelFormat, unsigned int surfaces = 0);
  virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture);
  virtual void Reset();
  virtual void Close();
  virtual int  Check(AVCodecContext* avctx);
  virtual const std::string Name() { return "xvba"; }

  int SetTexture(XVBA_SURFACE_FLAG field, int flipBufferIdx);
  GLuint GetTexture();

protected:
  bool CreateSession(AVCodecContext* avctx);
  void DestroySession();
  bool EnsureDataControlBuffers(int num);

  // callbacks for ffmpeg
  static void  FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic);
  static void  FFDrawSlice(struct AVCodecContext *avctx,
                               const AVFrame *src, int offset[4],
                               int y, int type, int height);
  static int   FFGetBuffer(AVCodecContext *avctx, AVFrame *pic);

  DllAvUtil m_dllAvUtil;
  CXVBAContext *m_context;
  int m_ctxId;

  XVBADecodeCap m_decoderCap;
  void *m_xvbaSession;
  std::vector<XVBABufferDescriptor*> m_dataControlBuffers;

  std::vector<xvba_render_state*> m_videoSurfaces;
  xvba_context m_decoderContext;
  pictureAge picAge;

  struct OutputPicture
  {
    DVDVideoPicture dvdPic;
    xvba_render_state *render;
    void *glSurface;
  };
  OutputPicture m_allOutPic[NUM_OUTPUT_PICS];
  std::deque<OutputPicture*> m_freeOutPic;
  std::deque<OutputPicture*> m_usedOutPic;
  OutputPicture *m_flipBuffer[];
  GLuint m_glTexture;
};

}
