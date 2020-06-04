// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NACL_SPAWN_ELF_READER_H_
#define NACL_SPAWN_ELF_READER_H_

#include <elf.h>
#include <stdio.h>

#include <string>
#include <vector>

// An ELF reader which extracts shared objects which are necessary to
// run the specified binary (DT_NEEDED). As no NaCl binary in the NaCl
// SDK does not have DT_RPATH and DT_RUNPATH (as of Jan. 2014), we do
// not support them.
class ElfReader {
 public:
  explicit ElfReader(const char* filename);

  bool is_valid() const { return is_valid_; }
  bool is_static() const { return is_static_; }
  Elf64_Half machine() const { return machine_; }
  const std::vector<std::string>& neededs() const { return neededs_; }

 private:
  bool ReadHeaders(FILE* fp, std::vector<Elf64_Phdr>* phdrs);
  bool ReadDynamic(FILE* fp, const std::vector<Elf64_Phdr>& phdrs,
                   Elf64_Addr* straddr, size_t* strsize,
                   std::vector<int>* neededs);
  bool ReadStrtab(FILE* fp, const std::vector<Elf64_Phdr>& phdrs,
                  Elf64_Addr straddr, size_t strsize,
                  std::string* strtab);
  void PrintError(const char* fmt, ...);

  const char* filename_;
  bool is_valid_;
  bool is_static_;
  Elf64_Half machine_;
  unsigned char elf_class_;
  std::vector<std::string> neededs_;
};

#endif  // NACL_SPAWN_ELF_READER_H_
