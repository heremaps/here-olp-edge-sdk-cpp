#!/bin/bash -ex
#
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

#
# This script will compile Data SDK for C++
# with ANDROID_PLATFORM=android-28 and -DANDROID_ABI=arm64-v8a
# by using Android NDK 21.
#

# Install required NDK version (output disabled as it causes issues during page loading)
env
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --list
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --install "ndk;21.3.6528147" --sdk_root=${ANDROID_HOME} >/dev/null
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --install "platforms;android-28" >/dev/null
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --list
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/21.3.6528147
env
# Verify content of NDK directories
ls -la $ANDROID_NDK_HOME
ls -la $ANDROID_NDK_HOME/platforms

mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="$ANDROID_HOME/ndk/21.3.6528147/build/cmake/android.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DANDROID_PLATFORM=android-28 \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NDK="$ANDROID_HOME/ndk/21.3.6528147" \
  -DOLP_SDK_ENABLE_TESTING=NO \
  -DOLP_SDK_ENABLE_ANDROID_CURL=ON \
  -DOLP_SDK_BUILD_EXAMPLES=ON

#cmake --build . # this is alternative option for build
sudo make install -j$(nproc)

pushd examples/android
  # check java version
  echo "java is $(java --version)"
  echo "sudo java is $(sudo java --version)"
  ./gradlew assemble --stacktrace
popd
