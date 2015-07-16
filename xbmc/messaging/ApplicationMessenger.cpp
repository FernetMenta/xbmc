/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "ApplicationMessenger.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"

#include "threads/SingleLock.h"
#include "guilib/GraphicContext.h"


namespace KODI
{
namespace MESSAGING
{

CDelayedMessage::CDelayedMessage(ThreadMessage& msg, unsigned int delay) : CThread("DelayedMessage")
{
  m_msg = msg;

  m_delay = delay;
}

void CDelayedMessage::Process()
{
  Sleep(m_delay);

  if (!m_bStop)
    CApplicationMessenger::Get().PostMsg(m_msg.dwMessage, m_msg.param1, m_msg.param1, m_msg.lpVoid, m_msg.strParam, m_msg.params);
}


CApplicationMessenger& CApplicationMessenger::Get()
{
  static CApplicationMessenger appMessenger;
  return appMessenger;
}

CApplicationMessenger::CApplicationMessenger()
{
}

CApplicationMessenger::~CApplicationMessenger()
{
  Cleanup();
}

void CApplicationMessenger::Cleanup()
{
  CSingleLock lock (m_critSection);

  while (!m_vecMessages.empty())
  {
    ThreadMessage* pMsg = m_vecMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecMessages.pop();
  }

  while (!m_vecWindowMessages.empty())
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecWindowMessages.pop();
  }
}

void CApplicationMessenger::SendMsg(ThreadMessage&& message, bool wait)
{
  std::shared_ptr<CEvent> waitEvent;
  if (wait)
  { // check that we're not being called from our application thread, else we'll be waiting
    // forever!
    if (!g_application.IsCurrentThread())
    {
      message.waitEvent.reset(new CEvent(true));
      waitEvent = message.waitEvent;
    }
    else
    {
      //OutputDebugString("Attempting to wait on a SendMessage() from our application thread will cause lockup!\n");
      //OutputDebugString("Sending immediately\n");
      ProcessMessage(&message);
      return;
    }
  }


  if (g_application.m_bStop)
    return;

  ThreadMessage* msg = new ThreadMessage(std::move(message));
  
  CSingleLock lock (m_critSection);

  if (msg->dwMessage == TMSG_GUI_DIALOG_OPEN)
    m_vecWindowMessages.push(msg);
  else
    m_vecMessages.push(msg);
  lock.Leave();  // this releases the lock on the vec of messages and
                 //   allows the ProcessMessage to execute and therefore
                 //   delete the message itself. Therefore any accesss
                 //   of the message itself after this point consittutes
                 //   a race condition (yarc - "yet another race condition")
                 //
  if (waitEvent) // ... it just so happens we have a spare reference to the
                 //  waitEvent ... just for such contingencies :)
  { 
    // ensure the thread doesn't hold the graphics lock
    CSingleExit exit(g_graphicsContext);
    waitEvent->Wait();
  }
}

void CApplicationMessenger::SendMsg(uint32_t messageId)
{
   SendMsg(ThreadMessage{ messageId }, true);
}

void CApplicationMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload)
{
  SendMsg(ThreadMessage{ messageId, param1, param2, payload }, true);
}

void CApplicationMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam)
{
  SendMsg(ThreadMessage{ messageId, param1, param2, payload, strParam, std::vector<std::string>{} }, true);
}

void CApplicationMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params)
{
  SendMsg(ThreadMessage{ messageId, param1, param2, payload, strParam, params }, true);
}

void CApplicationMessenger::PostMsg(uint32_t messageId)
{
  SendMsg(ThreadMessage{ messageId }, false);
}

void CApplicationMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload)
{
  SendMsg(ThreadMessage{ messageId, param1, param2, payload }, false);
}

void CApplicationMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam)
{
  SendMsg(ThreadMessage{ messageId, param1, param2, payload, strParam, std::vector<std::string>{} }, false);
}

void CApplicationMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params)
{
  SendMsg(ThreadMessage{ messageId, param1, param2, payload, strParam, params }, false);
}

void CApplicationMessenger::ProcessMessages()
{
  // process threadmessages
  CSingleLock lock (m_critSection);
  while (!m_vecMessages.empty())
  {
    ThreadMessage* pMsg = m_vecMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecMessages.pop();

    //Leave here as the message might make another
    //thread call processmessages or sendmessage

    std::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
    lock.Leave(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);
    if (waitEvent)
      waitEvent->Set();
    delete pMsg;

    lock.Enter();
  }
}

void CApplicationMessenger::ProcessMessage(ThreadMessage *pMsg)
{
  //special case for this that we handle ourselves
  if (pMsg->dwMessage == TMSG_CALLBACK)
  {
    ThreadMessageCallback *callback = static_cast<ThreadMessageCallback*>(pMsg->lpVoid);
    callback->callback(callback->userptr);
    return;
  }
  else if(pMsg->dwMessage == TMSG_SETAUDIODSPSTATE)
  {
    if(pMsg->param1 != 0)
      g_application.StartAudioDSPEngine();
    else
      g_application.StopAudioDSPEngine();
  }
  else if(pMsg->dwMessage == TMSG_MEDIA_RESTART)
  {
    g_application.Restart(true);
  }
  else if(pMsg->dwMessage == TMSG_MEDIA_STOP)
  {
    // restore to previous window if needed
    bool stopSlideshow = true;
    bool stopVideo = true;
    bool stopMusic = true;
    if(pMsg->param1 >= PLAYLIST_MUSIC && pMsg->param1 <= PLAYLIST_PICTURE)
    {
      stopSlideshow = (pMsg->param1 == PLAYLIST_PICTURE);
      stopVideo = (pMsg->param1 == PLAYLIST_VIDEO);
      stopMusic = (pMsg->param1 == PLAYLIST_MUSIC);
    }

    if((stopSlideshow && g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) ||
      (stopVideo && g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO) ||
      (stopMusic && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION))
      g_windowManager.PreviousWindow();

    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();

    // stop playing file
    if(g_application.m_pPlayer->IsPlaying()) g_application.StopPlaying();
  }

  CSingleLock lock(m_critSection);
  int mask = pMsg->dwMessage & TMSG_MASK_MESSAGE;

  auto target = m_mapTargets.at(mask);
  if (target != nullptr)
  {
    CSingleExit exit(m_critSection);
    target->OnApplicationMessage(pMsg);
  }
}

void CApplicationMessenger::ProcessWindowMessages()
{
  CSingleLock lock (m_critSection);
  //message type is window, process window messages
  while (!m_vecWindowMessages.empty())
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecWindowMessages.pop();

    // leave here in case we make more thread messages from this one

    std::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
    lock.Leave(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);
    if (waitEvent)
      waitEvent->Set();
    delete pMsg;

    lock.Enter();
  }
}

void CApplicationMessenger::SendGUIMessage(const CGUIMessage &message, int windowID, bool waitResult)
{
  ThreadMessage tMsg(TMSG_GUI_MESSAGE);
  tMsg.param1 = windowID == WINDOW_INVALID ? 0 : windowID;
  tMsg.lpVoid = new CGUIMessage(message);
  SendMsg(std::move(tMsg), waitResult);
}

void CApplicationMessenger::RegisterReceveiver(IMessageTarget* target)
{
  CSingleLock lock(m_critSection);
  m_mapTargets.insert(std::make_pair(target->GetMessageMask(), target));
}

void CApplicationMessenger::SetAudioDSPEngineState(bool onOff)
{
  ThreadMessage tMsg(TMSG_SETAUDIODSPSTATE);
  tMsg.param1 = onOff ? 1 : 0;
  SendMsg(std::move(tMsg), false);
}

void CApplicationMessenger::MediaRestart(bool bWait)
{
  ThreadMessage tMsg(TMSG_MEDIA_RESTART);
  SendMsg(std::move(tMsg), bWait);
}

void CApplicationMessenger::MediaStop(bool bWait /* = true */, int playlistid /* = -1 */)
{
  ThreadMessage tMsg(TMSG_MEDIA_STOP);
  tMsg.param1 = playlistid;
  SendMsg(std::move(tMsg), bWait);
}

}
}
