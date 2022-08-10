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

#ifndef GOOGLE_SMART_CARD_COMMON_NACL_IO_UTILS_H_
#define GOOGLE_SMART_CARD_COMMON_NACL_IO_UTILS_H_

#ifndef __native_client__
#error "This file should only be used in Native Client builds"
#endif  // __native_client__

#include <ppapi/cpp/instance.h>

namespace google_smart_card {

// Initializes NaCl nacl_io library.
//
// Note: This function must be called not from the main Pepper thread!
void InitializeNaclIo(const pp::Instance& pp_instance);

// Mounts / and /tmp directories.
//
// The / directory is mounted to the executable module's folder (which, in
// extension/app environment, is the contents of the .crx package).
//
// The /tmp directory is mounted to a temporary in-memory file system.
void MountNaclIoFolders();

// Unmounts directories mounted by `MountNaclIoFolders()`.
bool UnmountNaclIoFolders();

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_NACL_IO_UTILS_H_
