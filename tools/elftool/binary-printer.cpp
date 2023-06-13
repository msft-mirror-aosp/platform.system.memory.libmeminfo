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

#include "binary-printer.h"

#include <stdint.h>
#include <iomanip>
#include <iostream>

using namespace std;

namespace lib {
namespace elf {

// Prints the data in hexadecimal and ASCII format. The output will look like
//
// 0001c140  6d 69 74 65 72 5f 62 61  73 65 49 50 50 38 45 6c  |miter_baseIPP8El|
// 0001c150  66 36 34 5f 53 63 45 44  54 63 6c 31 32 5f 5f 6d  |f64_ScEDTcl12__m|
//
//
// @param data contains a pointer to an array of {@code uint8_t}.
// @param length contains the number of bytes to print.
// @param vaddress contains the initial address to be printed in the left hand
//        side of the first line.
void BinaryPrinter::print(uint8_t* data, uint64_t length, uint64_t vaddress) {
    const int kBytesRow = 16;

    // Prints the rows that contain the hex and ASCII representation.
    for (int i = 0; i < length; i += kBytesRow) {
        if (i != 0) std::cout << std::endl;

        // Print virtual Address.
        std::cout << std::showbase << std::internal << std::setfill('0') << std::hex
                  << std::setw(10) << vaddress << "   ";
        vaddress += kBytesRow;

        // Prints Hexadecimal representation.
        for (int byteIdx = 0; byteIdx < kBytesRow; byteIdx++) {
            if (byteIdx != 0 && byteIdx % 8 == 0) std::cout << " ";

            if ((byteIdx + i) < length) {
                std::cout << std::noshowbase << std::internal << std::setfill('0') << std::hex
                          << std::setw(2) << (int)data[byteIdx + i] << " ";
            } else {
                std::cout << "   ";
            }
        }

        std::cout << "  ";

        // Prints the printable ASCII characters.
        for (int byteIdx = 0; byteIdx < kBytesRow && (byteIdx + i) < length; byteIdx++) {
            char c = data[byteIdx + i];
            c = (c >= 32 && c <= 127) ? c : '.';
            std::cout << c;
        }
    }
}

void BinaryPrinter::printHex(uint64_t val, int width) {
    std::cout << "0x" << std::noshowbase << std::internal << std::setfill('0') << std::hex
              << std::setw(width) << val;
}

void BinaryPrinter::printDec(uint32_t val, int width) {
    std::cout << std::noshowbase << std::internal << std::setfill(' ') << std::dec
              << std::setw(width) << val;
}

void BinaryPrinter::printDec(uint64_t val, int width) {
    std::cout << std::noshowbase << std::internal << std::setfill(' ') << std::dec
              << std::setw(width) << val;
}

}  // namespace elf
}  // namespace lib
