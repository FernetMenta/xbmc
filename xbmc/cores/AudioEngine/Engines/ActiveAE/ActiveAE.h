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

#include "system.h"
#include "threads/Thread.h"

#include "ActiveAESink.h"
#include "ActiveAEResample.h"
#include "Interfaces/AEStream.h"
#include "Interfaces/AESound.h"
#include "AEFactory.h"

// ffmpeg
#include "DllAvFormat.h"
#include "DllAvCodec.h"
#include "DllAvUtil.h"

namespace ActiveAE
{

class IAESink;
class IAEEncoder;
class CActiveAESound;
class CActiveAEStream;

struct AudioSettings
{
  std::string device;
  std::string passthoughdevice;
  int mode;
  int channels;
  bool ac3passthrough;
  bool dtspassthrough;
  bool aacpassthrough;
  bool truehdpassthrough;
  bool dtshdpassthrough;
  bool multichannellpcm;
  bool stereoupmix;
};

class CActiveAEControlProtocol : public Protocol
{
public:
  CActiveAEControlProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
  enum OutSignal
  {
    INIT,
    RECONFIGURE,
    TIMEOUT,
  };
  enum InSignal
  {
    ACC,
    ERROR,
    STATS,
  };
};

class CActiveAEDataProtocol : public Protocol
{
public:
  CActiveAEDataProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
  enum OutSignal
  {
    NEWSOUND = 0,
    PLAYSOUND,
    NEWSTREAM,
    FREESTREAM,
    STREAMSAMPLE,
    DRAINSTREAM,
  };
  enum InSignal
  {
    ACC,
    ERROR,
    STREAMBUFFER,
    STREAMDRAINED,
  };
};

struct MsgStreamSample
{
  CSampleBuffer *buffer;
  CActiveAEStream *stream;
};

class CEngineStats
{
public:
  void Reset(unsigned int sampleRate);
  void UpdateSinkDelay(double delay, int samples);
  void AddSamples(int samples, std::list<CActiveAEStream*> &streams);
  float GetDelay();
  float GetDelay(CActiveAEStream *stream);
  float GetCacheTime(CActiveAEStream *stream);
  float GetCacheTotal(CActiveAEStream *stream);
  float GetWaterLevel();
  CCriticalSection *GetLock() { return &m_lock; }
protected:
  float m_sinkDelay;
  int m_bufferedSamples;
  unsigned int m_sinkSampleRate;
  unsigned int m_sinkUpdate;
  CCriticalSection m_lock;
};

class CActiveAE : public IAE, private CThread
{
protected:
  friend class ::CAEFactory;
  friend class CActiveAESound;
  friend class CActiveAEStream;
  friend class CSoundPacket;
  friend class CActiveAEBufferPoolResample;
  CActiveAE();
  virtual ~CActiveAE();
  virtual bool  Initialize();

public:
  virtual void   Shutdown();
  virtual bool   Suspend();
  virtual bool   Resume();
  virtual bool   IsSuspended();
  virtual void   OnSettingsChange(const std::string& setting);

  virtual float GetVolume();
  virtual void  SetVolume(const float volume);
  virtual void  SetMute(const bool enabled);
  virtual bool  IsMuted();
  virtual void  SetSoundMode(const int mode);

  /* returns a new stream for data in the specified format */
  virtual IAEStream *MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options = 0);
  virtual IAEStream *FreeStream(IAEStream *stream);

  /* returns a new sound object */
  virtual IAESound *MakeSound(const std::string& file);
  virtual void      FreeSound(IAESound *sound);

  virtual void GarbageCollect() {};

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual std::string GetDefaultDevice(bool passthrough);
  virtual bool SupportsRaw();

protected:
  void PlaySound(CActiveAESound *sound);
  uint8_t **AllocSoundSample(SampleConfig &config, int &samples, int &bytes_per_sample, int &planes, int &linesize);
  void FreeSoundSample(uint8_t **data);
  float GetDelay(CActiveAEStream *stream) { return m_stats.GetDelay(stream); }
  float GetCacheTime(CActiveAEStream *stream) { return m_stats.GetCacheTime(stream); }
  float GetCacheTotal(CActiveAEStream *stream) { return m_stats.GetCacheTotal(stream); }

protected:
  void Process();
  void StateMachine(int signal, Protocol *port, Message *msg);
  void InitSink();
  void DrainSink();
  void Start();
  void Dispose();
  void LoadSettings();
  bool NeedReconfigureBuffers();
  bool NeedReconfigureSink();
  void ApplySettingsToFormat(AEAudioFormat &format, AudioSettings &settings);
  void Configure();
  CActiveAEStream* CreateStream(AEAudioFormat *format);
  void DiscardStream(CActiveAEStream *stream);
  void ClearDiscardedBuffers();

  bool RunStages();
  bool HasWork();

  void ResampleSounds();
  bool ResampleSound(CActiveAESound *sound);
  void MixSounds(CSoundPacket &dstSample);
  void Deamplify(CSoundPacket &dstSample);

  CEvent m_inMsgEvent;
  CEvent m_outMsgEvent;
  CActiveAEControlProtocol m_controlPort;
  CActiveAEDataProtocol m_dataPort;
  int m_state;
  bool m_bStateMachineSelfTrigger;
  int m_extTimeout;
  bool m_extError;
  bool m_extDrain;
  XbmcThreads::EndTime m_extDrainTimer;

  enum
  {
    MODE_RAW,
    MODE_TRANSCODE,
    MODE_PCM
  }m_mode;

  CActiveAESink m_sink;
  AEAudioFormat m_sinkFormat;
  AEAudioFormat m_sinkRequestFormat;
  AudioSettings m_settings;
  CEngineStats m_stats;

  // buffers
  CActiveAEBufferPoolResample *m_sinkBuffers;
  CActiveAEBufferPool *m_silenceBuffers;  // needed to drive gui sounds if we have no streams

  // streams
  std::list<CActiveAEStream*> m_streams;
  std::list<CActiveAEBufferPool*> m_discardBufferPools;

  // gui sounds
  struct SoundState
  {
    CActiveAESound *sound;
    int samples_played;
  };
  std::list<SoundState> m_sounds_playing;
  std::vector<CActiveAESound*> m_sounds;

  float m_volume;

  // ffmpeg
  DllAvFormat m_dllAvFormat;
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;
};
};
