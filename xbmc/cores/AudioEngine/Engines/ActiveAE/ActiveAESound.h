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

#include "utils/StdString.h"
#include "Interfaces/AESound.h"
#include "ActiveAEResample.h"

class DllAvUtil;

namespace ActiveAE
{

class CActiveAESound : public IAESound
{
public:
  CActiveAESound (const std::string &filename);
  virtual ~CActiveAESound();

  virtual void Play();
  virtual void Stop();
  virtual bool IsPlaying();

  virtual void SetVolume(float volume) { m_volume = std::max(0.0f, std::min(1.0f, volume)); }
  virtual float GetVolume() { return m_volume; }

  uint8_t** InitSound(bool orig, SampleConfig config, int nb_samples);
  bool StoreSound(bool orig, uint8_t **buffer, int samples, int linesize);
  CSoundPacket *GetSound(bool orig);

  bool IsConverted() { return m_isConverted; }
  void SetConverted(bool state) { m_isConverted = state; }

protected:
  std::string m_filename;
  float m_volume;

  CSoundPacket *m_orig_sound;
  CSoundPacket *m_dst_sound;

  bool m_isConverted;
};
}
