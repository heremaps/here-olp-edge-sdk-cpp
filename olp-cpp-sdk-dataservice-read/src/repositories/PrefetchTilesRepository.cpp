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

#include "PrefetchTilesRepository.h"

#include <inttypes.h>
#include <vector>

#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/Atomic.h>
#include <olp/core/thread/TaskScheduler.h>
#include "generated/api/QueryApi.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "PrefetchTilesRepository";
constexpr std::uint32_t kMaxQuadTreeIndexDepth = 4u;

model::Partition PartitionFromSubQuad(const model::SubQuad& sub_quad,
                                      const std::string& partition) {
  model::Partition ret;
  ret.SetPartition(partition);
  ret.SetDataHandle(sub_quad.GetDataHandle());
  ret.SetVersion(sub_quad.GetVersion());
  ret.SetDataSize(sub_quad.GetDataSize());
  ret.SetChecksum(sub_quad.GetChecksum());
  ret.SetCompressedDataSize(sub_quad.GetCompressedDataSize());
  return ret;
}
}  // namespace

using namespace olp::client;

PrefetchTilesRepository::PrefetchTilesRepository(
    const HRN& hrn, std::string layer_id,
    std::shared_ptr<olp::client::OlpClientSettings> settings)
    : hrn_(hrn),
      layer_id_(std::move(layer_id)),
      settings_(std::move(settings)) {}

SubQuadsRequest PrefetchTilesRepository::EffectiveTileKeys(
    const std::vector<geo::TileKey>& tile_keys, unsigned int min_level,
    unsigned int max_level) {
  SubQuadsRequest ret;
  for (auto tile_key : tile_keys) {
    auto child_tiles = EffectiveTileKeys(tile_key, min_level, max_level, true);
    for (auto tile : child_tiles) {
      // check if child already exist, if so, use the greater depth
      auto old_child = ret.find(tile.first);
      if (old_child != ret.end()) {
        ret[tile.first] = std::max((*old_child).second, tile.second);
      } else {
        ret[tile.first] = tile.second;
      }
    }
  }

  return ret;
}

SubQuadsRequest PrefetchTilesRepository::EffectiveTileKeys(
    const geo::TileKey& tile_key, unsigned int min_level,
    unsigned int max_level, bool add_ancestors) {
  SubQuadsRequest ret;
  auto current_level = tile_key.Level();

  auto AddParents = [&]() {
    auto tile{tile_key.Parent()};
    while (tile.Level() >= min_level) {
      if (tile.Level() <= max_level)
        ret[tile.ToHereTile()] = std::make_pair(tile, 0);
      tile = tile.Parent();
    }
  };

  auto AddChildren = [&](const geo::TileKey& tile) {
    auto child_tiles = EffectiveTileKeys(tile, min_level, max_level, false);
    for (auto tile : child_tiles) {
      // check if child already exist, if so, use the greater depth
      auto old_child = ret.find(tile.first);
      if (old_child != ret.end())
        ret[tile.first] = std::max((*old_child).second, tile.second);
      else
        ret[tile.first] = tile.second;
    }
  };

  if (current_level > max_level) {
    // if this tile is greater than max, find the parent of this and push the
    // ones between min and max
    AddParents();
  } else if (current_level < min_level) {
    // if this tile is less than min, find the children that are min and start
    // from there
    auto children = GetChildAtLevel(tile_key, min_level);
    for (auto child : children) {
      AddChildren(child);
    }
  } else {
    // tile is within min and max
    if (add_ancestors) {
      AddParents();
    }

    auto tile_key_str = tile_key.ToHereTile();
    if (max_level - current_level <= kMaxQuadTreeIndexDepth) {
      ret[tile_key_str] = std::make_pair(tile_key, max_level - current_level);
    } else {
      // Backend only takes MAX_QUAD_TREE_INDEX_DEPTH at a time, so we have to
      // manually calculate all the tiles that should included
      ret[tile_key_str] = std::make_pair(tile_key, kMaxQuadTreeIndexDepth);
      auto children =
          GetChildAtLevel(tile_key, current_level + kMaxQuadTreeIndexDepth + 1);
      for (auto child : children) {
        AddChildren(child);
      }
    }
  }

  return ret;
}

std::vector<geo::TileKey> PrefetchTilesRepository::GetChildAtLevel(
    const geo::TileKey& tile_key, unsigned int min_level) {
  if (tile_key.Level() >= min_level) {
    return {tile_key};
  }

  std::vector<geo::TileKey> ret;
  for (std::uint8_t index = 0; index < kMaxQuadTreeIndexDepth; ++index) {
    auto child = GetChildAtLevel(tile_key.GetChild(index), min_level);
    ret.insert(ret.end(), child.begin(), child.end());
  }

  return ret;
}

SubTilesResponse PrefetchTilesRepository::GetSubTiles(
    const std::string& layer_id, const PrefetchTilesRequest& request,
    const SubQuadsRequest& sub_quads, CancellationContext context) {
  // Version needs to be set, else we cannot move forward
  if (!request.GetVersion()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetSubTiles: catalog version missing, key=%s",
                          request.CreateKey(layer_id).c_str());
    return {{ErrorCode::InvalidArgument, "Catalog version invalid"}};
  }

  OLP_SDK_LOG_INFO_F(kLogTag, "GetSubTiles: hrn=%s, layer=%s, quads=%zu",
                     hrn_.ToString().c_str(), layer_id.c_str(),
                     sub_quads.size());

  SubTilesResult result;

  for (const auto& quad : sub_quads) {
    if (context.IsCancelled()) {
      return {{ErrorCode::Cancelled, "Cancelled", true}};
    }

    auto& tile = quad.second.first;
    auto& depth = quad.second.second;

    auto response =
        GetSubQuads(hrn_, layer_id, request, tile, depth, *settings_, context);

    if (!response.IsSuccessful()) {
      // Just abort if something else then 404 Not Found is returned
      auto& error = response.GetError();
      if (error.GetHttpStatusCode() != http::HttpStatusCode::NOT_FOUND) {
        return error;
      }
    }

    const auto& subtiles = response.GetResult();
    result.reserve(result.size() + subtiles.size());
    result.insert(result.end(), subtiles.begin(), subtiles.end());
  }

  return result;
}

SubQuadsResponse PrefetchTilesRepository::GetSubQuads(
    const HRN& catalog, const std::string& layer_id,
    const PrefetchTilesRequest& request, geo::TileKey tile, int32_t depth,
    const OlpClientSettings& settings, CancellationContext context) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetSubQuads(%s, %" PRId64 ", %" PRId32 ")",
                      tile.ToHereTile().c_str(), request.GetVersion().get(),
                      depth);

  auto query_api =
      ApiClientLookup::LookupApi(catalog, context, "query", "v1",
                                 FetchOptions::OnlineIfNotFound, settings);

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  using QuadTreeIndexResponse = QueryApi::QuadTreeIndexResponse;
  using QuadTreeIndexPromise = std::promise<QuadTreeIndexResponse>;
  auto promise = std::make_shared<QuadTreeIndexPromise>();
  auto tile_key = tile.ToHereTile();
  auto version = request.GetVersion().get();

  context.ExecuteOrCancelled(
      [&] {
        OLP_SDK_LOG_INFO_F(kLogTag,
                           "GetSubQuads execute(%s, %" PRId64 ", %" PRId32 ")",
                           tile_key.c_str(), version, depth);

        return QueryApi::QuadTreeIndex(
            query_api.GetResult(), layer_id, version, tile_key, depth,
            boost::none, request.GetBillingTag(),
            [=](QuadTreeIndexResponse response) {
              promise->set_value(std::move(response));
            });
      },
      [=]() {
        OLP_SDK_LOG_INFO_F(
            kLogTag, "GetSubQuads cancelled(%s, %" PRId64 ", %" PRId32 ")",
            tile_key.c_str(), version, depth);
        promise->set_value({{ErrorCode::Cancelled, "Cancelled", true}});
      });

  // Wait for response
  auto future = promise->get_future();
  auto quad_tree = future.get();

  if (context.IsCancelled()) {
    return {{ErrorCode::Cancelled, "Cancelled", true}};
  }

  if (!quad_tree.IsSuccessful()) {
    OLP_SDK_LOG_INFO_F(kLogTag,
                       "GetSubQuads failed(%s, %" PRId64 ", %" PRId32 ")",
                       tile_key.c_str(), version, depth);
    return quad_tree.GetError();
  }

  SubQuadsResult result;
  model::Partitions partitions;

  auto subquads = quad_tree.GetResult().GetSubQuads();
  result.reserve(subquads.size());
  partitions.GetMutablePartitions().reserve(subquads.size());

  for (auto subquad : subquads) {
    auto subtile = tile.AddedSubHereTile(subquad->GetSubQuadKey());

    // Add to result
    result.emplace_back(subtile, subquad->GetDataHandle());

    // add to bulk partitions for cacheing
    partitions.GetMutablePartitions().emplace_back(
        PartitionFromSubQuad(*subquad, subtile.ToHereTile()));
  }

  // add to cache
  repository::PartitionsCacheRepository cache(catalog, settings.cache);
  cache.Put(PartitionsRequest().WithVersion(version), partitions, layer_id,
            boost::none, false);

  return result;
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
