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

#include "IPTestAdministrator.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <core/core.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE);

#ifdef WITH_CODE_COVERAGE
extern "C" void __gcov_flush();
#endif

IPTestAdministrator::IPTestAdministrator(OtherSideMain otherSideMain, void* data, const uint32_t waitTime)
   : m_sharedData(nullptr)
   , m_childPid(0)
   , m_data(data)
   , m_maxWaitTime(waitTime)
{
    ForkChildProcess(otherSideMain);
}
IPTestAdministrator::IPTestAdministrator(OtherSideMain otherSideMain, const uint32_t waitTime)
   : m_sharedData(nullptr)
   , m_childPid(0)
   , m_data(nullptr)
   , m_maxWaitTime(waitTime)
{
    ForkChildProcess(otherSideMain);
}

void IPTestAdministrator::ForkChildProcess(OtherSideMain otherSideMain)
{
   m_sharedData = static_cast<SharedData *>(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));

   pthread_mutexattr_t mutexAttr;
   pthread_mutexattr_init(&mutexAttr);
   pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
   pthread_mutex_init(&m_sharedData->m_stateMutex, &mutexAttr);
   pthread_mutex_init(&m_sharedData->m_waitingForSecondCondMutex, &mutexAttr);
   pthread_mutex_init(&m_sharedData->m_waitingForNormalCondMutex, &mutexAttr);
   pthread_mutexattr_destroy(&mutexAttr);

   pthread_condattr_t condAttr;
   pthread_condattr_init(&condAttr);
   pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
   pthread_cond_init(&m_sharedData->m_waitingForSecondCond, &condAttr);
   pthread_cond_init(&m_sharedData->m_waitingForNormalCond, &condAttr);
   pthread_condattr_destroy(&condAttr);

   pid_t childProcess = fork();

   if (childProcess == 0) {
      // In child process
      otherSideMain(*this);

      // TODO: should we clean up stuff here or not?
      //Thunder::Core::Singleton::Dispose();

      // Make sure no gtest cleanup code is called (summary etc).
      #ifdef WITH_CODE_COVERAGE
      __gcov_flush();
      #endif

      abort();
   } else {
      // In parent process, store child pid, so we can kill it later.
      m_childPid = childProcess;
   }
}

IPTestAdministrator::~IPTestAdministrator()
{
   waitpid(m_childPid, 0, 0);
}

void IPTestAdministrator::WaitForChildCompletion()
{
   waitpid(m_childPid, 0, 0);
}

bool IPTestAdministrator::Sync(const std::string & str)
{
   bool result = false;

   // Get hold of mutex guarding state.
   TimedLock(&m_sharedData->m_stateMutex, str);

   if (!m_sharedData->m_waitingForOther) {
      // We are the first.
      if (str.length() >= m_messageBufferSize) {
         fprintf(stderr, "Warning: sync string is too long: \"%s\"\n", str.c_str());
      }

      strncpy(m_sharedData->m_message, str.c_str(), m_messageBufferSize);

      // Get hold of mutex of "waiting for second" cond var.
      TimedLock(&m_sharedData->m_waitingForSecondCondMutex, str);

      m_sharedData->m_waitingForOther = true;

      // Release state mutex, because we set "waiting for other".
      pthread_mutex_unlock(&m_sharedData->m_stateMutex);

      // Wait for other side to arrive and set "m_messageTheSame"
      while (m_sharedData->m_waitingForOther) {
         TimedWait(&m_sharedData->m_waitingForSecondCond, &m_sharedData->m_waitingForSecondCondMutex, str);
      }

      // Now other side if waiting for us to set everything to normal situation.

      // Cond var was triggered, release mutex we now hold.
      pthread_mutex_unlock(&m_sharedData->m_waitingForSecondCondMutex);

      // Other side arrived and set m_messageTheSame
      result = m_sharedData->m_messageTheSame;

      // Unset "waiting for other" bool
      m_sharedData->m_waitingForOther = false;

      // Get hold of mutex guarding "back to normal" cond var
      TimedLock(&m_sharedData->m_waitingForNormalCondMutex, str);

      m_sharedData->m_backToNormal = true;

      // Signal other side everything is back to normal.
      pthread_cond_signal(&m_sharedData->m_waitingForNormalCond);

      // Unlock mutex belonging to this cond var.
      pthread_mutex_unlock(&m_sharedData->m_waitingForNormalCondMutex);
   } else {
      if (str.length() >= m_messageBufferSize) {
         fprintf(stderr, "Warning: sync string is too long: \"%s\"\n", str.c_str());
      }

      // Other side came first and set "m_message", compare.
      result = (strcmp(m_sharedData->m_message, str.c_str()) == 0);

      // Straight away we can unlock state mutex
      pthread_mutex_unlock(&m_sharedData->m_stateMutex);

      // Store result, so other side will also return it.
      m_sharedData->m_messageTheSame = result;

      // We will have to wait for other side to set everything back to normal.
      // For this we need to lock the mutex with the cond var, and wait for it.
      TimedLock(&m_sharedData->m_waitingForNormalCondMutex, str);

      // We have to mutex, so now safe to unset this variable.
      m_sharedData->m_backToNormal = false;

      // We ("other side") are done, tell other process about it.
      TimedLock(&m_sharedData->m_waitingForSecondCondMutex, str);
      m_sharedData->m_waitingForOther = false;
      pthread_cond_signal(&m_sharedData->m_waitingForSecondCond);
      pthread_mutex_unlock(&m_sharedData->m_waitingForSecondCondMutex);

      // Wait until other process tells us all is back to normal.
      while (!m_sharedData->m_backToNormal) {
         TimedWait(&m_sharedData->m_waitingForNormalCond, &m_sharedData->m_waitingForNormalCondMutex, str);
      }

      // We hold that mutex now, unlock it.
      pthread_mutex_unlock(&m_sharedData->m_waitingForNormalCondMutex);
   }

   return result;
}

const char * IPTestAdministrator::GetProcessName() const
{
   if (m_childPid != 0) {
      return "parent";
   } else {
      return "child";
   }
}

void IPTestAdministrator::TimedLock(pthread_mutex_t * mutex, const std::string & str)
{
   timespec timeSpec;
   FillTimeOut(timeSpec);
   int result = pthread_mutex_timedlock(mutex, &timeSpec);
   if (result == ETIMEDOUT) {
      fprintf(stderr, "While locking mutex, time out expired for \"%s\" in %s.\n", str.c_str(), GetProcessName());
      abort();
   }
}
void IPTestAdministrator::TimedWait(pthread_cond_t * cond, pthread_mutex_t * mutex, const std::string & str)
{
   timespec timeSpec;
   FillTimeOut(timeSpec);
   int result = pthread_cond_timedwait(cond, mutex, &timeSpec);
   if (result == ETIMEDOUT) {
      fprintf(stderr, "While waiting on cond var, time out expired for \"%s\" in %s (%u).\n", str.c_str(), GetProcessName(), getpid());
      abort();
   }
}

void IPTestAdministrator::FillTimeOut(timespec & timeSpec)
{
   clock_gettime(CLOCK_REALTIME, &timeSpec);
   timeSpec.tv_sec += m_maxWaitTime;
}
