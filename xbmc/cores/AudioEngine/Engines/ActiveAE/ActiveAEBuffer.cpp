/*
 *      Copyright (C) 2010-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ActiveAEBuffer.h"
#include "AEFactory.h"
#include "ActiveAE.h"

using namespace ActiveAE;

/* typecast AE to CActiveAE */
#define AE (*((CActiveAE*)CAEFactory::GetEngine()))

CSoundPacket::CSoundPacket(SampleConfig conf, int samples) : config(conf)
{
  data = AE.AllocSoundSample(config, samples, bytes_per_sample, planes, linesize);
  max_nb_samples = samples;
  nb_samples = 0;
}

CSoundPacket::~CSoundPacket()
{
  if (data)
    AE.FreeSoundSample(data);
}

CSampleBuffer::CSampleBuffer() : pkt(NULL), pool(NULL)
{
  refCount = 0;
}

CSampleBuffer::~CSampleBuffer()
{
  if (pkt)
    delete pkt;
}

CSampleBuffer* CSampleBuffer::Acquire()
{
  refCount++;
  return this;
}

void CSampleBuffer::Return()
{
  refCount--;
  if (pool && refCount <= 0)
    pool->ReturnBuffer(this);
}

CActiveAEBufferPool::CActiveAEBufferPool(AEAudioFormat format)
{
  m_format = format;
  if (AE_IS_RAW(m_format.m_dataFormat))
    m_format.m_dataFormat = AE_FMT_S16NE;
}

CActiveAEBufferPool::~CActiveAEBufferPool()
{
  CSampleBuffer *buffer;
  while(!m_allSamples.empty())
  {
    buffer = m_allSamples.front();
    m_allSamples.pop_front();
    delete buffer;
  }
}

CSampleBuffer* CActiveAEBufferPool::GetFreeBuffer()
{
  CSampleBuffer* buf = NULL;

  if (!m_freeSamples.empty())
  {
    buf = m_freeSamples.front();
    m_freeSamples.pop_front();
    buf->refCount = 1;
  }
  return buf;
}

void CActiveAEBufferPool::ReturnBuffer(CSampleBuffer *buffer)
{
  buffer->pkt->nb_samples = 0;
  m_freeSamples.push_back(buffer);
}

bool CActiveAEBufferPool::Create(unsigned int totaltime)
{
  CSampleBuffer *buffer;
  SampleConfig config;
  config.fmt = CActiveAEResample::GetAVSampleFormat(m_format.m_dataFormat);
  config.channels = m_format.m_channelLayout.Count();
  config.sample_rate = m_format.m_sampleRate;
  config.channel_layout = CActiveAEResample::GetAVChannelLayout(m_format.m_channelLayout);

  unsigned int time = 0;
  unsigned int buffertime = (m_format.m_frames*1000) / m_format.m_sampleRate;
  unsigned int n = 0;
  while (time < totaltime || n < 5)
  {
    buffer = new CSampleBuffer();
    buffer->pool = this;
    buffer->pkt = new CSoundPacket(config, m_format.m_frames);

    m_allSamples.push_back(buffer);
    m_freeSamples.push_back(buffer);
    time += buffertime;
    n++;
  }

  return true;
}

//-----------------------------------------------------------------------------

CActiveAEBufferPoolResample::CActiveAEBufferPoolResample(AEAudioFormat inputFormat, AEAudioFormat outputFormat)
  : CActiveAEBufferPool(outputFormat)
{
  m_inputFormat = inputFormat;
  if (AE_IS_RAW(m_inputFormat.m_dataFormat))
    m_inputFormat.m_dataFormat = AE_FMT_S16NE;
  m_resampler = NULL;
  m_fillPackets = false;
  m_drain = false;
  m_procSample = NULL;
  m_resampleRatio = 1.0;
  m_changeRatio = false;
}

CActiveAEBufferPoolResample::~CActiveAEBufferPoolResample()
{
  if (m_resampler)
    delete m_resampler;
}

bool CActiveAEBufferPoolResample::Create(unsigned int totaltime, bool remap)
{
  CActiveAEBufferPool::Create(totaltime);

  if (m_inputFormat.m_channelLayout != m_format.m_channelLayout ||
      m_inputFormat.m_sampleRate != m_format.m_sampleRate ||
      m_inputFormat.m_dataFormat != m_format.m_dataFormat)
  {
    m_resampler = new CActiveAEResample();
    m_resampler->Init(CActiveAEResample::GetAVChannelLayout(m_format.m_channelLayout),
                                m_format.m_channelLayout.Count(),
                                m_format.m_sampleRate,
                                CActiveAEResample::GetAVSampleFormat(m_format.m_dataFormat),
                                CActiveAEResample::GetAVChannelLayout(m_inputFormat.m_channelLayout),
                                m_inputFormat.m_channelLayout.Count(),
                                m_inputFormat.m_sampleRate,
                                CActiveAEResample::GetAVSampleFormat(m_inputFormat.m_dataFormat),
                                remap ? &m_format.m_channelLayout : NULL);
  }

  // store output sampling rate, needed when ratio gets changed
  m_outSampleRate = m_format.m_sampleRate;

  return true;
}

void CActiveAEBufferPoolResample::ChangeRatio()
{
//  CLog::Log(LOGNOTICE,"---------- sample rate changed from: %d, to: %d",
//      m_outSampleRate, (int)(m_format.m_sampleRate * m_resampleRatio));

  m_outSampleRate = m_format.m_sampleRate * m_resampleRatio;

  if (m_resampler)
    delete m_resampler;

  m_resampler = new CActiveAEResample();
  m_resampler->Init(CActiveAEResample::GetAVChannelLayout(m_format.m_channelLayout),
                                m_format.m_channelLayout.Count(),
                                m_outSampleRate,
                                CActiveAEResample::GetAVSampleFormat(m_format.m_dataFormat),
                                CActiveAEResample::GetAVChannelLayout(m_inputFormat.m_channelLayout),
                                m_inputFormat.m_channelLayout.Count(),
                                m_inputFormat.m_sampleRate,
                                CActiveAEResample::GetAVSampleFormat(m_inputFormat.m_dataFormat),
                                NULL);

  m_changeRatio = false;
}

bool CActiveAEBufferPoolResample::ResampleBuffers(unsigned int timestamp)
{
  bool busy = false;
  CSampleBuffer *in;

  if (m_changeRatio)
  {
    if ((unsigned int)(m_format.m_sampleRate * m_resampleRatio) == m_outSampleRate)
      m_changeRatio = false;
  }

  if (!m_resampler)
  {
    if (m_changeRatio)
    {
      ChangeRatio();
      return true;
    }
    while(!m_inputSamples.empty())
    {
      in = m_inputSamples.front();
      m_inputSamples.pop_front();
      m_outputSamples.push_back(in);
      busy = true;
    }
  }
  else if (m_procSample || !m_freeSamples.empty())
  {
    int out_samples = m_resampler->GetBufferedSamples();
    int free_samples;
    if (m_procSample)
      free_samples = m_procSample->pkt->max_nb_samples - m_procSample->pkt->nb_samples;
    else
      free_samples = m_format.m_frames;

    bool skipInput = false;
    if (out_samples > free_samples * 2)
      skipInput = true;

    bool hasInput = !m_inputSamples.empty();

    if (hasInput || skipInput || m_drain || m_changeRatio)
    {
      if (!m_procSample)
      {
        m_procSample = GetFreeBuffer();
      }

      if (hasInput && !skipInput && !m_changeRatio)
      {
        in = m_inputSamples.front();
        m_inputSamples.pop_front();
      }
      else
        in = NULL;

      int start = m_procSample->pkt->nb_samples *
                  m_procSample->pkt->bytes_per_sample *
                  m_procSample->pkt->config.channels /
                  m_procSample->pkt->planes;

      for(int i=0; i<m_procSample->pkt->planes; i++)
      {
        m_planes[i] = m_procSample->pkt->data[i] + start;
      }

      out_samples = m_resampler->Resample(m_planes,
                                          m_procSample->pkt->max_nb_samples - m_procSample->pkt->nb_samples,
                                          in ? in->pkt->data : NULL,
                                          in ? in->pkt->nb_samples : 0);
      m_procSample->pkt->nb_samples += out_samples;
      busy = true;

      if ((m_drain || m_changeRatio) && out_samples == 0)
      {
        if (m_fillPackets && m_procSample->pkt->nb_samples != 0)
        {
          // pad with zero
          start = m_procSample->pkt->nb_samples *
                  m_procSample->pkt->bytes_per_sample *
                  m_procSample->pkt->config.channels /
                  m_procSample->pkt->planes;
          for(int i=0; i<m_procSample->pkt->planes; i++)
          {
            memset(m_procSample->pkt->data[i]+start, 0, m_procSample->pkt->linesize-start);
          }
        }
        m_procSample->timestamp = timestamp;

        // check if draining is finished
        if (m_drain && m_procSample->pkt->nb_samples == 0)
        {
          m_procSample->Return();
          busy = false;
        }
        else
          m_outputSamples.push_back(m_procSample);

        m_procSample = NULL;
        if (m_changeRatio)
          ChangeRatio();
      }
      // some methods like encode require completely filled packets
      else if (!m_fillPackets || (m_procSample->pkt->nb_samples == m_procSample->pkt->max_nb_samples))
      {
        m_procSample->timestamp = timestamp;
        m_outputSamples.push_back(m_procSample);
        m_procSample = NULL;
      }

      if (in)
        in->Return();
    }
  }
  return busy;
}

float CActiveAEBufferPoolResample::GetDelay()
{
  float delay = 0;
  std::deque<CSampleBuffer*>::iterator itBuf;

  if (m_procSample)
    delay += m_procSample->pkt->nb_samples / m_procSample->pkt->config.sample_rate;

  for(itBuf=m_inputSamples.begin(); itBuf!=m_inputSamples.end(); ++itBuf)
  {
    delay += (float)(*itBuf)->pkt->nb_samples / (*itBuf)->pkt->config.sample_rate;
  }

  for(itBuf=m_outputSamples.begin(); itBuf!=m_outputSamples.end(); ++itBuf)
  {
    delay += (float)(*itBuf)->pkt->nb_samples / (*itBuf)->pkt->config.sample_rate;
  }

  if (m_resampler)
  {
    int samples = m_resampler->GetBufferedSamples();
    delay += (float)samples / m_outSampleRate;
  }

  return delay;
}

void CActiveAEBufferPoolResample::Flush()
{
  if (m_procSample)
  {
    m_procSample->Return();
    m_procSample = NULL;
  }
  while (!m_inputSamples.empty())
  {
    m_inputSamples.front()->Return();
    m_inputSamples.pop_front();
  }
  while (!m_outputSamples.empty())
  {
    m_outputSamples.front()->Return();
    m_outputSamples.pop_front();
  }
}
