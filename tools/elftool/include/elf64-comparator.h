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
#include <vector>

#include "elf64-binary.h"

namespace lib {
namespace elf {

// Class to compare the ELF64 parts:
//
// - Executable header
// - Program headers
// - Sections
// - Section headers
class Elf64Comparator {
  public:
    // Compares the ELF64 Executable Headers.
    static bool AreEhdrsEqual(Elf64_Ehdr* ehdrPtr1, Elf64_Ehdr* ehdrPtr2);

    // Compares the ELF64 Program (Segment) Headers.
    static bool ArePhdrsEqual(std::vector<Elf64_Phdr*>* phdrsPtr1,
                              std::vector<Elf64_Phdr*>* phdrsPtr2);

    // Compares the ELF64 Section Headers.
    static bool AreShdrsEqual(std::vector<Elf64_Shdr*>* shdrsPtr1,
                              std::vector<Elf64_Shdr*>* shdrsPtr2);

    // Compares the ELF64 Section data.
    static bool AreSdEqual(std::vector<Elf64_Sc*>* sectionsPtr1,
                           std::vector<Elf64_Sc*>* sectionsPtr2);
};

}  // namespace elf
}  // namespace lib
