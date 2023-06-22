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

#include "elf64-fragmentation.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "binary-printer.h"
#include "elf.h"
#include "elf64-binary.h"
#include "elf64-parser.h"
#include "utils.h"

#define PS_4K (4096UL)
#define PS_16K (16384UL)
#define PS_64K (65536UL)

namespace lib {
namespace elf {

Elf64Fragmentation::Elf64Fragmentation(std::string rootDir) {
    this->rootDir = rootDir;
}

void Elf64Fragmentation::CalculateFragmentation() {
    ProcessDir(rootDir);

    std::cout << std::endl
              << "Fragmentation results (unused bytes)" << std::endl
              << std::setw(10) << std::right << "Page Size"
              << " | " << std::right << "Total Fragmentation (bytes)" << std::endl;

    BinaryPrinter::printDec(PS_4K, 10);
    std::cout << " | ";
    lib::elf::BinaryPrinter::printDec(totalFrag4k, 14);
    std::cout << std::endl;

    BinaryPrinter::printDec(PS_16K, 10);
    std::cout << " | ";
    lib::elf::BinaryPrinter::printDec(totalFrag16k, 14);
    std::cout << std::endl;

    BinaryPrinter::printDec(PS_64K, 10);
    std::cout << " | ";
    lib::elf::BinaryPrinter::printDec(totalFrag64k, 14);
    std::cout << std::endl << std::endl;

    std::cout << "ELF 64 shared libraries processed: " << processedFiles << std::endl;
}

void Elf64Fragmentation::ProcessDir(std::string& dir) {
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        std::string path = entry.path();

        if (lib::elf::Utils::EndsWith(path, ".so") && !entry.is_symlink() &&
            entry.is_regular_file()) {
            lib::elf::Elf64Binary elf64Binary;
            lib::elf::Elf64Parser::ParseExecutableHeader(path, elf64Binary);

            if (elf64Binary.ehdr.e_ident[EI_CLASS] == ELFCLASS64) {
                std::cout << "Analyzing elf64: " << path << std::endl;
                lib::elf::Elf64Parser::ParseElfFile(path, elf64Binary);
                CalculateFragmentation(elf64Binary);
                processedFiles++;
            }
        } else if (entry.is_directory()) {
            ProcessDir(path);
        }
    }
}

void Elf64Fragmentation::CalculateFragmentation(lib::elf::Elf64Binary& elf64Binary) {
    for (int i = 0; i < elf64Binary.phdrs.size(); i++) {
        Elf64_Phdr* phdrPtr = elf64Binary.phdrs[i];
        if (phdrPtr->p_type == PT_LOAD) {
            totalFrag4k += (PS_4K - (phdrPtr->p_memsz % PS_4K));
            totalFrag16k += (PS_16K - (phdrPtr->p_memsz % PS_16K));
            totalFrag64k += (PS_64K - (phdrPtr->p_memsz % PS_64K));
        }
    }
}

}  // namespace elf
}  // namespace lib
