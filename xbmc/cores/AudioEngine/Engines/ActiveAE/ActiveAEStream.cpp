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
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/MathUtils.h"

#include "AEFactory.h"
#include "Utils/AEUtil.h"

#include "ActiveAE.h"
#include "ActiveAEStream.h"

using namespace ActiveAE;

/* typecast AE to CActiveAE */
#define AE (*((CActiveAE*)CAEFactory::GetEngine()))


CActiveAEStream::CActiveAEStream(AEAudioFormat *format)
{
  m_format = *format;
  m_bufferedTime = 0;
  m_currentBuffer = NULL;
  m_drain = false;
  m_paused = false;
}

CActiveAEStream::~CActiveAEStream()
{
}

unsigned int CActiveAEStream::GetSpace()
{
}

unsigned int CActiveAEStream::AddData(void *data, unsigned int size)
{
  Message *msg;
  while(true)
  {
    if (m_currentBuffer)
    {
      int start = m_currentBuffer->pkt->nb_samples *
                  m_currentBuffer->pkt->bytes_per_sample *
                  m_currentBuffer->pkt->config.channels /
                  m_currentBuffer->pkt->planes;
      int samples = size / m_currentBuffer->pkt->bytes_per_sample / m_currentBuffer->pkt->config.channels;
      //TODO: handle planar formats
      memcpy(m_currentBuffer->pkt->data[0] + start, (uint8_t*)data, size);
      {
        CSingleLock lock(*m_statsLock);
        m_currentBuffer->pkt->nb_samples += samples;
        m_bufferedTime += (double)samples / m_currentBuffer->pkt->config.sample_rate;
      }
      if (m_currentBuffer->pkt->nb_samples > m_currentBuffer->pkt->max_nb_samples / 10)
      {
        MsgStreamSample msgData;
        msgData.buffer = m_currentBuffer;
        msgData.stream = this;
        m_streamPort->SendOutMessage(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
        m_currentBuffer = NULL;
      }
      return size;
    }
    else if (m_streamPort->ReceiveInMessage(&msg))
    {
      if (msg->signal == CActiveAEDataProtocol::STREAMBUFFER)
      {
        m_currentBuffer = *((CSampleBuffer**)msg->data);
        msg->Release();
        continue;
      }
      else
      {
        CLog::Log(LOGERROR, "CActiveAEStream::AddData - unknown signal");
        msg->Release();
        return 0;
      }
    }
    if (!m_inMsgEvent.WaitMSec(200))
      return 0;
  }
  return 0;
}

double CActiveAEStream::GetDelay()
{
  return AE.GetDelay(this);
}

bool CActiveAEStream::IsBuffering()
{
  // buffers are filled progressively
  return false;
}

double CActiveAEStream::GetCacheTime()
{
  return AE.GetCacheTime(this);
}

double CActiveAEStream::GetCacheTotal()
{
  return AE.GetCacheTotal(this);
}

void CActiveAEStream::Pause()
{
  AE.PauseStream(this, true);
}

void CActiveAEStream::Resume()
{
  AE.PauseStream(this, false);
}

void CActiveAEStream::Drain()
{
  Message *msg;
  XbmcThreads::EndTime timer(2000);

  CActiveAEStream *stream = this;
  if (m_currentBuffer)
  {
    MsgStreamSample msgData;
    msgData.buffer = m_currentBuffer;
    msgData.stream = this;
    m_streamPort->SendOutMessage(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
    m_currentBuffer = NULL;
  }
  m_streamPort->SendOutMessage(CActiveAEDataProtocol::DRAINSTREAM, &stream, sizeof(CActiveAEStream*));

  while (!timer.IsTimePast())
  {
    if (m_streamPort->ReceiveInMessage(&msg))
    {
      if (msg->signal == CActiveAEDataProtocol::STREAMBUFFER)
      {
        MsgStreamSample msgData;
        msgData.stream = this;
        msgData.buffer = *((CSampleBuffer**)msg->data);
        msg->Reply(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
        continue;
      }
      else if (msg->signal == CActiveAEDataProtocol::STREAMDRAINED)
      {
        msg->Release();
        return;
      }
    }
    m_inMsgEvent.WaitMSec(timer.MillisLeft());
  }
  CLog::Log(LOGERROR, "CActiveAEStream::Drain - timeout out");
}

bool CActiveAEStream::IsDraining()
{
  return false;
}

bool CActiveAEStream::IsDrained()
{
  return true;
}

void CActiveAEStream::Flush()
{
  AE.FlushStream(this);
}

float CActiveAEStream::GetAmplification()
{

}

void CActiveAEStream::SetAmplification(float amplify)
{

}

const unsigned int CActiveAEStream::GetFrameSize() const
{
  return m_format.m_frameSize;
}

const unsigned int CActiveAEStream::GetChannelCount() const
{
  return m_format.m_channelLayout.Count();
}

const unsigned int CActiveAEStream::GetSampleRate() const
{
  return m_format.m_sampleRate;
}

const unsigned int CActiveAEStream::GetEncodedSampleRate() const
{
  return m_format.m_encodedRate;
}

const enum AEDataFormat CActiveAEStream::GetDataFormat() const
{
  return m_format.m_dataFormat;
}

double CActiveAEStream::GetResampleRatio()
{
}

bool CActiveAEStream::SetResampleRatio(double ratio)
{
}

void CActiveAEStream::RegisterAudioCallback(IAudioCallback* pCallback)
{
  AE.RegisterAudioCallback(pCallback);
}

void CActiveAEStream::UnRegisterAudioCallback()
{
  AE.UnRegisterAudioCallback();
}

void CActiveAEStream::FadeVolume(float from, float target, unsigned int time)
{
}

bool CActiveAEStream::IsFading()
{
}

void CActiveAEStream::RegisterSlave(IAEStream *slave)
{
}

