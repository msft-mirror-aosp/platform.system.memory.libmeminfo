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

#include <elf.h>
#include <iostream>
#include <vector>

#include "elf64-binary.h"
#include "elf64-comparator.h"
#include "elf64-ehdr-printer.h"
#include "elf64-parser.h"
#include "elf64-phdr-printer.h"
#include "elf64-shdr-printer.h"

using namespace std;

// Program that parses ELF64 files, prints their parts and the differences.
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "usage: " << argv[0] << " <elf-file1> <elf-file2>" << std::endl;
        return -1;
    }

    std::string fileName1(argv[1]);
    std::string fileName2(argv[2]);

    lib::elf::Elf64Binary elf64Binary1;
    lib::elf::Elf64Parser::ParseElfFile(fileName1, elf64Binary1);

    lib::elf::Elf64Binary elf64Binary2;
    lib::elf::Elf64Parser::ParseElfFile(fileName2, elf64Binary2);

    elf64Binary2.PrintAll();

    Elf64_Ehdr* ehdrPtr1 = &elf64Binary1.ehdr;
    Elf64_Ehdr* ehdrPtr2 = &elf64Binary2.ehdr;
    if (lib::elf::Elf64Comparator::AreEhdrsEqual(ehdrPtr1, ehdrPtr2)) {
        cout << "-- Executable Headers are equal --" << std::endl;
    } else {
        cout << "-- Executable Headers are NOT equal --" << std::endl;
    }

    std::vector<Elf64_Phdr*>* phdrsPtr1 = &elf64Binary1.phdrs;
    std::vector<Elf64_Phdr*>* phdrsPtr2 = &elf64Binary2.phdrs;
    if (lib::elf::Elf64Comparator::ArePhdrsEqual(phdrsPtr1, phdrsPtr2)) {
        cout << "-- Program Headers are equal --" << std::endl;
    } else {
        cout << "-- Program Headers are NOT equal --" << std::endl;
    }

    std::vector<Elf64_Shdr*>* shdrsPtr1 = &elf64Binary1.shdrs;
    std::vector<Elf64_Shdr*>* shdrsPtr2 = &elf64Binary2.shdrs;
    if (lib::elf::Elf64Comparator::AreShdrsEqual(shdrsPtr1, shdrsPtr2)) {
        cout << "-- Section Headers are equal --" << std::endl;
    } else {
        cout << "-- Section Headers are NOT equal --" << std::endl;
    }

    if (lib::elf::Elf64Comparator::AreSdEqual(&elf64Binary1.sections, &elf64Binary2.sections)) {
        cout << "-- Sections are equal --" << std::endl;
    } else {
        cout << "-- Sections are NOT equal --" << std::endl;
    }

    return 0;
}
