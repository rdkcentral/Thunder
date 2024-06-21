/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#define MODULE_NAME ThunderUnitTests

#include <string>
#include <time.h>

class IPTestAdministrator;

class IPTestAdministrator
{
private:
   static constexpr uint32_t MaxWaitTime = 2; // In seconds

public:
   typedef void (*OtherSideMain)(IPTestAdministrator & testAdmin);

   IPTestAdministrator(OtherSideMain otherSideMain, const uint32_t waitTime = MaxWaitTime);
   IPTestAdministrator(OtherSideMain otherSideMain, void* data, const uint32_t waitTime = MaxWaitTime);
   ~IPTestAdministrator();

   void ForkChildProcess(OtherSideMain otherSideMain);
   // Method to sync the two test processes.
   bool Sync(const std::string & str);
   void WaitForChildCompletion();

   void* Data() { return m_data; }
private:
   static const uint32_t m_messageBufferSize = 1024;

   struct SharedData
   {
      pthread_mutex_t m_stateMutex; // Guards state (are we first or second?)
      pthread_cond_t m_waitingForSecondCond; // Used to wait for second
      pthread_mutex_t m_waitingForSecondCondMutex;
      bool m_waitingForOther;   // whether we are waiting for the other side or not
      bool m_messageTheSame;    // whether message comparison was a success
      char m_message[m_messageBufferSize];     // Expected message string
      bool m_backToNormal;
      pthread_cond_t m_waitingForNormalCond; // To wait for first to restore state to normal
      pthread_mutex_t m_waitingForNormalCondMutex;
   };

   SharedData * m_sharedData;
   pid_t m_childPid; // Set if we are parent processs.
   void* m_data;
   uint32_t m_maxWaitTime; // In seconds.

   const char * GetProcessName() const;
   void TimedLock(pthread_mutex_t * mutex, const std::string & str);
   void TimedWait(pthread_cond_t * cond, pthread_mutex_t * mutex, const std::string & str);
   
   void FillTimeOut(timespec & timeSpec);
};
