/*
*      Copyright (C) 2007-2018 Team XBMC
 *      http://kodi.tv
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

#include "LIRC.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "input/InputManager.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include <lirc/lirc_client.h>
#include <fcntl.h>

CLirc::CLirc() : CThread("Lirc")
{
  Create();
}

CLirc::~CLirc()
{
  StopThread();
}

void CLirc::Process()
{
  int fd = -1;
  struct lirc_config *config;

  while (!m_bStop)
  {
    fd = lirc_init("kodi", 0);
    if (fd <= 0)
    {
      Sleep(1000);
      continue;
    }

    // set non-blocking mode
    fcntl(fd, F_SETOWN, getpid());
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
      lirc_deinit();
      Sleep(1000);
      continue;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);


    char *code;
    while (!m_bStop)
    {
      int ret = lirc_nextcode(&code);
      if (ret < 0)
      {
        lirc_deinit();
        Sleep(1000);
        break;
      }
      if (code == nullptr)
      {
        Sleep(100);
        continue;
      }
      ProcessCode(code);
      free(code);
    }
  }

  lirc_deinit();
}

void CLirc::ProcessCode(char *buf)
{
  // Parse the result. Sample line:
  // 000000037ff07bdd 00 OK mceusb
  char scanCode[128];
  char buttonName[128];
  char repeatStr[4];
  char deviceName[128];
  sscanf(buf, "%s %s %s %s", &scanCode[0], &repeatStr[0], &buttonName[0], &deviceName[0]);

  // Some template LIRC configuration have button names in apostrophes or quotes.
  // If we got a quoted button name, strip 'em
  unsigned int buttonNameLen = strlen(buttonName);
  if (buttonNameLen > 2 &&
      ((buttonName[0] == '\'' && buttonName[buttonNameLen-1] == '\'') ||
      ((buttonName[0] == '"' && buttonName[buttonNameLen-1] == '"'))))
  {
    memmove(buttonName, buttonName + 1, buttonNameLen - 2);
    buttonName[buttonNameLen - 2] = '\0';
  }

  int button = CServiceBroker::GetInputManager().TranslateLircRemoteString(deviceName, buttonName);

  char *end = nullptr;
  long repeat = strtol(repeatStr, &end, 16);
  if (!end || *end != 0)
    CLog::Log(LOGERROR, "LIRC: invalid non-numeric character in expression %s", repeatStr);

  if (repeat == 0)
  {
    CLog::Log(LOGDEBUG, "LIRC: - NEW %s (%s)", buf, buttonName);
    m_firstClickTime = XbmcThreads::SystemClockMillis();

    XBMC_Event newEvent;
    newEvent.type = XBMC_BUTTON;
    newEvent.keybutton.button = button;
    newEvent.keybutton.holdtime = 0;
    g_application.OnEvent(newEvent);
    return;
  }
  else if (repeat > g_advancedSettings.m_remoteDelay)
  {
    XBMC_Event newEvent;
    newEvent.type = XBMC_BUTTON;
    newEvent.keybutton.button = button;
    newEvent.keybutton.holdtime = XbmcThreads::SystemClockMillis() - m_firstClickTime;
    g_application.OnEvent(newEvent);
  }
}
