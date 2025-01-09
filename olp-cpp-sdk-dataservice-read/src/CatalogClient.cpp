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

#include "olp/dataservice/read/CatalogClient.h"

#include <olp/core/porting/make_unique.h>
#include "CatalogClientImpl.h"

namespace olp {
namespace dataservice {
namespace read {

CatalogClient::CatalogClient(client::HRN catalog,
                             client::OlpClientSettings settings)
    : impl_(std::make_unique<CatalogClientImpl>(std::move(catalog),
                                                std::move(settings))) {}

CatalogClient::CatalogClient(CatalogClient&& other) noexcept = default;

CatalogClient& CatalogClient::operator=(CatalogClient&& other) noexcept =
    default;

CatalogClient::~CatalogClient() = default;

bool CatalogClient::CancelPendingRequests() {
  return impl_->CancelPendingRequests();
}

client::CancellationToken CatalogClient::GetCatalog(
    CatalogRequest request, CatalogResponseCallback callback) {
  return impl_->GetCatalog(std::move(request), std::move(callback));
}

client::CancellableFuture<CatalogResponse> CatalogClient::GetCatalog(
    CatalogRequest request) {
  return impl_->GetCatalog(std::move(request));
}

client::CancellationToken CatalogClient::GetLatestVersion(
    CatalogVersionRequest request, CatalogVersionCallback callback) {
  return impl_->GetLatestVersion(std::move(request), std::move(callback));
}

client::CancellableFuture<CatalogVersionResponse>
CatalogClient::GetLatestVersion(CatalogVersionRequest request) {
  return impl_->GetLatestVersion(std::move(request));
}

CatalogVersionResponse CatalogClient::GetLatestVersion(
    CatalogVersionRequest request, client::CancellationContext context) {
  return impl_->GetLatestVersion(std::move(request), std::move(context));
}

client::CancellationToken CatalogClient::ListVersions(
    VersionsRequest request, VersionsResponseCallback callback) {
  return impl_->ListVersions(std::move(request), std::move(callback));
}

client::CancellableFuture<VersionsResponse> CatalogClient::ListVersions(
    VersionsRequest request) {
  return impl_->ListVersions(std::move(request));
}

client::CancellationToken CatalogClient::GetCompatibleVersions(
    CompatibleVersionsRequest request, CompatibleVersionsCallback callback) {
  return impl_->GetCompatibleVersions(std::move(request), std::move(callback));
}

client::CancellableFuture<CompatibleVersionsResponse>
CatalogClient::GetCompatibleVersions(CompatibleVersionsRequest request) {
  return impl_->GetCompatibleVersions(std::move(request));
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
