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

namespace lib {
namespace elf {

// Class to print the ELF64 executable header.
class Elf64EhdrPrinter {
  public:
    Elf64EhdrPrinter(Elf64_Ehdr* ehdrPtr);
    ~Elf64EhdrPrinter();

    void PrintEhdr();

  private:
    Elf64_Ehdr* ehdrPtr;
    void PrintElfIdent();
    void PrintElfClass();
    void PrintElfData();
    void PrintElfVersion();
    void PrintElfOsAbi();
    void PrintElfFileType();
    void PrintMachineArch();
};

}  // namespace elf
}  // namespace lib
