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
#include <string>

#include "elf64-binary.h"

namespace lib {
namespace elf {

// Struct to store the stats for the num pages and fragmentation.
typedef struct {
    uint64_t p_flags;
    uint64_t numSegments;
    uint64_t memSize;
    uint64_t num4kPages;
    uint64_t num16kPages;
    uint64_t num64kPages;
    uint64_t frag4kInBytes;
    uint64_t frag16kInBytes;
    uint64_t frag64kInBytes;
} SegmentStats;

// Class to calculate the fragmentation in ELF 64 shared libraries.
//
// The ELF64 program header contains the p_type and p_memsz fields.
//
//  - p_type: indicates the type of segment. The type of segments that will be
//    used to calculate the fragmentation are PT_LOAD. These segments will be loaded
//    in memory during runtime. The memory blocks used by the segments have to
//    be page size multiples.
//  - p_memsz: indicates the memory size required by the segment.
//
// The memory fragmentation in ELF64 file is calculated by the formula:
//
//     fragmentation = page_size - (p_memsz % page_size)
//
// where fragmentation is equal to the memory not used in the page.
//
// Note: The program headers are memory mapped in PAGE SIZE blocks.
class Elf64Fragmentation {
  public:
    Elf64Fragmentation(std::string rootDir);
    // Calculates the fragmentation in ELF 64 shared libraries in the given directory
    // and subdirectories.
    void CalculateFragmentation();

  private:
    void CalculateFragmentation(Elf64Binary& elf64Binary);
    void PrintSegmentStats(SegmentStats& segStats);
    void PrintSegmentStatsHeader();
    void PopulateSegmentStats(Elf64_Phdr* phdr, SegmentStats& segStats);
    void UpdateTotalSegmentStats(SegmentStats& totalSegStats, SegmentStats& segStats);
    void ProcessDir(std::string& dir);

  private:
    std::string rootDir;
    int processedFiles = 0;
    SegmentStats totalExecStats = {PF_X, 0, 0, 0, 0, 0, 0, 0, 0};
    SegmentStats totalReadOnlyStats = {PF_R, 0, 0, 0, 0, 0, 0, 0, 0};
    SegmentStats totalReadWriteStats = {PF_R | PF_W, 0, 0, 0, 0, 0, 0, 0, 0};
};

}  // namespace elf
}  // namespace lib
