/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkgRPC.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "platform/posix/XTimeUtils.h"

#include "rpc/gRPCConnector.h"

std::unique_ptr<CRPCConnector> CAESinkgRPC::m_pConnector;

CAESinkgRPC::CAESinkgRPC()
{
}

CAESinkgRPC::~CAESinkgRPC()
{

}

void CAESinkgRPC::Register()
{
  m_pConnector.reset(new CRPCConnector());
  m_pConnector->Start();

  AE::AESinkRegEntry reg;
  reg.sinkName = "gRPC";
  reg.createFunc = CAESinkgRPC::Create;
  reg.enumerateFunc = CAESinkgRPC::EnumerateDevicesEx;
  reg.cleanupFunc = CAESinkgRPC::Cleanup;
  AE::CAESinkFactory::RegisterSink(reg);
}

void CAESinkgRPC::Cleanup()
{
  m_pConnector.reset();
}

void CAESinkgRPC::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  list.clear();

  CAEDeviceInfo info;
  if (m_pConnector->GetDeviceInfo(info))
  {
    list.push_back(info);
  }
  else
  {
    info.m_deviceName = "no Soundserver connected";
    info.m_displayName = "no Soundserver connected";
    list.push_back(info);
  }
}

IAESink* CAESinkgRPC::Create(std::string &device, AEAudioFormat &desiredFormat)
{
  IAESink *sink = new CAESinkgRPC();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  delete sink;
  return nullptr;
}

bool CAESinkgRPC::Initialize(AEAudioFormat& format, std::string& device)
{
  return false;
}

void CAESinkgRPC::Deinitialize()
{

}

void CAESinkgRPC::GetDelay(AEDelayStatus& status)
{

}

double CAESinkgRPC::GetCacheTotal()
{
  return 0.5;
}

unsigned int CAESinkgRPC::AddPackets(uint8_t** data, unsigned int frames, unsigned int offset)
{
  Sleep(10);
  return frames;
}

void CAESinkgRPC::Drain()
{

}

