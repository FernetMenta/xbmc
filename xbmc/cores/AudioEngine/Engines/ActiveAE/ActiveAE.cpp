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

#include "ActiveAE.h"

using namespace ActiveAE;
#include "ActiveAESound.h"
#include "ActiveAEStream.h"
#include "Utils/AEUtil.h"

#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"

#define MAX_CACHE_LEVEL 0.5   // total cache time of stream in seconds
#define MAX_WATER_LEVEL 0.25  // buffered time after stream stages in seconds

void CEngineStats::Reset(unsigned int sampleRate)
{
  CSingleLock lock(m_lock);
  m_sinkUpdate = XbmcThreads::SystemClockMillis();
  m_sinkDelay = 0;
  m_sinkSampleRate = sampleRate;
  m_bufferedSamples = 0;
}

void CEngineStats::UpdateSinkDelay(double delay, int samples)
{
  CSingleLock lock(m_lock);
  m_sinkUpdate = XbmcThreads::SystemClockMillis();
  m_sinkDelay = delay;
  if (samples > m_bufferedSamples)
  {
    CLog::Log(LOGERROR, "CEngineStats::UpdateSinkDelay - inconsistency in buffer time");
  }
  else
    m_bufferedSamples -= samples;
}

void CEngineStats::AddSamples(int samples, std::list<CActiveAEStream*> &streams)
{
  CSingleLock lock(m_lock);
  m_bufferedSamples += samples;

  //update buffered time of streams
  std::list<CActiveAEStream*>::iterator it;
  for(it=streams.begin(); it!=streams.end(); ++it)
  {
    float delay = 0;
    std::deque<CSampleBuffer*>::iterator itBuf;
    for(itBuf=(*it)->m_processingSamples.begin(); itBuf!=(*it)->m_processingSamples.end(); ++itBuf)
    {
      delay += (float)(*itBuf)->pkt->nb_samples / (*itBuf)->pkt->config.sample_rate;
    }
    delay += (*it)->m_resampleBuffers->GetDelay();
    (*it)->m_bufferedTime = delay;
  }
}

float CEngineStats::GetDelay()
{
  CSingleLock lock(m_lock);
  unsigned int now = XbmcThreads::SystemClockMillis();
  float delay = m_sinkDelay - (double)(now-m_sinkUpdate) / 1000;
  delay += (float)m_bufferedSamples / m_sinkSampleRate;

  return delay;
}

float CEngineStats::GetDelay(CActiveAEStream *stream)
{
  CSingleLock lock(m_lock);
  unsigned int now = XbmcThreads::SystemClockMillis();
  float delay = m_sinkDelay - (double)(now-m_sinkUpdate) / 1000;
  delay += (float)m_bufferedSamples / m_sinkSampleRate;

  delay += stream->m_bufferedTime;
  return delay;
}

float CEngineStats::GetCacheTime(CActiveAEStream *stream)
{
  CSingleLock lock(m_lock);
  float delay = (float)m_bufferedSamples / m_sinkSampleRate;

  delay += stream->m_bufferedTime;
  return delay;
}

float CEngineStats::GetCacheTotal(CActiveAEStream *stream)
{
  return MAX_CACHE_LEVEL;
}

float CEngineStats::GetWaterLevel()
{
  return (float)m_bufferedSamples / m_sinkSampleRate;
}

CActiveAE::CActiveAE() :
  CThread("ActiveAE"),
  m_controlPort("OutputControlPort", &m_inMsgEvent, &m_outMsgEvent),
  m_dataPort("OutputDataPort", &m_inMsgEvent, &m_outMsgEvent),
  m_sink(&m_outMsgEvent)
{
  m_sink.EnumerateSinkList();
  m_sinkBuffers = NULL;
  m_silenceBuffers = NULL;
  m_volume = 1.0;
  m_mode = MODE_PCM;
}

CActiveAE::~CActiveAE()
{
  Dispose();
}

void CActiveAE::Dispose()
{
  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();
  m_controlPort.Purge();
  m_dataPort.Purge();

  m_dllAvFormat.Unload();
  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
}

//-----------------------------------------------------------------------------
// Behavior
//-----------------------------------------------------------------------------

enum AE_STATES
{
  AE_TOP = 0,                      // 0
  AE_TOP_ERROR,                    // 1
  AE_TOP_UNCONFIGURED,             // 2
  AE_TOP_RECONFIGURING,            // 3
  AE_TOP_CONFIGURED,               // 4
  AE_TOP_CONFIGURED_SUSPEND,       // 5
  AE_TOP_CONFIGURED_IDLE,          // 6
  AE_TOP_CONFIGURED_PLAY,          // 7
};

int AE_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    0, //TOP_RECONFIGURING
    4, //TOP_CONFIGURED_SUSPEND
    4, //TOP_CONFIGURED_IDLE
    4, //TOP_CONFIGURED_PLAY
};

void CActiveAE::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = AE_parentStates[state])
  {
    switch (state)
    {
    case AE_TOP: // TOP
      if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CActiveAEDataProtocol::NEWSOUND:
          CActiveAESound *sound;
          sound = *(CActiveAESound**)msg->data;
          if (sound)
          {
            m_sounds.push_back(sound);
            ResampleSounds();
          }
          return;
        case CActiveAEDataProtocol::FREESTREAM:
          CActiveAEStream *stream;
          stream = *(CActiveAEStream**)msg->data;
          DiscardStream(stream);
          return;
        default:
          break;
        }
      }
      else if (&m_sink.m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::RETURNSAMPLE:
          CSampleBuffer **buffer;
          buffer = (CSampleBuffer**)msg->data;
          if (buffer)
          {
            (*buffer)->Return();
          }
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "CActive::%s - signal: %d form port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case AE_TOP_ERROR:
      break;

    case AE_TOP_UNCONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::INIT:
          LoadSettings();
          Configure();

          msg->Reply(CActiveAEControlProtocol::ACC);
          if (!m_extError)
          {
            m_state = AE_TOP_CONFIGURED_IDLE;
            m_extTimeout = 0;
          }
          else
          {
            m_state = AE_TOP_ERROR;
          }
          return;

        default:
          break;
        }
      }
      break;

    case AE_TOP_RECONFIGURING:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::TIMEOUT:
          // drain
          if (RunStages())
          {
            m_extTimeout = 0;
            return;
          }
          if (HasWork())
          {
            m_extTimeout = 100;
            return;
          }
          if (NeedReconfigureSink())
            DrainSink();
          Configure();
          if (!m_extError)
          {
            m_state = AE_TOP_CONFIGURED_IDLE;
            m_extTimeout = 0;
          }
          else
          {
            m_state = AE_TOP_ERROR;
            m_extTimeout = 1000;
          }
          m_dataPort.DeferOut(false);
          return;
        default:
          break;
        }
      }
      break;

    case AE_TOP_CONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::RECONFIGURE:
          LoadSettings();
          if (!NeedReconfigureBuffers() && !NeedReconfigureSink())
            return;
          m_state = AE_TOP_RECONFIGURING;
          m_extTimeout = 0;
          // don't accept any data until we are reconfigured
          m_dataPort.DeferOut(true);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CActiveAEDataProtocol::PLAYSOUND:
          CActiveAESound *sound;
          sound = *(CActiveAESound**)msg->data;
          if (sound)
          {
            SoundState st = {sound, 0};
            m_sounds_playing.push_back(st);
            m_extTimeout = 0;
            m_state = AE_TOP_CONFIGURED_PLAY;
          }
          return;
        case CActiveAEDataProtocol::NEWSTREAM:
          AEAudioFormat *format;
          CActiveAEStream *stream;
          format = (AEAudioFormat*)msg->data;
          stream = CreateStream(format);
          if(stream)
          {
            msg->Reply(CActiveAEDataProtocol::ACC, &stream, sizeof(CActiveAEStream*));
            Configure();
            m_extTimeout = 0;
            m_state = AE_TOP_CONFIGURED_PLAY;
          }
          else
            msg->Reply(CActiveAEDataProtocol::ERROR);
          return;
        case CActiveAEDataProtocol::STREAMSAMPLE:
          MsgStreamSample *msgData;
          CSampleBuffer *samples;
          msgData = (MsgStreamSample*)msg->data;
          samples = msgData->stream->m_processingSamples.front();
          msgData->stream->m_processingSamples.pop_front();
          if (samples != msgData->buffer)
            CLog::Log(LOGERROR, "CActiveAE - inconsistency in stream sample message");
          msgData->stream->m_resampleBuffers->m_inputSamples.push_back(msgData->buffer);
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          return;
        case CActiveAEDataProtocol::FREESTREAM:
          stream = *(CActiveAEStream**)msg->data;
          DiscardStream(stream);
          if (m_streams.empty())
          {
            m_extDrainTimer.Set(m_stats.GetDelay() * 1000);
            m_extDrain = true;
          }
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          return;
        default:
          break;
        }
      }
      else if (&m_sink.m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::RETURNSAMPLE:
          CSampleBuffer **buffer;
          buffer = (CSampleBuffer**)msg->data;
          if (buffer)
          {
            (*buffer)->Return();
          }
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_PLAY;
          return;
        default:
          break;
        }
      }
      break;

    case AE_TOP_CONFIGURED_IDLE:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::TIMEOUT:
          ResampleSounds();
          ClearDiscardedBuffers();
          if (m_extDrain)
          {
            if (m_extDrainTimer.IsTimePast())
              Configure();
            m_extTimeout = m_extDrainTimer.MillisLeft();
          }
          else
            m_extTimeout = 5000;
          return;
        default:
          break;
        }
      }
      break;

    case AE_TOP_CONFIGURED_PLAY:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CActiveAEControlProtocol::TIMEOUT:
          if (RunStages())
          {
            m_extTimeout = 0;
            return;
          }
          if (HasWork())
          {
            ResampleSounds();
            ClearDiscardedBuffers();
            m_extTimeout = 100;
            return;
          }
          m_extTimeout = 0;
          m_state = AE_TOP_CONFIGURED_IDLE;
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "CActiveAE::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void CActiveAE::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;

  m_state = AE_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;
  m_extDrain = false;

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
    // check sink data port
    else if (m_sink.m_dataPort.ReceiveInMessage(&msg))
    {
      gotMsg = true;
      port = &m_sink.m_dataPort;
    }
    // stream data ports
    else
    {
      std::list<CActiveAEStream*>::iterator it;
      for(it=m_streams.begin(); it!=m_streams.end(); ++it)
      {
        if((*it)->m_streamPort->ReceiveOutMessage(&msg))
        {
          gotMsg = true;
          port = &m_dataPort;
        }
      }
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
      msg->signal = CSinkControlProtocol::TIMEOUT;
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
}

void CActiveAE::Configure()
{
  bool initSink = false;
  AEAudioFormat sinkInputFormat, inputFormat;
  m_mode = MODE_PCM;

  if (m_streams.empty())
  {
    inputFormat.m_dataFormat    = AE_FMT_FLOAT;
    inputFormat.m_sampleRate    = 44100;
    inputFormat.m_encodedRate   = 0;
    inputFormat.m_channelLayout = AE_CH_LAYOUT_2_0;
    inputFormat.m_frames        = 0;
    inputFormat.m_frameSamples  = 0;
    inputFormat.m_frameSize     = 0;

    if (g_advancedSettings.m_audioResample)
    {
      inputFormat.m_sampleRate = g_advancedSettings.m_audioResample;
      CLog::Log(LOGINFO, "CActiveAE::Configure - Forcing samplerate to %d", inputFormat.m_sampleRate);
    }
  }
  else
  {
    inputFormat = m_streams.front()->m_imputBuffers->m_format;
  }

  m_sinkRequestFormat = inputFormat;
  ApplySettingsToFormat(m_sinkRequestFormat, m_settings);
  std::string device = AE_IS_RAW(m_sinkRequestFormat.m_dataFormat) ? m_settings.passthoughdevice : m_settings.device;
  std::string driver;
  CAESinkFactory::ParseDevice(device, driver);
  if (!m_sink.IsCompatible(m_sinkRequestFormat, device))
  {
    InitSink();
    initSink = true;
    m_stats.Reset(m_sinkFormat.m_sampleRate);
  }

  if (m_silenceBuffers)
  {
    delete m_silenceBuffers;
    m_silenceBuffers = NULL;
  }

  // buffers for driving gui sounds if no streams are active
  if (m_streams.empty())
  {
    inputFormat = m_sinkFormat;
    inputFormat.m_dataFormat = AE_FMT_FLOAT;
    inputFormat.m_frameSize = inputFormat.m_channelLayout.Count() *
                              (CAEUtil::DataFormatToBits(inputFormat.m_dataFormat) >> 3);
    m_silenceBuffers = new CActiveAEBufferPoolResample(inputFormat, inputFormat);
    m_silenceBuffers->Create();
    sinkInputFormat = inputFormat;
  }
  // resample buffers for streams
  else
  {
    AEAudioFormat outputFormat;
    if (AE_IS_RAW(inputFormat.m_dataFormat))
    {
      outputFormat = inputFormat;
      sinkInputFormat = m_sinkFormat;
      m_mode = MODE_RAW;
    }
    // transcode
    else if (m_settings.ac3passthrough && !m_settings.multichannellpcm)
    {
      outputFormat = inputFormat;
      outputFormat.m_dataFormat = AE_FMT_FLOAT;
      outputFormat.m_channelLayout = AE_CH_LAYOUT_5_1;
      outputFormat.m_frames = 1536;

      if (g_advancedSettings.m_audioResample)
      {
        outputFormat.m_sampleRate = g_advancedSettings.m_audioResample;
        CLog::Log(LOGINFO, "CActiveAE::Configure - Forcing samplerate to %d", inputFormat.m_sampleRate);
      }

      m_mode = MODE_TRANSCODE;
      sinkInputFormat = m_sinkFormat;
    }
    else
    {
      outputFormat = m_sinkFormat;
      outputFormat.m_dataFormat = AE_FMT_FLOAT;
      outputFormat.m_frameSize = outputFormat.m_channelLayout.Count() *
                                 (CAEUtil::DataFormatToBits(outputFormat.m_dataFormat) >> 3);
      // TODO: adjust to decoder
      outputFormat.m_frames = outputFormat.m_sampleRate / 10;
      sinkInputFormat = outputFormat;
    }

    std::list<CActiveAEStream*>::iterator it;
    for(it=m_streams.begin(); it!=m_streams.end(); ++it)
    {
      if (initSink && (*it)->m_resampleBuffers)
      {
        m_discardBufferPools.push_back((*it)->m_resampleBuffers);
        (*it)->m_resampleBuffers = NULL;
      }
      if (!(*it)->m_resampleBuffers)
      {
        (*it)->m_resampleBuffers = new CActiveAEBufferPoolResample((*it)->m_imputBuffers->m_format, outputFormat);
        (*it)->m_resampleBuffers->Create();
        if (m_mode == MODE_TRANSCODE || m_streams.size() > 1)
          (*it)->m_resampleBuffers->m_fillPackets = true;
      }
    }
  }

  // resample buffers for sink
  if (m_sinkBuffers && !m_sink.IsCompatible(m_sinkBuffers->m_format, device))
  {
    m_discardBufferPools.push_back(m_sinkBuffers);
    m_sinkBuffers = NULL;
  }
  if (!m_sinkBuffers)
  {
    m_sinkBuffers = new CActiveAEBufferPoolResample(sinkInputFormat, m_sinkFormat);
    m_sinkBuffers->Create(true);
  }

  // reset gui sounds
  std::list<SoundState>::iterator it;
  for (it = m_sounds_playing.begin(); it != m_sounds_playing.end(); ++it)
  {
    it->sound->SetConverted(false);
  }

  m_extDrain = false;
}

CActiveAEStream* CActiveAE::CreateStream(AEAudioFormat *format)
{
  // we only can handle a single pass through stream
  if(m_mode == MODE_RAW || (!m_streams.empty() && AE_IS_RAW(format->m_dataFormat)))
  {
    return NULL;
  }

  // create the stream
  CActiveAEStream *stream;
  stream = new CActiveAEStream(format);
  stream->m_streamPort = new CActiveAEDataProtocol("stream",
                             &stream->m_inMsgEvent, &m_outMsgEvent);

  // create buffer pool
  stream->m_imputBuffers = new CActiveAEBufferPool(*format);
  stream->m_imputBuffers->Create();
  stream->m_resampleBuffers = NULL; // create in Configure when we know the sink format
  stream->m_statsLock = m_stats.GetLock();

  m_streams.push_back(stream);

  return stream;
}

void CActiveAE::DiscardStream(CActiveAEStream *stream)
{
  std::list<CActiveAEStream*>::iterator it;
  for (it=m_streams.begin(); it!=m_streams.end(); ++it)
  {
    if (stream == (*it))
    {
      while (!(*it)->m_processingSamples.empty())
      {
        (*it)->m_processingSamples.front()->Return();
        (*it)->m_processingSamples.pop_front();
      }
      m_discardBufferPools.push_back((*it)->m_imputBuffers);
      m_discardBufferPools.push_back((*it)->m_resampleBuffers);
      CLog::Log(LOGDEBUG, "CActiveAE::DiscardStream - audio stream deleted");
      delete (*it)->m_streamPort;
      delete (*it);
      m_streams.erase(it);
      return;
    }
  }
}

void CActiveAE::ClearDiscardedBuffers()
{
  std::list<CActiveAEBufferPool*>::iterator it;
  for (it=m_discardBufferPools.begin(); it!=m_discardBufferPools.end(); ++it)
  {
    CActiveAEBufferPoolResample *rbuf = dynamic_cast<CActiveAEBufferPoolResample*>(*it);
    if (rbuf)
    {
      if (rbuf->m_procSample)
      {
        rbuf->m_procSample->Return();
        rbuf->m_procSample = NULL;
      }
      while (!rbuf->m_inputSamples.empty())
      {
        rbuf->m_inputSamples.front()->Return();
        rbuf->m_inputSamples.pop_front();
      }
      while (!rbuf->m_outputSamples.empty())
      {
        rbuf->m_outputSamples.front()->Return();
        rbuf->m_outputSamples.pop_front();
      }
    }
    // if all buffers have returned, we can delete the buffer pool
    int all = (*it)->m_allSamples.size();
    int free = (*it)->m_freeSamples.size();
    if ((*it)->m_allSamples.size() == (*it)->m_freeSamples.size())
    {
      delete (*it);
      CLog::Log(LOGDEBUG, "CActiveAE::ClearDiscardedBuffers - buffer pool deleted");
      m_discardBufferPools.erase(it);
      return;
    }
  }
}

void CActiveAE::ApplySettingsToFormat(AEAudioFormat &format, AudioSettings &settings)
{
  // raw pass through
  if (AE_IS_RAW(format.m_dataFormat))
  {
    if ((format.m_dataFormat == AE_FMT_AC3 && !settings.ac3passthrough) ||
        (format.m_dataFormat == AE_FMT_TRUEHD && !settings.truehdpassthrough) ||
        (format.m_dataFormat == AE_FMT_DTS && !settings.dtspassthrough) ||
        (format.m_dataFormat == AE_FMT_DTSHD && !settings.dtshdpassthrough))
    {
      CLog::Log(LOGERROR, "CActiveAE::ApplySettingsToFormat - input audio format is wrong");
    }
  }
  // transcode
  else if (settings.ac3passthrough && !settings.multichannellpcm && !m_streams.empty())
  {
    format.m_dataFormat = AE_FMT_AC3;
    format.m_sampleRate = 48000;
  }
  else
  {
    if ((format.m_channelLayout.Count() > 2) || settings.stereoupmix)
    {
      switch (settings.channels)
      {
        default:
        case  0: format.m_channelLayout = AE_CH_LAYOUT_2_0; break;
        case  1: format.m_channelLayout = AE_CH_LAYOUT_2_0; break;
        case  2: format.m_channelLayout = AE_CH_LAYOUT_2_1; break;
        case  3: format.m_channelLayout = AE_CH_LAYOUT_3_0; break;
        case  4: format.m_channelLayout = AE_CH_LAYOUT_3_1; break;
        case  5: format.m_channelLayout = AE_CH_LAYOUT_4_0; break;
        case  6: format.m_channelLayout = AE_CH_LAYOUT_4_1; break;
        case  7: format.m_channelLayout = AE_CH_LAYOUT_5_0; break;
        case  8: format.m_channelLayout = AE_CH_LAYOUT_5_1; break;
        case  9: format.m_channelLayout = AE_CH_LAYOUT_7_0; break;
        case 10: format.m_channelLayout = AE_CH_LAYOUT_7_1; break;
      }
    }
    CAEChannelInfo stdLayout = format.m_channelLayout;
    format.m_channelLayout.ResolveChannels(stdLayout);
  }
}

bool CActiveAE::NeedReconfigureBuffers()
{
  AEAudioFormat newFormat = m_sinkRequestFormat;
  ApplySettingsToFormat(newFormat, m_settings);

  if (newFormat.m_dataFormat != m_sinkRequestFormat.m_dataFormat ||
      newFormat.m_channelLayout != m_sinkRequestFormat.m_channelLayout ||
      newFormat.m_sampleRate != m_sinkRequestFormat.m_sampleRate)
    return true;

  return false;
}

bool CActiveAE::NeedReconfigureSink()
{
  AEAudioFormat newFormat = m_sinkRequestFormat;
  ApplySettingsToFormat(newFormat, m_settings);

  std::string device = AE_IS_RAW(newFormat.m_dataFormat) ? m_settings.passthoughdevice : m_settings.device;
  std::string driver;
  CAESinkFactory::ParseDevice(device, driver);
  if (!m_sink.IsCompatible(newFormat, device))
    return true;

  return false;
}

void CActiveAE::InitSink()
{
  SinkConfig config;
  config.format = m_sinkRequestFormat;
  config.passthrough = false;
  config.stats = &m_stats;

  // start sink
  m_sink.Start();

  // send message to sink
  Message *reply;
  if (m_sink.m_controlPort.SendOutMessageSync(CSinkControlProtocol::CONFIGURE,
                                                 &reply,
                                                 2000,
                                                 &config, sizeof(config)))
  {
    bool success = reply->signal == CSinkControlProtocol::ACC ? true : false;
    if (!success)
    {
      reply->Release();
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
      Dispose();
      m_extError = true;
      return;
    }
    AEAudioFormat *data;
    data = (AEAudioFormat*)reply->data;
    if (data)
    {
      m_sinkFormat = *data;
    }
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to init", __FUNCTION__);
    Dispose();
    m_extError = true;
    return;
  }

  m_inMsgEvent.Reset();
}

void CActiveAE::DrainSink()
{
  // send message to sink
  Message *reply;
  if (m_sink.m_controlPort.SendOutMessageSync(CSinkControlProtocol::DRAIN,
                                                 &reply,
                                                 2000))
  {
    bool success = reply->signal == CSinkControlProtocol::ACC ? true : false;
    if (!success)
    {
      reply->Release();
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error on drain", __FUNCTION__);
      m_extError = true;
      return;
    }
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to drain", __FUNCTION__);
    m_extError = true;
    return;
  }
}

bool CActiveAE::RunStages()
{
  bool busy = false;

  if (!m_sounds_playing.empty() && m_streams.empty())
  {
    if (!m_silenceBuffers->m_freeSamples.empty())
    {
      CSampleBuffer *buf = m_silenceBuffers->m_freeSamples.front();
      m_silenceBuffers->m_freeSamples.pop_front();
      for (int i=0; i<buf->pkt->planes; i++)
      {
        memset(buf->pkt->data[i], 0, buf->pkt->linesize);
      }
      buf->pkt->nb_samples = buf->pkt->max_nb_samples;
      m_silenceBuffers->m_inputSamples.push_back(buf);
      m_silenceBuffers->ResampleBuffers();
      busy = true;
    }
  }

  // serve input streams
  std::list<CActiveAEStream*>::iterator it;
  for (it = m_streams.begin(); it != m_streams.end(); ++it)
  {
    if ((*it)->m_resampleBuffers)
      busy = (*it)->m_resampleBuffers->ResampleBuffers();

    // provide buffers to stream
    float time = m_stats.GetCacheTime((*it));
    CSampleBuffer *buffer;
    while (time < MAX_CACHE_LEVEL && !(*it)->m_imputBuffers->m_freeSamples.empty())
    {
      buffer = (*it)->m_imputBuffers->m_freeSamples.front();
      (*it)->m_imputBuffers->m_freeSamples.pop_front();
      (*it)->m_processingSamples.push_back(buffer);
      (*it)->m_streamPort->SendInMessage(CActiveAEDataProtocol::STREAMBUFFER, &buffer, sizeof(CSampleBuffer*));
      time += (float)buffer->pkt->max_nb_samples / buffer->pkt->config.sample_rate;
    }
  }

  if (m_stats.GetWaterLevel() < MAX_WATER_LEVEL)
  {
    // mix streams and sounds sounds
    if (m_mode != MODE_RAW)
    {
      CSampleBuffer *out = NULL;
      if (m_silenceBuffers && !m_silenceBuffers->m_outputSamples.empty())
      {
        out = m_silenceBuffers->m_outputSamples.front();
        m_silenceBuffers->m_outputSamples.pop_front();
      }

      // mix streams
      std::list<CActiveAEStream*>::iterator it;
      for (it = m_streams.begin(); it != m_streams.end(); ++it)
      {
        if ((*it)->m_resampleBuffers && !(*it)->m_resampleBuffers->m_outputSamples.empty())
        {
          if (!out)
          {
            out = (*it)->m_resampleBuffers->m_outputSamples.front();
            (*it)->m_resampleBuffers->m_outputSamples.pop_front();
          }
          else
          {
            CSampleBuffer *mix = NULL;
            mix = (*it)->m_resampleBuffers->m_outputSamples.front();
            (*it)->m_resampleBuffers->m_outputSamples.pop_front();
            for(int j=0; j<out->pkt->planes; j++)
            {
              float volume = 1.0; //TODO
              float *dst = (float*)out->pkt->data[j];
              float *src = (float*)mix->pkt->data[j];
              int nb_floats = out->pkt->nb_samples / out->pkt->planes;
#ifdef __SSE__
              CAEUtil::SSEMulAddArray(dst, src, volume, nb_floats);
#else
              for (int k = 0; k < nb_floats; ++k)
                *dst++ += *src++ * volume;
#endif
            }
            mix->Return();
          }
          busy = true;
        }
      }

      // mix gui sounds
      if (out)
      {
        MixSounds(*(out->pkt));
        Deamplify(*(out->pkt));
        m_stats.AddSamples(out->pkt->nb_samples, m_streams);
        m_sinkBuffers->m_inputSamples.push_back(out);

        // TODO: encoder
        busy = true;
      }
    }
    // pass through
    else
    {
      std::list<CActiveAEStream*>::iterator it;
      CSampleBuffer *buffer;
      for (it = m_streams.begin(); it != m_streams.end(); ++it)
      {
        if (!(*it)->m_resampleBuffers->m_outputSamples.empty())
        {
          buffer =  (*it)->m_resampleBuffers->m_outputSamples.front();
          (*it)->m_resampleBuffers->m_outputSamples.pop_front();
          m_stats.AddSamples(buffer->pkt->nb_samples, m_streams);
          m_sinkBuffers->m_inputSamples.push_back(buffer);
        }
      }
    }

    // serve sink buffers
    busy = m_sinkBuffers->ResampleBuffers();
    while(!m_sinkBuffers->m_outputSamples.empty())
    {
      CSampleBuffer *out = NULL;
      out = m_sinkBuffers->m_outputSamples.front();
      m_sinkBuffers->m_outputSamples.pop_front();
      m_sink.m_dataPort.SendOutMessage(CSinkDataProtocol::SAMPLE,
          &out, sizeof(CSampleBuffer*));
      busy = true;
    }
  }

  return busy;
}

bool CActiveAE::HasWork()
{
  if (!m_sounds_playing.empty())
    return true;
  if (m_silenceBuffers && !m_silenceBuffers->m_inputSamples.empty())
    return true;
  if (m_silenceBuffers && !m_silenceBuffers->m_outputSamples.empty())
    return true;
  if (!m_sinkBuffers->m_inputSamples.empty())
    return true;
  if (!m_sinkBuffers->m_outputSamples.empty())
    return true;

  std::list<CActiveAEStream*>::iterator it;
  for (it = m_streams.begin(); it != m_streams.end(); ++it)
  {
    if (!(*it)->m_resampleBuffers->m_inputSamples.empty())
      return true;
    if (!(*it)->m_resampleBuffers->m_outputSamples.empty())
      return true;
    if (!(*it)->m_processingSamples.empty())
      return true;
  }

  return false;
}

void CActiveAE::MixSounds(CSoundPacket &dstSample)
{
  if (m_sounds_playing.empty())
    return;

  float volume;
  float *out;
  float *sample_buffer;
  int max_samples = dstSample.nb_samples;

  std::list<SoundState>::iterator it;
  for (it = m_sounds_playing.begin(); it != m_sounds_playing.end(); )
  {
    if (!it->sound->IsConverted())
      ResampleSound(it->sound);
    int available_samples = it->sound->GetSound(false)->nb_samples - it->samples_played;
    int mix_samples = std::min(max_samples, available_samples);
    int start = it->samples_played *
                m_dllAvUtil.av_get_bytes_per_sample(it->sound->GetSound(false)->config.fmt) *
                it->sound->GetSound(false)->config.channels /
                it->sound->GetSound(false)->planes;

    for(int j=0; j<dstSample.planes; j++)
    {
      volume = it->sound->GetVolume();
      out = (float*)dstSample.data[j];
      sample_buffer = (float*)(it->sound->GetSound(false)->data[j]+start);
      int nb_floats = mix_samples * dstSample.config.channels / dstSample.planes;
#ifdef __SSE__
      CAEUtil::SSEMulAddArray(out, sample_buffer, volume, nb_floats);
#else
      for (int k = 0; k < nb_floats; ++k)
        *out++ += *sample_buffer++ * volume;
#endif
    }

    it->samples_played += mix_samples;

    // no more frames, so remove it from the list
    if (it->samples_played >= it->sound->GetSound(false)->nb_samples)
    {
      it = m_sounds_playing.erase(it);
      continue;
    }
    ++it;
  }
}

void CActiveAE::Deamplify(CSoundPacket &dstSample)
{
  if (m_volume < 1.0)
  {
    float *buffer;
    int nb_floats = dstSample.nb_samples * dstSample.config.channels / dstSample.planes;

    for(int j=0; j<dstSample.planes; j++)
    {
      buffer = (float*)dstSample.data[j];
#ifdef __SSE__
      CAEUtil::SSEMulArray(buffer, m_volume, nb_floats);
#else
      float *fbuffer = buffer;
      for (unsigned int i = 0; i < samples; i++)
        *fbuffer++ *= m_volume;
#endif
    }
  }
}

//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------

void CActiveAE::LoadSettings()
{
  m_settings.device = CSettings::Get().GetString("audiooutput.audiodevice");
  m_settings.passthoughdevice = CSettings::Get().GetString("audiooutput.passthroughdevice");

  m_settings.mode = CSettings::Get().GetInt("audiooutput.mode");
  m_settings.channels = CSettings::Get().GetInt("audiooutput.channels");

  m_settings.stereoupmix = CSettings::Get().GetBool("audiooutput.stereoupmix");
  m_settings.ac3passthrough = CSettings::Get().GetBool("audiooutput.ac3passthrough");
  m_settings.truehdpassthrough = CSettings::Get().GetBool("audiooutput.truehdpassthrough");
  m_settings.dtspassthrough = CSettings::Get().GetBool("audiooutput.dtspassthrough");
  m_settings.dtshdpassthrough = CSettings::Get().GetBool("audiooutput.dtshdpassthrough");
  m_settings.aacpassthrough = CSettings::Get().GetBool("audiooutput.passthroughaac");
  m_settings.multichannellpcm = CSettings::Get().GetBool("audiooutput.multichannellpcm");
}

bool CActiveAE::Initialize()
{
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load())
  {
    CLog::Log(LOGERROR,"CActiveAE::Initialize - failed to load ffmpeg libraries");
    return false;
  }
  m_dllAvFormat.av_register_all();

  Create();
  Message *reply;
  if (m_controlPort.SendOutMessageSync(CActiveAEControlProtocol::INIT,
                                                 &reply,
                                                 5000))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "ActiveAE::%s - returned error", __FUNCTION__);
      Dispose();
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "ActiveAE::%s - failed to init", __FUNCTION__);
    Dispose();
    return false;
  }

  m_inMsgEvent.Reset();
  return true;
}

void CActiveAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  m_sink.EnumerateOutputDevices(devices, passthrough);
}

std::string CActiveAE::GetDefaultDevice(bool passthrough)
{
  return m_sink.GetDefaultDevice(passthrough);
}

void CActiveAE::OnSettingsChange(const std::string& setting)
{
  if (setting == "audiooutput.passthroughdevice" ||
      setting == "audiooutput.audiodevice"       ||
      setting == "audiooutput.mode"              ||
      setting == "audiooutput.ac3passthrough"    ||
      setting == "audiooutput.dtspassthrough"    ||
      setting == "audiooutput.passthroughaac"    ||
      setting == "audiooutput.truehdpassthrough" ||
      setting == "audiooutput.dtshdpassthrough"  ||
      setting == "audiooutput.channels"          ||
      setting == "audiooutput.multichannellpcm"  ||
      setting == "audiooutput.stereoupmix")
  {
    m_controlPort.SendOutMessage(CActiveAEControlProtocol::RECONFIGURE);
  }
}

bool CActiveAE::SupportsRaw()
{
  return true;
}

void CActiveAE::Shutdown()
{

}

bool CActiveAE::Suspend()
{

}

bool CActiveAE::Resume()
{

}

bool CActiveAE::IsSuspended()
{
  return false;
}

float CActiveAE::GetVolume()
{
  return m_volume;
}

void CActiveAE::SetVolume(const float volume)
{
  m_volume = volume;
}

void CActiveAE::SetMute(const bool enabled)
{

}

bool CActiveAE::IsMuted()
{
  return false;
}

void CActiveAE::SetSoundMode(const int mode)
{

}

//-----------------------------------------------------------------------------
// Utils
//-----------------------------------------------------------------------------

uint8_t **CActiveAE::AllocSoundSample(SampleConfig &config, int &samples, int &bytes_per_sample, int &planes, int &linesize)
{
  uint8_t **buffer;
  planes = m_dllAvUtil.av_sample_fmt_is_planar(config.fmt) ? config.channels : 1;
  buffer = new uint8_t*[planes];
  m_dllAvUtil.av_samples_alloc(buffer, &linesize, config.channels,
                                 samples, config.fmt, 0);
  bytes_per_sample = m_dllAvUtil.av_get_bytes_per_sample(config.fmt);
  return buffer;
}

void CActiveAE::FreeSoundSample(uint8_t **data)
{
  m_dllAvUtil.av_freep(data);
  delete [] data;
}

//-----------------------------------------------------------------------------
// GUI Sounds
//-----------------------------------------------------------------------------

/**
 * load sound from an audio file and store original format
 * register the sound in ActiveAE
 * later when the engine is idle it will convert the sound to sink format
 */

#define SOUNDBUFFER_SIZE 20480
uint8_t soundLoadbuffer[SOUNDBUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];

IAESound *CActiveAE::MakeSound(const std::string& file)
{
  AVFormatContext *fmt_ctx = NULL;
  AVCodecContext *dec_ctx = NULL;
  AVCodec *dec = NULL;
  int bit_rate;
  CActiveAESound *sound = NULL;
  SampleConfig config;

  // get length of file in order to estimate number of samples
  FILE *f;
  int fileSize = 0;
  f = fopen(file.c_str(), "rb");
  if (!f)
  {
    return NULL;
  }
  if(fseek(f, 0, SEEK_END))
  {
    fclose(f);
    return NULL;
  }
  fileSize = ftell(f);
  if (fseek(f, 0, SEEK_SET))
  {
    fclose(f);
    return NULL;
  }
  fclose(f);

  // find decoder
  if (m_dllAvFormat.avformat_open_input(&fmt_ctx, file.c_str(), NULL, NULL) == 0)
  {
    fmt_ctx->flags |= AVFMT_FLAG_NOPARSE;
    if (m_dllAvFormat.avformat_find_stream_info(fmt_ctx, NULL) >= 0)
    {
      dec_ctx = fmt_ctx->streams[0]->codec;
      dec = m_dllAvCodec.avcodec_find_decoder(dec_ctx->codec_id);
      config.sample_rate = dec_ctx->sample_rate;
      bit_rate = dec_ctx->bit_rate;
      config.channels = dec_ctx->channels;
      config.channel_layout = dec_ctx->channel_layout;
    }
  }
  if (dec == NULL)
  {
    m_dllAvFormat.avformat_close_input(&fmt_ctx);
    return NULL;
  }

  dec_ctx = m_dllAvCodec.avcodec_alloc_context3(dec);
  dec_ctx->sample_rate = config.sample_rate;
  dec_ctx->channels = config.channels;
  if (!config.channel_layout)
    config.channel_layout = m_dllAvUtil.av_get_default_channel_layout(config.channels);
  dec_ctx->channel_layout = config.channel_layout;

  AVPacket avpkt;
  AVFrame *decoded_frame = NULL;
  decoded_frame = m_dllAvCodec.avcodec_alloc_frame();

  if (m_dllAvCodec.avcodec_open2(dec_ctx, dec, NULL) >= 0)
  {
    sound = new CActiveAESound(file);
    bool init = false;

    // decode until eof
    m_dllAvCodec.av_init_packet(&avpkt);
    int len;
    while (m_dllAvFormat.av_read_frame(fmt_ctx, &avpkt) >= 0)
    {
      int got_frame = 0;
      len = m_dllAvCodec.avcodec_decode_audio4(dec_ctx, decoded_frame, &got_frame, &avpkt);
      if (len < 0)
      {
        m_dllAvCodec.avcodec_close(dec_ctx);
        m_dllAvUtil.av_free(dec_ctx);
        m_dllAvUtil.av_free(&decoded_frame);
        m_dllAvFormat.avformat_close_input(&fmt_ctx);
        return NULL;
      }
      if (got_frame)
      {
        if (!init)
        {
          int samples = fileSize / m_dllAvUtil.av_get_bytes_per_sample(dec_ctx->sample_fmt) / config.channels;
          config.fmt = dec_ctx->sample_fmt;
          sound->InitSound(true, config, samples);
          init = true;
        }
        sound->StoreSound(true, decoded_frame->extended_data,
                          decoded_frame->nb_samples, decoded_frame->linesize[0]);
      }
    }
    m_dllAvCodec.avcodec_close(dec_ctx);
  }

  m_dllAvUtil.av_free(dec_ctx);
  m_dllAvUtil.av_free(decoded_frame);
  m_dllAvFormat.avformat_close_input(&fmt_ctx);

  // register sound
  m_dataPort.SendOutMessage(CActiveAEDataProtocol::NEWSOUND, &sound, sizeof(CActiveAESound*));

  return sound;
}

void CActiveAE::FreeSound(IAESound *sound)
{

}

void CActiveAE::PlaySound(CActiveAESound *sound)
{
  m_dataPort.SendOutMessage(CActiveAEDataProtocol::PLAYSOUND, &sound, sizeof(CActiveAESound*));
}

/**
 * resample sounds to destination format for mixing
 * destination format is either format of stream or
 * default sink format when no stream is playing
 */
void CActiveAE::ResampleSounds()
{
  if (m_mode == MODE_RAW)
    return;

  std::vector<CActiveAESound*>::iterator it;
  for (it = m_sounds.begin(); it != m_sounds.end(); ++it)
  {
    if (!(*it)->IsConverted())
      ResampleSound(*it);
  }
}

bool CActiveAE::ResampleSound(CActiveAESound *sound)
{
  SampleConfig orig_config, dst_config;
  uint8_t **dst_buffer;
  int dst_samples;
  if (!sound->GetSound(true))
    return false;

  orig_config = sound->GetSound(true)->config;

  AEAudioFormat dstFormat;
  if (m_silenceBuffers)
    dstFormat = m_silenceBuffers->m_format;
  else if (!m_streams.empty() && m_streams.front()->m_resampleBuffers)
    dstFormat = m_streams.front()->m_resampleBuffers->m_format;
  else
    return false;

  dst_config.channel_layout = CActiveAEResample::GetAVChannelLayout(dstFormat.m_channelLayout);
  dst_config.channels = dstFormat.m_channelLayout.Count();
  dst_config.sample_rate = dstFormat.m_sampleRate;
  dst_config.fmt = CActiveAEResample::GetAVSampleFormat(dstFormat.m_dataFormat);

  CActiveAEResample *resampler = new CActiveAEResample();
  resampler->Init(dst_config.channel_layout,
                  dst_config.channels,
                  dst_config.sample_rate,
                  dst_config.fmt,
                  orig_config.channel_layout,
                  orig_config.channels,
                  orig_config.sample_rate,
                  orig_config.fmt,
                  NULL);

  dst_samples = resampler->CalcDstSampleCount(sound->GetSound(true)->nb_samples,
                                              dstFormat.m_sampleRate,
                                              orig_config.sample_rate);

  dst_buffer = sound->InitSound(false, dst_config, dst_samples);
  if (!dst_buffer)
  {
    delete resampler;
    return false;
  }
  int samples = resampler->Resample(dst_buffer, dst_samples,
                                    sound->GetSound(true)->data,
                                    sound->GetSound(true)->nb_samples);

  sound->GetSound(false)->nb_samples = samples;

  delete resampler;
  sound->SetConverted(true);
  return true;
}

//-----------------------------------------------------------------------------
// Streams
//-----------------------------------------------------------------------------

IAEStream *CActiveAE::MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options)
{
  //TODO: pass number of samples in audio packet

  AEAudioFormat format;
  format.m_dataFormat = dataFormat;
  format.m_sampleRate = sampleRate;
  format.m_encodedRate = encodedSampleRate;
  format.m_channelLayout = channelLayout;
  format.m_frames = format.m_sampleRate / 10;
  format.m_frameSize = format.m_channelLayout.Count() *
                       (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);

  Message *reply;
  if (m_dataPort.SendOutMessageSync(CActiveAEDataProtocol::NEWSTREAM,
                                    &reply,1000,
                                    &format, sizeof(AEAudioFormat)))
  {
    bool success = reply->signal == CActiveAEControlProtocol::ACC ? true : false;
    if (success)
    {
      CActiveAEStream *stream = *(CActiveAEStream**)reply->data;
      reply->Release();
      return stream;
    }
    reply->Release();
  }

  CLog::Log(LOGERROR, "ActiveAE::%s - could not create stream", __FUNCTION__);
  return NULL;
}

IAEStream *CActiveAE::FreeStream(IAEStream *stream)
{
  m_dataPort.SendOutMessage(CActiveAEDataProtocol::FREESTREAM, &stream, sizeof(IAEStream*));
  return NULL;
}





