#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "AEAudioFormat.h"
#include "Interfaces/AEStream.h"

namespace ActiveAE
{

class CActiveAEStream : public IAEStream
{
protected:
  friend class CActiveAE;
  friend class CEngineStats;
  CActiveAEStream(AEAudioFormat *format);
  virtual ~CActiveAEStream();

public:
  virtual unsigned int GetSpace();
  virtual unsigned int AddData(void *data, unsigned int size);
  virtual double GetDelay();
  virtual bool IsBuffering();
  virtual double GetCacheTime();
  virtual double GetCacheTotal();

  virtual void Pause();
  virtual void Resume();
  virtual void Drain();
  virtual bool IsDraining();
  virtual bool IsDrained();
  virtual void Flush();

  virtual float GetVolume() { return m_volume; }
  virtual float GetReplayGain() { return m_rgain ; }
  virtual float GetAmplification();
  virtual void SetVolume       (float volume) { m_volume = std::max( 0.0f, std::min(1.0f, volume)); }
  virtual void SetReplayGain   (float factor) { m_rgain  = std::max( 0.0f, factor); }
  virtual void SetAmplification(float amplify);

  virtual const unsigned int GetFrameSize() const;
  virtual const unsigned int GetChannelCount() const;
  
  virtual const unsigned int GetSampleRate() const ;
  virtual const unsigned int GetEncodedSampleRate() const;
  virtual const enum AEDataFormat GetDataFormat() const;
  
  virtual double GetResampleRatio();
  virtual bool SetResampleRatio(double ratio);
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();
  virtual void FadeVolume(float from, float to, unsigned int time);
  virtual bool IsFading();
  virtual void RegisterSlave(IAEStream *stream);

protected:

  AEAudioFormat m_format;
  float m_bufferedTime;
  float m_volume;
  float m_rgain;

  CActiveAEBufferPool *m_imputBuffers;
  CActiveAEBufferPoolResample *m_resampleBuffers;
  std::deque<CSampleBuffer*> m_processingSamples;
  CSampleBuffer *m_currentBuffer;
  CActiveAEDataProtocol *m_streamPort;
  CEvent m_inMsgEvent;
  CCriticalSection *m_statsLock;
};
}

