/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include "StreamLayerReadExample.h"

#include <future>
#include <iostream>

#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/utils/Base64.h>
#include <olp/dataservice/read/StreamLayerClient.h>

namespace {
constexpr auto kLogTag = "read-stream-layer-example";
constexpr auto kNumberOfThreads = 2u;

bool CreateSubscription(
    olp::dataservice::read::StreamLayerClient& client,
    olp::dataservice::read::SubscribeRequest subscribe_request) {
  auto subscribe_future = client.Subscribe(subscribe_request);
  auto subscribe_response = subscribe_future.GetFuture().get();
  if (!subscribe_response.IsSuccessful()) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Failed to create subscription - HTTP Status: %d Message: %s",
        subscribe_response.GetError().GetHttpStatusCode(),
        subscribe_response.GetError().GetMessage().c_str());
    return false;
  }
  return true;
}

int GetDataFromMessages(olp::dataservice::read::StreamLayerClient& client,
                        const olp::dataservice::read::model::Messages& result) {
  const auto& messages = result.GetMessages();
  for (const auto& message : messages) {
    // If data is greater than 1 MB, the data handle is present. The data handle
    // is a unique identifier that is used to identify content and retrieve the
    // content using GetData.
    auto handle = message.GetMetaData().GetDataHandle();
    if (handle) {
      OLP_SDK_LOG_INFO_F(kLogTag, "Message data: handle - %s, size - %lu",
                         handle.get().c_str(),
                         message.GetMetaData().GetDataSize().get());
      // use GetData(const model::Message& message) with message instance to
      // request actual data with data handle.
      auto message_future = client.GetData(message);
      auto message_result = message_future.GetFuture().get();
      if (!message_result.IsSuccessful()) {
        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "Failed to get data for data handle %s - HTTP "
                              "Status: %d Message: %s",
                              handle.get().c_str(),
                              message_result.GetError().GetHttpStatusCode(),
                              message_result.GetError().GetMessage().c_str());
        continue;
      }
      auto message_data = message_result.MoveResult();
      OLP_SDK_LOG_INFO_F(kLogTag, "GetData for %s successful: size - %lu",
                         handle.get().c_str(), message_data->size());
    } else {
      // If data is less than 1 MB, the data  content published directly in the
      // metadata and encoded in Base64.
      OLP_SDK_LOG_INFO_F(kLogTag, "Message data: size - %lu",
                         message.GetData()->size());
    }
  }
  return messages.size();
}

void RunPoll(olp::dataservice::read::StreamLayerClient& client) {
  unsigned int total_messages_size = 0;
  // Get the messages, and commit offsets till all data is consumed, or max
  // times 5
  unsigned int max_times_to_poll = 5;
  while (max_times_to_poll-- > 0) {
    auto poll_future = client.Poll();
    auto poll_response = poll_future.GetFuture().get();
    if (!poll_response.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Failed to poll data - HTTP Status: %d Message: %s",
                            poll_response.GetError().GetHttpStatusCode(),
                            poll_response.GetError().GetMessage().c_str());
      continue;
    }

    auto result = poll_response.MoveResult();
    auto messages_size = GetDataFromMessages(client, result);
    total_messages_size += messages_size;
    if (!messages_size) {
      OLP_SDK_LOG_INFO(kLogTag, "No new messages is received");
      break;
    }
  }

  if (total_messages_size > 0) {
    OLP_SDK_LOG_INFO_F(kLogTag, "Poll data - Success, messages size - %u",
                       total_messages_size);
  } else {
    OLP_SDK_LOG_INFO(kLogTag, "No messages is received");
  }
}

bool DeleteSubscription(olp::dataservice::read::StreamLayerClient& client) {
  auto unsubscribe_future = client.Unsubscribe();
  auto unsubscribe_response = unsubscribe_future.GetFuture().get();
  if (!unsubscribe_response.IsSuccessful()) {
    OLP_SDK_LOG_ERROR_F(kLogTag,
                        "Failed to unsubscribe - HTTP Status: %d Message: %s",
                        unsubscribe_response.GetError().GetHttpStatusCode(),
                        unsubscribe_response.GetError().GetMessage().c_str());
    return false;
  }
  return true;
}
}  // namespace

int RunStreamLayerExampleRead(
    const AccessKey& access_key, const std::string& catalog,
    const std::string& layer_id,
    olp::dataservice::read::SubscribeRequest::SubscriptionMode
        subscription_mode) {
  // Create a task scheduler instance
  auto task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  // Create a network client
  auto http_client = olp::client::OlpClientSettingsFactory::
      CreateDefaultNetworkRequestHandler();

  // Initialize authentication settings
  olp::authentication::Settings settings({access_key.id, access_key.secret});
  settings.task_scheduler = std::move(task_scheduler);
  settings.network_request_handler = http_client;
  // Setup AuthenticationSettings with a default token provider that will
  // retrieve an OAuth 2.0 token from OLP.
  olp::client::AuthenticationSettings auth_settings;
  auth_settings.provider =
      olp::authentication::TokenProviderDefault(std::move(settings));

  // Setup OlpClientSettings and provide it to the StreamLayerClient.
  olp::client::OlpClientSettings client_settings;
  client_settings.authentication_settings = auth_settings;
  client_settings.network_request_handler = std::move(http_client);

  // Create subscription, used kSerial or kParallel subscription mode
  olp::dataservice::read::SubscribeRequest subscribe_request;
  subscribe_request.WithSubscriptionMode(subscription_mode);

  // set options, default values described here:
  // https://developer.here.com/olp/documentation/data-api/api-reference-stream.html
  olp::dataservice::read::ConsumerOptions expected_options = {
      olp::dataservice::read::ConsumerOption("auto.commit.interval.ms",
                                             int32_t{5000}),
      olp::dataservice::read::ConsumerOption("auto.offset.reset", "earliest"),
      olp::dataservice::read::ConsumerOption("enable.auto.commit", "false"),
      olp::dataservice::read::ConsumerOption("fetch.max.bytes",
                                             int32_t{52428800}),
      olp::dataservice::read::ConsumerOption("fetch.max.wait.ms", int32_t{500}),
      olp::dataservice::read::ConsumerOption("fetch.min.bytes", int32_t{1}),
      olp::dataservice::read::ConsumerOption("group.id", "group_id_1"),
      olp::dataservice::read::ConsumerOption("max.partition.fetch.bytes",
                                             int32_t{1048576}),
      olp::dataservice::read::ConsumerOption("max.poll.records", int32_t{500})};
  subscribe_request.WithConsumerProperties(
      olp::dataservice::read::ConsumerProperties(expected_options));

  auto read_from_stream_layer = [&]() {
    // Create stream layer client with settings and catalog, layer specified
    olp::dataservice::read::StreamLayerClient client(olp::client::HRN{catalog},
                                                     layer_id, client_settings);
    if (!CreateSubscription(client, subscribe_request)) {
      return false;
    }
    RunPoll(client);
    if (!DeleteSubscription(client)) {
      return false;
    }
    return true;
  };

  int result = 0;
  if (subscription_mode ==
      olp::dataservice::read::SubscribeRequest::SubscriptionMode::kParallel) {
    // With parallel subscription you can read large volumes of data in a
    // parallel manner. The subscription and message reading workflow is similar
    // to serial subscription except that you are allowed to create multiple
    // subscriptions for the same aid, catalog, layer, group.id combination
    // using multiple processes/threads. This allows you to read and commit
    // messages for each subscription in parallel.
    OLP_SDK_LOG_INFO_F(kLogTag, "Run Poll in parallel, threads count = %u",
                       kNumberOfThreads);
    std::atomic<int> value(0);
    std::thread threads[kNumberOfThreads];
    for (auto& thread : threads) {
      thread = std::thread([&]() {
        if (!read_from_stream_layer()) {
          // acumulate result from all threads
          value.store(-1);
        }
      });
    }
    for (auto& thread : threads) {
      thread.join();
    }
    result = value.load();
  } else {
    // With serial subscription you can read  smaller volumes of data with a
    // single subscription.
    result = read_from_stream_layer() ? 0 : -1;
  }
  return result;
}
