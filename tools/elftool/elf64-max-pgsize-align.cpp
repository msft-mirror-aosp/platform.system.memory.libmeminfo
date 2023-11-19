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

#include <iostream>
#include <vector>

#include "elf.h"
#include "elf64-binary.h"
#include "elf64-comparator.h"
#include "elf64-ehdr-printer.h"
#include "elf64-parser.h"
#include "elf64-phdr-printer.h"
#include "elf64-shdr-printer.h"
#include "elf64-writer.h"

void AlignProgramHeaders(std::vector<Elf64_Phdr*>* phdrsPtr) {
    std::cout << "Number of Program Headers" << phdrsPtr->size() << std::endl;

    for (int i = 0; i < phdrsPtr->size(); i++) {
        Elf64_Phdr* phdrPtr = phdrsPtr->at(i);

        if (phdrPtr->p_type == PT_LOAD) {
            std::cout << "PT_LOAD Segment: " << i << std::endl
                      << "\t p_memsz:  " << phdrPtr->p_memsz << std::endl
                      << "\t p_filesz: " << phdrPtr->p_filesz << std::endl
                      << "\t p_align:  " << phdrPtr->p_align << std::endl;
        }

        phdrPtr->p_filesz = (phdrPtr->p_filesz + phdrPtr->p_align - 1) & ~(phdrPtr->p_align - 1);
        phdrPtr->p_memsz = (phdrPtr->p_memsz + phdrPtr->p_align - 1) & ~(phdrPtr->p_align - 1);
    }
}

// Align Program headers to max-page-size
//
// Problem: When the page size is 4k and shared libraries and binaries are
//          16k/64k elf aligned with the flag -Wl,-z,max-page-size=[16384|65536],
//          the dynamic linker (loader) does not unmap the hole between segments an
//          a extra vm_area_struct is created.
//
//          This happens because the loader allocates a memory area big enough to
//          map the shared library and then maps and mprotect every segment at
//          page size boundaries instead of a p_align boundary.
//
// How to reproduce it: In a 4k page size kernel, compile a shared library
// 4k and 16k elf alignment. Use the shared library in a binary, and then observe
// the /proc/<pid>/maps.
//
// When a shared library is linked using the flag -Wl,-z,max-page-size=4096 and loaded
// by the dynamic linker, we can see that there is NOT an extra vm_area_struct with
// permissions ---p.
//
//  $ cat /proc/1532516/maps
//  ...
// 7f9176f29000-7f9176f2a000 r--p 00000000 08:05 2372953  /shared-libs/build/libshared_4k.so
// 7f9176f2a000-7f9176f2b000 r-xp 00001000 08:05 2372953  /shared-libs/build/libshared_4k.so
// 7f9176f2b000-7f9176f2c000 r--p 00002000 08:05 2372953  /shared-libs/build/libshared_4k.so
// 7f9176f2c000-7f9176f2d000 r--p 00002000 08:05 2372953  /shared-libs/build/libshared_4k.so
// 7f9176f2d000-7f9176f2e000 rw-p 00003000 08:05 2372953  /shared-libs/build/libshared_4k.so
//
// When a shared library is linked using the flag -Wl,-z,max-page-size=65536 and loaded
// by the dynamic linker, we can see that an extra vm_area_struct is used with the
// permissions ---p.
//
//  $ cat /proc/1581453/maps
// ...
// 7fafe0c92000-7fafe0c93000 r--p 00000000 08:05 2372957  /shared-libs/build/libshared_64k.so
// 7fafe0c93000-7fafe0ca2000 ---p 00001000 08:05 2372957  /shared-libs/build/libshared_64k.so
// 7fafe0ca2000-7fafe0ca3000 r-xp 00010000 08:05 2372957  /shared-libs/build/libshared_64k.so
// 7fafe0ca3000-7fafe0cb2000 ---p 00011000 08:05 2372957  /shared-libs/build/libshared_64k.so
// 7fafe0cb2000-7fafe0cb3000 r--p 00020000 08:05 2372957  /shared-libs/build/libshared_64k.so
// 7fafe0cb3000-7fafe0cc2000 ---p 00021000 08:05 2372957  /shared-libs/build/libshared_64k.so
// 7fafe0cc2000-7fafe0cc3000 r--p 00020000 08:05 2372957  /shared-libs/build/libshared_64k.so
// 7fafe0cc3000-7fafe0cc4000 rw-p 00021000 08:05 2372957  /shared-libs/build/libshared_64k.so
//
//
// Solution 1 - Modify the dynamic linker: When the dynamic linker loads the
//              shared libraries, extend the vm_area_struct to be at a p_align
//              boundary.
//
// Solution 2 - Modify the dynamic linker: When the dynamic linker loads the
//              shared libraries, unmap the extra vm_area_struct that maps the hole.
//
// Solution 3 - Modify the static linker: When the static linker creates the
//              program segments during compilation time, make sure that the p_filesz
//              and p_memsz are extended to a p_align boundary. This can be achieved by
//              adding a new elf64 section to each PT_LOAD segment to fill the hole.
//
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "usage: " << argv[0] << " <elf-file-to-align> <new-file-aligned>"
                  << std::endl;
        return -1;
    }

    std::string fileName(argv[1]);
    std::string newAlignedFileName(argv[2]);

    lib::elf::Elf64Binary elf64Binary;
    lib::elf::Elf64Parser::ParseElfFile(fileName, elf64Binary);

    std::vector<Elf64_Phdr*>* phdrsPtr = &elf64Binary.phdrs;
    AlignProgramHeaders(phdrsPtr);

    lib::elf::Elf64Writer::WriteElfFile(elf64Binary, newAlignedFileName);

    return 0;
}

