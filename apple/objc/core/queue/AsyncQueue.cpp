/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <WCDB/Assertion.hpp>
#include <WCDB/AsyncQueue.hpp>
#include <WCDB/CoreConst.h>
#include <WCDB/Exiting.hpp>
#include <WCDB/String.hpp>
#include <atomic>
#include <thread>

namespace WCDB {

AsyncQueue::AsyncQueue(const String& name_)
: name(name_), m_running(false), m_started(false)
{
}

AsyncQueue::~AsyncQueue()
{
    LockGuard lockGuard(m_lock);
    while (m_running) {
        // wait until done
        m_conditional.wait_for(m_lock, AsyncQueueTimeOutForExiting);
    }
    WCTRemedialAssert(
    !m_running, String::formatted("Queue: %s does not exit on time.", name.c_str()), ;);
}

void AsyncQueue::run()
{
    LockGuard lockGuard(m_lock);
    if (!m_started && !isExiting()) {
        m_started = true;
#warning TODO
        std::thread(std::bind(&AsyncQueue::doRun, this)).detach();
    }
}

void AsyncQueue::doRun()
{
    Thread::setName(name);
    if (!isExiting()) {
        m_running.store(true);
        loop();
        LockGuard lockGuard(m_lock);
        m_running.store(false);
        m_conditional.signal();
    }
}

void AsyncQueue::lazyRun()
{
    // lock free if it's already running
    if (!m_running.load()) {
        run();
    }
}

} // namespace WCDB
