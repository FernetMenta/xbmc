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

CAESinkgRPC::CAESinkgRPC()
{
}

CAESinkgRPC::~CAESinkgRPC()
{

}

void CAESinkgRPC::Register()
{
  AE::AESinkRegEntry reg;
  reg.sinkName = "gRPC";
  reg.createFunc = CAESinkgRPC::Create;
  reg.enumerateFunc = CAESinkgRPC::EnumerateDevicesEx;
  AE::CAESinkFactory::RegisterSink(reg);
}

void CAESinkgRPC::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  list.clear();

  CAEDeviceInfo info;
  info.m_deviceName = "RPC Sound Server";
  info.m_displayName = "RPC Sound Server";
  list.push_back(info);
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
  m_pConnector.reset(new CRPCConnector());
  std::string reply = m_pConnector->SayHello("huhu");
  return true;
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
