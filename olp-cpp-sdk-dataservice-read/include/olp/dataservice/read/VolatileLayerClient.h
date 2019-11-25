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

#pragma once

#include <memory>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/Types.h>

namespace olp {
namespace dataservice {
namespace read {
class VolatileLayerClientImpl;

/**
 * @brief The VolatileLayerClient is aimed at acquiring data from a volatile
 * layer of the OLP. The volatile layer is a key/value store. Values for a given
 * key can change, and only the latest value is retrievable.
 *
 * Example:
 * @code{.cpp}
 * auto task_scheduler =
 *    olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);
 * auto http_client = olp::client::
 *    OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
 *
 * olp::authentication::Settings settings{"Your.Key.Id", "Your.Key.Secret"};
 * settings.task_scheduler = task_scheduler;
 * settings.network_request_handler = http_client;
 *
 * olp::client::AuthenticationSettings auth_settings;
 * auth_settings.provider =
 *     olp::authentication::TokenProviderDefault(std::move(settings));
 *
 * auto client_settings = olp::client::OlpClientSettings();
 * client_settings.authentication_settings = auth_settings;
 * client_settings.task_scheduler = std::move(task_scheduler);
 * client_settings.network_request_handler = std::move(http_client);
 *
 * VolatileLayerClient client{"hrn:here:data:::your-catalog-hrn" "your-layer-id", client_settings};
 * auto callback = [](olp::client::ApiResponse<olp::model::Data, olp::client::ApiError> response) {};
 * auto request = DataRequest().WithPartitionId("269");
 * auto token = client.GetData(request, callback);
 * @endcode
 *
 * @see
 * https://developer.here.com/olp/documentation/data-api/data_dev_guide/shared_content/topics/olp/concepts/layers.html
 */
class DATASERVICE_READ_API VolatileLayerClient final {
 public:
  /**
   * @brief VolatileLayerClient constructor.
   * @param catalog a catalog that the volatile layer client uses during
   * requests.
   * @param layer_id a layer id that the volatile layer client uses during
   * requests.
   * @param settings settings used to control the client instance behavior.
   */
  VolatileLayerClient(client::HRN catalog, std::string layer_id,
                      client::OlpClientSettings settings);

  // Movable, non-copyable
  VolatileLayerClient(const VolatileLayerClient& other) = delete; 
  VolatileLayerClient(VolatileLayerClient&& other) noexcept; 
  VolatileLayerClient& operator=(const VolatileLayerClient& other) = delete; 
  VolatileLayerClient& operator=(VolatileLayerClient&& other) noexcept;

  ~VolatileLayerClient();

  /**
   * @brief Cancels all active and pending requests.
   * @return True on success.
   */
  bool CancelPendingRequests();

  /**
   * @brief Fetches a list partitions for given volatile layer asynchronously.
   * @note If your layer has lots of partitions or uses TileKeys as
   * partition IDs, then this operation can fail because of the large amount of data.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the list of partitions is available,
   * or an error is encountered.
   * @return A token that can be used to cancel this request
   */
  client::CancellationToken GetPartitions(PartitionsRequest request,
                                          PartitionsResponseCallback callback);

  /**
   * @brief Fetches a list partitions for given volatile layer asynchronously.
   * @note If your layer has lots of partitions or uses TileKeys as
   * partition IDs, then this operation can fail because of the large amount of data.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture, which when complete will contain the
   * PartitionsResponse or an error. Alternatively, the CancellableFuture can be
   * used to cancel this request.
   */
  olp::client::CancellableFuture<PartitionsResponse> GetPartitions(
      PartitionsRequest request);

  /**
   * @brief GetData fetches data for a partition or data handle asynchronously.
   * If the specified partition or data handle cannot be found in the layer, the
   * callback is invoked with an empty DataResponse (a nullptr result and
   * error). If Partition Id or Data Handle are not set in the request, the
   * callback is invoked with the error ErrorCode::InvalidRequest.
   * @param request Contains a complete set of request parameters.
   * @param callback is invoked once the DataResult is available, or an
   * error is encountered.
   * @return Token that can be used to cancel this request.
   */
  olp::client::CancellationToken GetData(DataRequest request,
                                         DataResponseCallback callback);

  /**
   * @brief Fetches data for a partition or data handle asynchronously.
   * If the specified partition or data handle cannot be found in the layer, the
   * callback is invoked with an empty DataResponse (a nullptr result and
   * error). If Partition Id or Data Handle are not set in the request, the
   * callback is invoked with the error ErrorCode::InvalidRequest.
   * @param request contains a complete set of request parameters.
   * @return CancellableFuture, which when complete will contain the
   * PartitionsResponse or an error. Alternatively, the CancellableFuture can be
   * used to cancel this request.
   */
  olp::client::CancellableFuture<DataResponse> GetData(DataRequest request);

 private:
  std::unique_ptr<VolatileLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
