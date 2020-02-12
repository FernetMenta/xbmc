/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Sinks/rpc/gRPCConnector.h"
#include <memory>

class CRPCConnector;

class CAESinkgRPC : public IAESink, ISinkgRPC
{
public:
  const char* GetName() override { return "gRPC"; }

  CAESinkgRPC();
  ~CAESinkgRPC() override;

  static void Register();
  static void Cleanup();
  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force);
  static IAESink* Create(std::string &device, AEAudioFormat &desiredFormat);

  bool Initialize(AEAudioFormat& format, std::string& device) override;
  void Deinitialize() override;

  void GetDelay(AEDelayStatus& status) override;
  double GetCacheTotal() override;
  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;
  void Drain() override;

protected:
  static std::unique_ptr<CRPCConnector> m_pConnector;
};
