/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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

#include <functional>
#include <memory>

#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/porting/deprecated.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/CompatibleVersionsRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/Types.h>
#include <olp/dataservice/read/VersionsRequest.h>

namespace olp {

namespace client {
class HRN;
}  // namespace client

namespace dataservice {
namespace read {

class CatalogClientImpl;

/**
 * @brief Provides a high-level interface to access data hosted on `OLP` using
 * the Data API.
 *
 * The behavior of the `CatalogClient` object can be defined via
 * `OlpClientSettings`.
 *
 * You can overwrite the default implementation for the following items:
 *   * The task scheduler.
 *     By default, all request calls are performed synchronously.
 *   * The network.
 *     You can set the default implementation
 *     (`olp::client::OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler`)
 *     or pass a custom network implementation to the `CatalogClient` object.
 *   * The disk cache.
 *     By default, the `CatalogClient` object uses the default implementation
 *     of `olp::cache::DefaultCache`.
 */
class DATASERVICE_READ_API CatalogClient final {
 public:
  /**
   * @brief Creates the `CatalogClient` instance.
   *
   * @param catalog The HERE Resource Name (HRN) of the `OLP` catalog.
   * @param settings The desired configuration of the `CatalogClient` instance.
   */
  CatalogClient(client::HRN catalog, client::OlpClientSettings settings);

  /// A copy constructor.
  CatalogClient(const CatalogClient& other) = delete;

  /// A default move constructor.
  CatalogClient(CatalogClient&& other) noexcept;
  CatalogClient& operator=(const CatalogClient& other) = delete;

  /// A copy assignment operator.
  CatalogClient& operator=(CatalogClient&& other) noexcept;

  ~CatalogClient();

  /**
   * @brief Cancels the currently active requests.
   *
   * @return True on success.
   */
  bool CancelPendingRequests();

  /**
   * @brief Gets the catalog configuration asynchronously.
   *
   * @param request The `CatalogRequest` instance that contains a complete set
   * of request parameters.
   * @param callback The `CatalogResponseCallback` object that is invoked if
   * the catalog configuration is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetCatalog(CatalogRequest request,
                                       CatalogResponseCallback callback);

  /**
   * @brief Gets the catalog configuration asynchronously.
   *
   * @param request The `CatalogRequest` instance that contains a complete set
   * of request parameters.
   *
   * @return `CancellableFuture` that contains the `CatalogResponse`
   * instance with the catalog configuration or an error. You can also
   * use `CancellableFuture` to cancel this request.
   */
  client::CancellableFuture<CatalogResponse> GetCatalog(CatalogRequest request);

  /**
   * @brief Gets the catalog version asynchronously.
   *
   * @note In case you call this API with `FetchOptions::CacheOnly` and a valid
   * version in `CatalogVersionRequest::WithStartVersion()`, i.e. >= 0, then
   * please make sure that the provided version is a existing catalog version as
   * it will be written for later use to the cache as latest version in the
   * following cases:
   * - There is no latest version yet written to cache.
   * - The latest version written to cache is less then the provided version.
   *
   * @param request The `CatalogVersionRequest` instance that contains
   * a complete set of request parameters.
   * @param callback The `CatalogVersionCallback` object that is invoked if
   * the catalog version is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetLatestVersion(CatalogVersionRequest request,
                                             CatalogVersionCallback callback);

  /**
   * @brief Gets the catalog version asynchronously.
   *
   * @note In case you call this API with `FetchOptions::CacheOnly` and a valid
   * version in `CatalogVersionRequest::WithStartVersion()`, i.e. >= 0, then
   * please make sure that the provided version is a existing catalog version as
   * it will be written for later use to the cache as latest version in the
   * following cases:
   * - There is no latest version yet written to cache.
   * - The latest version written to cache is less then the provided version.
   *
   * @param request The `CatalogVersionRequest` instance that contains
   * a complete set of request parameters.
   *
   * @return `CancellableFuture` that contains the `CatalogVersionResponse`
   * instance with the catalog configuration or an error. You can also
   * use `CancellableFuture` to cancel this request.
   */
  client::CancellableFuture<CatalogVersionResponse> GetLatestVersion(
      CatalogVersionRequest request);

  /**
   * @brief Gets the catalog version synchronously.
   *
   * @param request The `CatalogVersionRequest` instance that contains
   * a complete set of request parameters.
   * @param context The `CancellationContext` instance.
   *
   * @return `CatalogVersionResponse` instance with the catalog configuration or
   * an error
   */
  CatalogVersionResponse GetLatestVersion(CatalogVersionRequest request,
                                          client::CancellationContext context);

  /**
   * @brief Gets the catalog versions list.
   *
   * @note Request of catalog versions list works only online.
   *
   * @param request The `VersionsRequest` instance that contains
   * a complete set of request parameters.
   * @param callback The `VersionsResponseCallback` object that is invoked if
   * the list of versions is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken ListVersions(VersionsRequest request,
                                         VersionsResponseCallback callback);

  /**
   * @brief Gets the catalog versions list.
   *
   * @note Request of catalog versions list works only online.
   *
   * @param request The `VersionsRequest` instance that contains
   * a complete set of request parameters.
   *
   * @return CancellableFuture` that contains the `VersionsResponse`
   * instance with the list of versions or an error. You can also
   * use `CancellableFuture` to cancel this request.
   */
  client::CancellableFuture<VersionsResponse> ListVersions(
      VersionsRequest request);

  /**
   * @brief Gets the list of the current catalog versions that are compatible
   * with the dependencies provided by the request.
   *
   * @note This request is online only. It does not support
   * multiple pages and returns only the first page.
   *
   * @param request The `CompatibleVersionsRequest` instance that contains a
   * complete set of the request parameters.
   * @param callback The `CompatibleVersionsCallback` object that is invoked if
   * the compatible versions are available or an error occurred.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetCompatibleVersions(
      CompatibleVersionsRequest request, CompatibleVersionsCallback callback);

  /**
   * @brief Gets the list of the current catalog versions that are compatible
   * with the dependencies provided by the request.
   *
   * @note This request is online only. It does not support
   * multiple pages and returns only the first page.
   *
   * @param request The `CompatibleVersionsRequest` instance that contains a
   * complete set of the request parameters.
   *
   * @return `CancellableFuture` that contains the `VersionsResponse`
   * instance with the list of versions or an error. You can also
   * use `CancellableFuture` to cancel this request.
   */
  client::CancellableFuture<CompatibleVersionsResponse> GetCompatibleVersions(
      CompatibleVersionsRequest request);

 private:
  std::unique_ptr<CatalogClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
