// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "elf_reader.h"

#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

class ScopedFile {
 public:
  explicit ScopedFile(FILE* fp) : fp_(fp) {}
  ~ScopedFile() {
    if (fp_)
      fclose(fp_);
  }

  FILE* get() { return fp_; }

 private:
  FILE* fp_;
};

ElfReader::ElfReader(const char* filename)
    : filename_(filename), is_valid_(false), is_static_(false) {
  ScopedFile fp(fopen(filename, "rb"));
  if (!fp.get()) {
    PrintError("failed to open file");
    return;
  }

  std::vector<Elf64_Phdr> phdrs;
  if (!ReadHeaders(fp.get(), &phdrs))
    return;

  Elf64_Addr straddr = 0;
  size_t strsize = 0;
  std::vector<int> neededs;
  if (!ReadDynamic(fp.get(), phdrs, &straddr, &strsize, &neededs))
    return;

  std::string strtab;
  if (!ReadStrtab(fp.get(), phdrs, straddr, strsize, &strtab))
    return;

  for (size_t i = 0; i < neededs.size(); i++)
    neededs_.push_back(strtab.data() + neededs[i]);

  is_valid_= true;
}

bool ElfReader::ReadHeaders(FILE* fp, std::vector<Elf64_Phdr>* phdrs) {
  Elf32_Ehdr ehdr32;
  if (fread(&ehdr32, sizeof(ehdr32), 1, fp) != 1) {
    PrintError("failed to read ELF header");
    return false;
  }

  if (memcmp(ELFMAG, ehdr32.e_ident, SELFMAG)) {
    PrintError("not an ELF file");
    return false;
  }

  elf_class_ = ehdr32.e_ident[EI_CLASS];
  if (elf_class_ != ELFCLASS32 && elf_class_ != ELFCLASS64) {
    PrintError("bad ELFCLASS");
    return false;
  }

  Elf64_Ehdr ehdr64;
  if (elf_class_ == ELFCLASS64) {
    if (fseek(fp, 0, SEEK_SET) < 0) {
      PrintError("failed to seek back to ELF header");
      return false;
    }
    if (fread(&ehdr64, sizeof(ehdr64), 1, fp) != 1) {
      PrintError("failed to read ELF64 header");
      return false;
    }
  }

  if (elf_class_ == ELFCLASS32) {
    machine_ = ehdr32.e_machine;
  } else {
    machine_ = ehdr64.e_machine;
  }

  uint64_t off;
  if (elf_class_ == ELFCLASS32) {
    off = ehdr32.e_phoff;
  } else {
    off = ehdr64.e_phoff;
  }
  if (fseek(fp, off, SEEK_SET) < 0) {
    PrintError("failed to seek to program header");
    return false;
  }

  int phnum;
  if (elf_class_ == ELFCLASS32) {
    phnum = ehdr32.e_phnum;
  } else {
    phnum = ehdr64.e_phnum;
  }
  for (int i = 0; i < phnum; i++) {
    Elf64_Phdr phdr;
    if (elf_class_ == ELFCLASS32) {
      Elf32_Phdr phdr32;
      if (fread(&phdr32, sizeof(phdr32), 1, fp) != 1) {
        PrintError("failed to read a program header %d", i);
        return false;
      }
      phdr.p_type = phdr32.p_type;
      phdr.p_offset = phdr32.p_offset;
      phdr.p_vaddr = phdr32.p_vaddr;
      phdr.p_paddr = phdr32.p_paddr;
      phdr.p_filesz = phdr32.p_filesz;
      phdr.p_memsz = phdr32.p_memsz;
      phdr.p_flags = phdr32.p_flags;
      phdr.p_align = phdr32.p_align;
    } else {
      if (fread(&phdr, sizeof(phdr), 1, fp) != 1) {
        PrintError("failed to read a program header %d", i);
        return false;
      }
    }
    phdrs->push_back(phdr);
  }
  return true;
}

bool ElfReader::ReadDynamic(FILE* fp, const std::vector<Elf64_Phdr>& phdrs,
                            Elf64_Addr* straddr, size_t* strsize,
                            std::vector<int>* neededs) {
  bool dynamic_found = false;
  for (size_t i = 0; i < phdrs.size(); i++) {
    const Elf64_Phdr& phdr = phdrs[i];
    if (phdr.p_type != PT_DYNAMIC)
      continue;

    // NaCl glibc toolchain creates a dynamic segment with no contents
    // for statically linked binaries.
    if (phdr.p_filesz == 0) {
      PrintError("dynamic segment without no content");
      return false;
    }

    dynamic_found = true;

    if (fseek(fp, phdr.p_offset, SEEK_SET) < 0) {
      PrintError("failed to seek to dynamic segment");
      return false;
    }

    for (;;) {
      Elf64_Dyn dyn;
      if (elf_class_ == ELFCLASS32) {
        Elf32_Dyn dyn32;
        if (fread(&dyn32, sizeof(dyn32), 1, fp) != 1) {
          PrintError("failed to read a dynamic entry");
          return false;
        }
        dyn.d_tag = dyn32.d_tag;
        // TODO(bradnelson): This relies on little endian arches, fix.
        dyn.d_un.d_ptr = dyn32.d_un.d_ptr;
      } else {
        if (fread(&dyn, sizeof(dyn), 1, fp) != 1) {
          PrintError("failed to read a dynamic entry");
          return false;
        }
      }

      if (dyn.d_tag == DT_NULL)
        break;
      if (dyn.d_tag == DT_STRTAB)
        *straddr = dyn.d_un.d_ptr;
      else if (dyn.d_tag == DT_STRSZ)
        *strsize = dyn.d_un.d_val;
      else if (dyn.d_tag == DT_NEEDED)
        neededs->push_back(dyn.d_un.d_val);
    }
  }

  if (!dynamic_found) {
    is_valid_ = true;
    is_static_ = true;
    return false;
  }
  if (!strsize) {
    PrintError("DT_STRSZ does not exist");
    return false;
  }
  if (!straddr) {
    PrintError("DT_STRTAB does not exist");
    return false;
  }
  return true;
}

bool ElfReader::ReadStrtab(FILE* fp, const std::vector<Elf64_Phdr>& phdrs,
                           Elf64_Addr straddr, size_t strsize,
                           std::string* strtab) {
  // DT_STRTAB is specified by a pointer to a virtual address
  // space. We need to convert this value to a file offset. To do
  // this, we find a PT_LOAD segment which contains the address.
  Elf64_Addr stroff = 0;
  for (size_t i = 0; i < phdrs.size(); i++) {
    const Elf64_Phdr& phdr = phdrs[i];
    if (phdr.p_type == PT_LOAD &&
        phdr.p_vaddr <= straddr && straddr < phdr.p_vaddr + phdr.p_filesz) {
      stroff = straddr - phdr.p_vaddr + phdr.p_offset;
      break;
    }
  }
  if (!stroff) {
    PrintError("no segment which contains DT_STRTAB");
    return false;
  }

  strtab->resize(strsize);
  if (fseek(fp, stroff, SEEK_SET) < 0) {
    PrintError("failed to seek to dynamic strtab");
    return false;
  }
  if (fread(&(*strtab)[0], 1, strsize, fp) != strsize) {
    PrintError("failed to read dynamic strtab");
    return false;
  }
  return true;
}

void ElfReader::PrintError(const char* fmt, ...) {
  static const int kBufSize = 256;
  char buf[kBufSize];
  va_list ap;
  va_start(ap, fmt);
  int written = vsnprintf(buf, kBufSize - 1, fmt, ap);
  assert(written < kBufSize);
  if (written >= kBufSize)
    buf[kBufSize-1] = '\0';

  if (errno)
    fprintf(stderr, "%s: %s: %s\n", filename_, buf, strerror(errno));
  else
    fprintf(stderr, "%s: %s\n", filename_, buf);
}

#if defined(DEFINE_ELF_READER_MAIN)

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <elf>\n", argv[0]);
    return 1;
  }

  // For test.
  if (!getenv("LD_LIBRARY_PATH"))
    setenv("LD_LIBRARY_PATH", ".", 1);

  ElfReader elf_reader(argv[1]);
  if (!elf_reader.is_valid())
    return 1;

  for (size_t i = 0; i < elf_reader.neededs().size(); i++) {
    if (i)
      printf(" ");
    printf("%s", elf_reader.neededs()[i].c_str());
  }
}

#endif  // DEFINE_ELF_READER_MAIN
