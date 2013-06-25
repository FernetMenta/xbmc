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

#include "Interfaces/AESound.h"

#include "AEFactory.h"
#include "AEAudioFormat.h"
#include "ActiveAE.h"
#include "ActiveAESound.h"

#include "utils/log.h"

#include "DllAvUtil.h"

using namespace ActiveAE;

/* typecast AE to CActiveAE */
#define AE (*((CActiveAE*)CAEFactory::GetEngine()))

CActiveAESound::CActiveAESound(const std::string &filename) :
  IAESound         (filename),
  m_filename       (filename),
  m_volume         (1.0f    )
{
  m_orig_sound = NULL;
  m_dst_sound = NULL;
}

CActiveAESound::~CActiveAESound()
{
  if (m_orig_sound)
    delete m_orig_sound;
  if (m_dst_sound)
    delete m_dst_sound;
}

void CActiveAESound::Play()
{
  AE.PlaySound(this);
}

void CActiveAESound::Stop()
{
  AE.StopSound(this);
}

bool CActiveAESound::IsPlaying()
{
  // TODO
  return false;
}

uint8_t** CActiveAESound::InitSound(bool orig, SampleConfig config, int nb_samples)
{
  CSoundPacket **info;
  if (orig)
    info = &m_orig_sound;
  else
    info = &m_dst_sound;

  if (*info)
    delete *info;
  *info = new CSoundPacket(config, nb_samples);

  (*info)->nb_samples = 0;
  m_isConverted = false;
  return (*info)->data;
}

bool CActiveAESound::StoreSound(bool orig, uint8_t **buffer, int samples, int linesize)
{
  CSoundPacket **info;
  if (orig)
    info = &m_orig_sound;
  else
    info = &m_dst_sound;

  if ((*info)->nb_samples + samples > (*info)->max_nb_samples)
  {
    CLog::Log(LOGERROR, "CActiveAESound::StoreSound - exceeded max samples");
    return false;
  }

  int bytes_to_copy = samples * (*info)->bytes_per_sample * (*info)->config.channels;
  bytes_to_copy /= (*info)->planes;
  int start = (*info)->nb_samples * (*info)->bytes_per_sample * (*info)->config.channels;
  start /= (*info)->planes;

  for (int i=0; i<(*info)->planes; i++)
  {
    memcpy((*info)->data[i]+start, buffer[i], bytes_to_copy);
  }
  (*info)->nb_samples += samples;

  return true;
}

CSoundPacket *CActiveAESound::GetSound(bool orig)
{
  if (orig)
    return m_orig_sound;
  else
    return m_dst_sound;
}
