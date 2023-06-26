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
    if (elf64Binary.phdrs.size() > 0) {
        std::cout << "\t";
        std::cout << std::setw(10) << std::left << "Segment";
        std::cout << std::setw(10) << std::right << "Mem Size";
        std::cout << std::setw(12) << std::right << "# 4k pgs";
        std::cout << std::setw(12) << std::right << "# 16k pgs";
        std::cout << std::setw(12) << std::right << "# 64k pgs";
        std::cout << std::endl;
    }

    for (int i = 0; i < elf64Binary.phdrs.size(); i++) {
        Elf64_Phdr* phdrPtr = elf64Binary.phdrs[i];
        if (phdrPtr->p_type == PT_LOAD) {
            totalFrag4k += (PS_4K - (phdrPtr->p_memsz % PS_4K));
            totalFrag16k += (PS_16K - (phdrPtr->p_memsz % PS_16K));
            totalFrag64k += (PS_64K - (phdrPtr->p_memsz % PS_64K));

            PrintNumPagesPerPhdr(phdrPtr);
        }
    }
}

inline void printSegmentType(uint64_t p_flags) {
    std::cout << "\t" << std::setw(10) << std::left;
    if (Elf64Binary::IsExecSegment(p_flags)) {
        std::cout << "Exec";
    } else if (Elf64Binary::IsReadOnlySegment(p_flags)) {
        std::cout << "Read Only";
    } else if (Elf64Binary::IsReadWriteSegment(p_flags)) {
        std::cout << "Read/Write";
    }
}

inline void printNumPages(uint64_t memSize, uint64_t pageSize) {
    uint64_t numPgs = memSize / pageSize;
    numPgs += memSize % pageSize == 0 ? 0 : 1;

    std::cout << std::setw(12) << std::right;
    BinaryPrinter::printDec(numPgs, 12);
}

// Prints # pages needed for the segments. The output format
// looks like:
//
//    Segment     Mem Size    # 4k pgs   # 16k pgs   # 64k pgs
//    Read Only      14768           4           1           1
//    Exec           33917           9           3           1
//    Read/Write      3200           1           1           1
void Elf64Fragmentation::PrintNumPagesPerPhdr(Elf64_Phdr* phdrPtr) {
    printSegmentType(phdrPtr->p_flags);

    std::cout << std::setw(10) << std::right;
    BinaryPrinter::printDec((uint64_t)phdrPtr->p_memsz, 10);

    printNumPages(phdrPtr->p_memsz, PS_4K);
    printNumPages(phdrPtr->p_memsz, PS_16K);
    printNumPages(phdrPtr->p_memsz, PS_64K);

    std::cout << std::endl;
}

}  // namespace elf
}  // namespace lib
