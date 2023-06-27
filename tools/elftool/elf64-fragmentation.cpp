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

    std::cout << std::endl << "Fragmentation results (unused bytes)" << std::endl;

    PrintSegmentStatsHeader();
    PrintSegmentStats(totalExecStats);
    PrintSegmentStats(totalReadOnlyStats);
    PrintSegmentStats(totalReadWriteStats);

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
        PrintSegmentStatsHeader();
    }

    for (int i = 0; i < elf64Binary.phdrs.size(); i++) {
        Elf64_Phdr* phdrPtr = elf64Binary.phdrs[i];
        if (phdrPtr->p_type == PT_LOAD) {
            SegmentStats segStats;
            PopulateSegmentStats(phdrPtr, segStats);
            PrintSegmentStats(segStats);
        }
    }
}

inline uint64_t calculateFrag(uint64_t memSize, uint64_t pageSize) {
    return (pageSize - (memSize % pageSize));
}

inline uint64_t calculateNumPages(uint64_t memSize, uint64_t pageSize) {
    uint64_t numPgs = memSize / pageSize;
    numPgs += memSize % pageSize == 0 ? 0 : 1;

    return numPgs;
}

void Elf64Fragmentation::PopulateSegmentStats(Elf64_Phdr* phdrPtr, SegmentStats& segStats) {
    segStats.p_flags = phdrPtr->p_flags;
    segStats.numSegments = 1;
    segStats.memSize = phdrPtr->p_memsz;
    segStats.num4kPages = calculateNumPages(phdrPtr->p_memsz, PS_4K);
    segStats.num16kPages = calculateNumPages(phdrPtr->p_memsz, PS_16K);
    segStats.num64kPages = calculateNumPages(phdrPtr->p_memsz, PS_64K);
    segStats.frag4kInBytes = calculateFrag(phdrPtr->p_memsz, PS_4K);
    segStats.frag16kInBytes = calculateFrag(phdrPtr->p_memsz, PS_16K);
    segStats.frag64kInBytes = calculateFrag(phdrPtr->p_memsz, PS_64K);

    // Update the global segment stats.
    if (Elf64Binary::IsExecSegment(phdrPtr->p_flags)) {
        UpdateTotalSegmentStats(totalExecStats, segStats);
    } else if (Elf64Binary::IsReadOnlySegment(phdrPtr->p_flags)) {
        UpdateTotalSegmentStats(totalReadOnlyStats, segStats);
    } else if (Elf64Binary::IsReadWriteSegment(phdrPtr->p_flags)) {
        UpdateTotalSegmentStats(totalReadWriteStats, segStats);
    }
}

void Elf64Fragmentation::UpdateTotalSegmentStats(SegmentStats& totalSegStats,
                                                 SegmentStats& segStats) {
    totalSegStats.numSegments += segStats.numSegments;
    totalSegStats.memSize += segStats.memSize;
    totalSegStats.num4kPages += segStats.num4kPages;
    totalSegStats.num16kPages += segStats.num16kPages;
    totalSegStats.num64kPages += segStats.num64kPages;
    totalSegStats.frag4kInBytes += segStats.frag4kInBytes;
    totalSegStats.frag16kInBytes += segStats.frag16kInBytes;
    totalSegStats.frag64kInBytes += segStats.frag64kInBytes;
}

void Elf64Fragmentation::PrintSegmentStatsHeader() {
    std::cout << "\t";
    std::cout << std::setw(10) << std::left << "Segment";
    std::cout << std::setw(10) << std::right << "Mem Size";
    std::cout << std::setw(12) << std::right << "# 4k pgs";
    std::cout << std::setw(12) << std::right << "# 16k pgs";
    std::cout << std::setw(12) << std::right << "# 64k pg";
    std::cout << std::setw(12) << std::right << "4k frag";
    std::cout << std::setw(12) << std::right << "16k frag";
    std::cout << std::setw(12) << std::right << "64k frag";
    std::cout << std::endl;
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

inline void printStat(uint64_t value) {
    std::cout << std::setw(12) << std::right;
    BinaryPrinter::printDec(value, 12);
}

inline void printMemSize(uint64_t memSize) {
    std::cout << std::setw(10) << std::right;
    BinaryPrinter::printDec(memSize, 10);
}

// Prints # pages needed for every the given segment and the fragmentation. The
// output format looks like:
//
//   Segment     Mem Size  # 4k pgs  # 16k pgs  # 64k pg  4k frag 16k frag  64k frag
//   Exec           67834        18          6         2     5894    30470     63238
//   Read Only      57904        16          4         4     7632     7632    204240
//   Read/Write      6400         2          2         2     1792     26368   124672
void Elf64Fragmentation::PrintSegmentStats(SegmentStats& segStats) {
    printSegmentType(segStats.p_flags);
    printMemSize(segStats.memSize);
    printStat(segStats.num4kPages);
    printStat(segStats.num16kPages);
    printStat(segStats.num64kPages);
    printStat(segStats.frag4kInBytes);
    printStat(segStats.frag16kInBytes);
    printStat(segStats.frag64kInBytes);
    std::cout << std::endl;
}

}  // namespace elf
}  // namespace lib
