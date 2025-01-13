# Copyright (C) 2019-2024 HERE Europe B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
# License-Filename: LICENSE

# Do not export external projects to the cmake user package registry
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY ON)

set(COMMON_PLATFORM_FLAGS)

string(REPLACE "\\" "/" CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM}")
string(REPLACE "\\" "/" CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}")
string(REPLACE "\\" "/" CMAKE_CURRENT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(COMMON_GENERATE_FLAGS -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})

if(ANDROID)
  set(COMMON_PLATFORM_FLAGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                            -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                            -DANDROID_STL=${ANDROID_STL}
                            -DANDROID_ABI=${ANDROID_ABI}
                            -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})
elseif(IOS)
  set(COMMON_PLATFORM_FLAGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                            -DPLATFORM=${PLATFORM}
                            -DSIMULATOR=${SIMULATOR})
endif()

# Setup versions for external project
set(OLP_SDK_CPP_GOOGLETEST_URL "https://github.com/google/googletest.git")
set(OLP_SDK_CPP_GOOGLETEST_TAG "release-1.12.1")

set(OLP_SDK_CPP_SNAPPY_URL "https://github.com/google/snappy.git")
set(OLP_SDK_CPP_SNAPPY_TAG "1.1.7")

set(OLP_SDK_CPP_LEVELDB_URL "https://github.com/google/leveldb.git")
set(OLP_SDK_CPP_LEVELDB_TAG "1.21")

set(OLP_SDK_CPP_RAPIDJSON_URL "https://github.com/Tencent/rapidjson.git")
set(OLP_SDK_CPP_RAPIDJSON_TAG "master")

set(OLP_SDK_CPP_BOOST_URL "https://github.com/boostorg/boost.git")
set(OLP_SDK_CPP_BOOST_TAG "boost-1.82.0")

set(OLP_SDK_CPP_LMDB_URL "https://github.com/LMDB/lmdb.git")
set(OLP_SDK_CPP_LMDB_TAG "LMDB_0.9.29")

set(OLP_SDK_CPP_ZLIB_URL "https://www.zlib.net/zlib-1.3.1.tar.gz")
set(OLP_SDK_CPP_ZLIB_HASH "SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23")

set(OLP_SDK_CPP_OPENSSL_URL "https://github.com/openssl/openssl.git")
set(OLP_SDK_CPP_OPENSSL_TAG "openssl-3.1.4")

set(OLP_SDK_CPP_CURL_URL "https://github.com/bagder/curl.git")
set(OLP_SDK_CPP_CURL_TAG "curl-7_52_1")

# Add external projects
find_package(GTest QUIET)
if(NOT TARGET GTest AND NOT GTest_FOUND)
    add_subdirectory(googletest)
endif()

find_package(RapidJSON 1.1.0 QUIET)
if(NOT TARGET RapidJSON AND NOT RapidJSON_FOUND)
    add_subdirectory(rapidjson)
endif()

if(OLP_SDK_ENABLE_DEFAULT_CACHE)
    find_package(leveldb QUIET)
    if(NOT TARGET leveldb AND NOT leveldb_FOUND)
        add_subdirectory(leveldb)
        set(leveldb_DIR ${EXTERNAL_leveldb_DIR} PARENT_SCOPE)
        set(leveldb_INCLUDE_DIR ${EXTERNAL_leveldb_INCLUDE_DIR} PARENT_SCOPE)
        set(Snappy_DIR ${EXTERNAL_Snappy_DIR} PARENT_SCOPE)
    endif()
endif()

if(OLP_SDK_ENABLE_DEFAULT_CACHE_LMDB)
    find_package(lmdb QUIET)
    if(NOT TARGET lmdb AND NOT lmdb_FOUND)
        add_subdirectory(lmdb)
        set(lmdb_DIR ${EXTERNAL_lmdb_DIR} PARENT_SCOPE)
        set(lmdb_INCLUDE_DIR ${EXTERNAL_lmdb_INCLUDE_DIR} PARENT_SCOPE)
    endif()
endif()

find_package(Boost QUIET)
if(NOT TARGET Boost AND NOT Boost_FOUND)
    add_subdirectory(boost)
    set(BOOST_ROOT ${EXTERNAL_BOOST_ROOT} PARENT_SCOPE)
    set(Boost_INCLUDE_DIR ${EXTERNAL_BOOST_ROOT_INCLUDE} PARENT_SCOPE)
    set(Boost_LIBRARY_DIR ${EXTERNAL_BOOST_ROOT_LIB} PARENT_SCOPE)
endif()

if(ANDROID AND OLP_SDK_ENABLE_ANDROID_CURL)
    find_package(CURL 7.52.0 QUIET)
    if(NOT TARGET CURL AND NOT CURL_FOUND)
        add_subdirectory(curl)
        set(CURL_DIR ${EXTERNAL_CURL_DIR} PARENT_SCOPE)
        set(CURL_INCLUDE_DIR ${EXTERNAL_CURL_INCLUDE_DIR} PARENT_SCOPE)
        set(CURL_LIBRARY ${EXTERNAL_BINARY_INSTALL_DIR}/lib/libcurl.so PARENT_SCOPE)
    endif()
endif()
