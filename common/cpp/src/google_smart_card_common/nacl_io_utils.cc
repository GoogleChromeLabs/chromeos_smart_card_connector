// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/cpp/src/google_smart_card_common/nacl_io_utils.h"

#include <sys/mount.h>

#include <nacl_io/nacl_io.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/module.h>

#include "common/cpp/src/google_smart_card_common/logging/logging.h"

namespace google_smart_card {

void InitializeNaclIo(const pp::Instance& pp_instance) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[nacl_io] Initializing...";

  pp::Module* const pp_module = pp::Module::Get();
  GOOGLE_SMART_CARD_CHECK(pp_module);

  GOOGLE_SMART_CARD_CHECK(!pp_module->core()->IsMainThread());

  GOOGLE_SMART_CARD_CHECK(
      ::nacl_io_init_ppapi(pp_instance.pp_instance(),
                           pp_module->get_browser_interface()) == 0);

  GOOGLE_SMART_CARD_LOG_DEBUG << "[nacl_io] successfully initialized";
}

void MountNaclIoFolders() {
  // Undo previous mounts in case there were any. The errors here are non-fatal.
  UnmountNaclIoFolders();

  GOOGLE_SMART_CARD_CHECK(
      ::mount("/", "/", "httpfs", 0, "manifest=/nacl_io_manifest.txt") == 0);

  GOOGLE_SMART_CARD_CHECK(::mount("", "/tmp", "memfs", 0, "") == 0);
}

bool UnmountNaclIoFolders() {
  bool success = true;
  // Try unmounting both even if one failed.
  if (::umount("/") != 0)
    success = false;
  if (::umount("/tmp") != 0)
    success = false;
  return success;
}

}  // namespace google_smart_card
