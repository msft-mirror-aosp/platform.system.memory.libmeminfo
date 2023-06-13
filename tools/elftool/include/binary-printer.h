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

#include <stdint.h>

namespace lib {
namespace elf {

// Class to print binaries in hex and ascii format.
class BinaryPrinter {
  public:
    static void print(uint8_t* data, uint64_t length, uint64_t vaddress);
    static void printHex(uint64_t val, int width);
    static void printDec(uint64_t val, int width);
    static void printDec(uint32_t val, int width);
};

}  // namespace elf
}  // namespace lib
