/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/thread/ThreadPoolTaskScheduler.h>

using SyncTaskType = std::function<void()>;
using CancellationContext = olp::client::CancellationContext;
using TaskScheduler = olp::thread::TaskScheduler;
using ThreadPool = olp::thread::ThreadPoolTaskScheduler;

namespace chrono = std::chrono;

namespace {
constexpr size_t kThreads{3u};
constexpr size_t kNumTasks{30u};
constexpr chrono::milliseconds kSleep{100};
constexpr int64_t kMaxWaitMs{1000};
}  // namespace

TEST(ThreadPoolTaskSchedulerTest, SingleUserPush) {
  SCOPED_TRACE("Single user pushes tasks");

  // Start thread pool
  auto thread_pool = std::make_shared<ThreadPool>(kThreads);
  TaskScheduler& scheduler = *thread_pool;
  std::atomic<uint32_t> counter(0u);

  // Allow threads to start
  std::this_thread::sleep_for(kSleep);

  // Add tasks to the queue, threads should start executing them
  for (uint32_t idx = 0u; idx < kNumTasks; ++idx) {
    scheduler.ScheduleTask([&](const CancellationContext&) { ++counter; });
    scheduler.ScheduleTask([&]() { ++counter; });
  }

  // Wait for threads to finish but do not exceed 1min
  const auto start = chrono::system_clock::now();
  constexpr size_t expected_tasks = 2 * kNumTasks;

  auto check_condition = [&]() {
    return counter.load() < expected_tasks &&
           chrono::duration_cast<chrono::milliseconds>(
               chrono::system_clock::now() - start)
                   .count() < kMaxWaitMs;
  };

  while (check_condition()) {
    std::this_thread::sleep_for(kSleep);
  }

  EXPECT_EQ(expected_tasks, counter.load());

  // Close queue and join threads.
  // SyncQueue and threads join should be done in destructor.
  thread_pool.reset();
}

TEST(ThreadPoolTaskSchedulerTest, MultiUserPush) {
  SCOPED_TRACE("Multiple users push tasks");

  constexpr uint32_t kPushThreads = 3;
  constexpr uint32_t kTotalTasks = kPushThreads * (2 * kNumTasks);

  // Start thread pool
  auto thread_pool = std::make_shared<ThreadPool>(kThreads);
  std::atomic<uint32_t> counter(0u);
  std::vector<std::thread> push_threads;

  // Allow threads to start
  std::this_thread::sleep_for(kSleep);

  // Create and start push threads for concurrent task creation
  push_threads.reserve(kPushThreads);
  for (size_t idx = 0; idx < kPushThreads; ++idx) {
    push_threads.emplace_back([=, &counter] {
      TaskScheduler& scheduler = *thread_pool;
      for (uint32_t idx = 0u; idx < kNumTasks; ++idx) {
        scheduler.ScheduleTask([&](const CancellationContext&) { ++counter; });
        scheduler.ScheduleTask([&]() { ++counter; });
        std::this_thread::sleep_for(kSleep / 100);
      }
    });
  }

  // Wait for threads to finish but do not exceed 1min
  const auto start = chrono::system_clock::now();
  auto check_condition = [&]() {
    return counter.load() < kTotalTasks &&
           chrono::duration_cast<chrono::milliseconds>(
               chrono::system_clock::now() - start)
                   .count() < kMaxWaitMs;
  };

  while (check_condition()) {
    std::this_thread::sleep_for(kSleep);
  }

  EXPECT_EQ(kTotalTasks, counter.load());

  // Close queue and join threads.
  // SyncQueue and threads join should be done in destructor.
  thread_pool.reset();

  for (auto& thread : push_threads) {
    thread.join();
  }
  push_threads.clear();
}

TEST(ThreadPoolTaskSchedulerTest, Prioritization) {
  auto thread_pool = std::make_shared<ThreadPool>(1);
  TaskScheduler& scheduler = *thread_pool;

  struct MockOp {
    MOCK_METHOD(void, Op, (olp::thread::Priority));
  } mockop;

  testing::Sequence sequence;

  EXPECT_CALL(mockop, Op(olp::thread::HIGH)).Times(10).InSequence(sequence);
  EXPECT_CALL(mockop, Op(olp::thread::NORMAL)).Times(10).InSequence(sequence);
  EXPECT_CALL(mockop, Op(olp::thread::LOW)).Times(10).InSequence(sequence);

  std::mutex mutex;
  std::condition_variable cv;

  scheduler.ScheduleTask([&]() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock);
  });

  const uint32_t expected_tasks = 30;
  uint32_t counter(0u);

  const olp::thread::Priority priorities[] = {
      olp::thread::LOW, olp::thread::NORMAL, olp::thread::HIGH};

  for (uint32_t i = 0; i < expected_tasks; i++) {
    auto priority = priorities[i % 3];
    scheduler.ScheduleTask(
        [&, priority]() {
          counter++;
          mockop.Op(priority);
        },
        priority);
  }

  cv.notify_all();

  const auto start = chrono::system_clock::now();
  auto check_condition = [&]() {
    return counter < expected_tasks &&
           chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() -
                                                  start) < chrono::seconds(10);
  };

  while (check_condition()) {
    std::this_thread::sleep_for(kSleep);
  }

  EXPECT_EQ(expected_tasks, counter);

  // Close queue and join threads.
  // SyncQueue and threads join should be done in destructor.
  thread_pool.reset();

  testing::Mock::VerifyAndClearExpectations(&mockop);
}
