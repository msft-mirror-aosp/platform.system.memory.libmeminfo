/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <elf.h>
#include <stdint.h>
#include <vector>

#include "elf64-binary.h"

namespace lib {
namespace elf {

// Class to print the Section Headers.
class Elf64ShdrPrinter {
  public:
    Elf64ShdrPrinter(Elf64_Sc* strTabSecPtr, std::vector<Elf64_Shdr*>* shdrsPtr);
    ~Elf64ShdrPrinter();

    // Print the ELF64 section headers.
    void PrintShdrs();

  private:
    Elf64_Sc* strTabSecPtr;
    std::vector<Elf64_Shdr*>* shdrsPtr;

    void PrintShdr(Elf64_Shdr* shdrPtr);
    void PrintShdrType(uint32_t sType);
    void PrintShdrFlags(uint32_t flags);
};

}  // namespace elf
}  // namespace lib
