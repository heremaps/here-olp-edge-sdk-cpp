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

#pragma once

#include <sstream>
#include <string>
#include <utility>

#include <olp/core/porting/deprecated.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/FetchOptions.h>
#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to request a list of versions for
 * the given catalog.
 */
class DATASERVICE_READ_API VersionsRequest final {
 public:
  /**
   * @brief Sets the catalog metadata start version.
   *
   * @note The beginning of the range of versions that you want to get
   * (exclusive). By convention, -1 indicates the initial version before the
   * first publication. After the first publication, the catalog version is 0.
   *
   * @param version The catalog metadata start version of the requested
   * versions list.
   *
   * @return A reference to the updated `VersionsRequest` instance.
   *
   */
  inline VersionsRequest& WithStartVersion(std::int64_t version) {
    start_version_ = version;
    return *this;
  }

  /**
   * @brief Gets the catalog metadata start version of the requested versions
   * list.
   *
   * @return The catalog metadata start version.
   *
   */
  inline std::int64_t GetStartVersion() const { return start_version_; }

  /**
   * @brief Sets the catalog metadata end version.
   *
   * @note The end of the range of versions that you want to get (inclusive). It
   * must be a valid catalog version greater than the `startVersion`. The
   * maximum value for this parameter is returned from the `/versions/latest`
   * endpoint. If this version does not exist, 400 Bad Request is returned.
   *
   * @param version The catalog metadata end version of the requested
   * versions list.
   *
   * @return A reference to the updated `VersionsRequest` instance.
   *
   */
  inline VersionsRequest& WithEndVersion(std::int64_t version) {
    end_version_ = version;
    return *this;
  }

  /**
   * @brief Gets the catalog metadata end version of the requested versions
   * list.
   *
   * @return The catalog metadata end version.
   *
   */
  inline std::int64_t GetEndVersion() const { return end_version_; }

  /**
   * @brief Gets the billing tag to group billing records together.
   *
   * The billing tag is an optional free-form tag that is used for grouping
   * billing records together. If supplied, it must be 4–16 characters
   * long and contain only alphanumeric ASCII characters [A-Za-z0-9].
   *
   * @return The `BillingTag` string or `boost::none` if the billing tag is not
   * set.
   */
  inline const boost::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param tag The `BillingTag` string or `boost::none`.
   *
   * @return A reference to the updated `VersionsRequest` instance.
   */
  inline VersionsRequest& WithBillingTag(boost::optional<std::string> tag) {
    billing_tag_ = std::move(tag);
    return *this;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param tag The rvalue reference to the `BillingTag` string or
   * `boost::none`.
   *
   * @return A reference to the updated `VersionsRequest` instance.
   */
  inline VersionsRequest& WithBillingTag(std::string tag) {
    billing_tag_ = std::move(tag);
    return *this;
  }

  /**
   * @brief Gets the fetch option that controls how requests are handled.
   *
   * The default option is `OnlineIfNotFound` that queries the network if
   * the requested resource is not in the cache.
   *
   * @return The fetch option.
   */
  inline FetchOptions GetFetchOption() const { return fetch_option_; }

  /**
   * @brief Sets the fetch option that you can use to set the source from
   * which data should be fetched.
   *
   * @see `GetFetchOption()` for information on usage and format.
   *
   * @param fetch_option The `FetchOption` enum.
   *
   * @return A reference to the updated `VersionsRequest` instance.
   */
  inline VersionsRequest& WithFetchOption(FetchOptions fetch_option) {
    fetch_option_ = fetch_option;
    return *this;
  }

  /**
   * @brief Creates a readable format for the request.
   *
   * @return A string representation of the request.
   */
  inline std::string CreateKey() const {
    std::stringstream out;
    out << "[";
    out << GetStartVersion() << ", " << GetEndVersion();
    out << "]";
    if (GetBillingTag()) {
      out << "$" << GetBillingTag().get();
    }
    return out.str();
  }

 private:
  std::int64_t start_version_;
  std::int64_t end_version_;
  boost::optional<std::string> billing_tag_;
  FetchOptions fetch_option_{OnlineIfNotFound};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
