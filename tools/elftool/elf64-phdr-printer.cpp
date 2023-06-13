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

#include "elf64-phdr-printer.h"

#include <elf.h>
#include <stdint.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <vector>

#include "binary-printer.h"

using namespace std;

namespace lib {
namespace elf {

Elf64PhdrPrinter::Elf64PhdrPrinter(std::vector<Elf64_Phdr*>* phdrsPtr) {
    this->phdrsPtr = phdrsPtr;
}

Elf64PhdrPrinter::~Elf64PhdrPrinter() {}

// Print the program headers.
void Elf64PhdrPrinter::PrintPhdrs() {
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "        ELF64 Program Headers" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "There are " << phdrsPtr->size() << " program headers " << std::endl << std::endl;

    std::cout << "Program Headers:" << std::endl
              << std::setw(15) << std::left << "Type" << std::setw(12) << std::right << "Offset"
              << std::setw(14) << std::right << "VirtAddr" << std::setw(14) << std::right
              << "PhysAddr" << std::setw(14) << std::right << "FileSize" << std::setw(14)
              << std::right << "MemSize" << std::setw(7) << std::right << "Flags" << std::setw(9)
              << std::right << "Align" << std::endl;

    for (int i = 0; i < phdrsPtr->size(); i++) {
        PrintPhdr(phdrsPtr->at(i));
    }
}

void Elf64PhdrPrinter::PrintPhdr(Elf64_Phdr* phdrPtr) {
    PrintPhdrType(phdrPtr->p_type);
    BinaryPrinter::printHex(phdrPtr->p_offset, 10);
    std::cout << "  ";
    BinaryPrinter::printHex(phdrPtr->p_vaddr, 10);
    std::cout << "  ";
    BinaryPrinter::printHex(phdrPtr->p_paddr, 10);
    std::cout << "  ";
    BinaryPrinter::printHex(phdrPtr->p_filesz, 10);
    std::cout << "  ";
    BinaryPrinter::printHex(phdrPtr->p_memsz, 10);
    ;
    std::cout << "  ";
    PrintPhdrFlags(phdrPtr->p_flags);
    std::cout << "   ";
    BinaryPrinter::printDec((uint32_t)phdrPtr->p_align, 8);
    std::cout << std::endl;
}

void Elf64PhdrPrinter::PrintPhdrType(uint32_t pType) {
    std::cout << std::left << std::setfill(' ') << std::setw(15);

    switch (pType) {
        case PT_NULL:
            std::cout << "NULL";
            break;
        case PT_LOAD:
            std::cout << "LOAD";
            break;
        case PT_DYNAMIC:
            std::cout << "DYNAMIC";
            break;
        case PT_INTERP:
            std::cout << "INTERP";
            break;
        case PT_NOTE:
            std::cout << "NOTE";
            break;
        case PT_SHLIB:
            std::cout << "SHLIB";
            break;
        case PT_PHDR:
            std::cout << "PHDR";
            break;
        case PT_TLS:
            std::cout << "TLS";
            break;
        case PT_LOOS:
            std::cout << "LOOS";
            break;
        case PT_GNU_EH_FRAME:
            std::cout << "GNU_EH_FRAME";
            break;
        case PT_GNU_STACK:
            std::cout << "GNU_STACK";
            break;
        case PT_GNU_RELRO:
            std::cout << "GNU_RELRO";
            break;
        case PT_GNU_PROPERTY:
            std::cout << "GNU_PROPERTY";
            break;
        case PT_LOSUNW:  // case PT_HIOS:
            std::cout << "LOSUNW";
            break;
        case PT_SUNWSTACK:
            std::cout << "SUNWSTACK";
            break;
        case PT_HISUNW:  // case PT_HIOS:
            std::cout << "HISUNW";
            break;
        case PT_LOPROC:
            std::cout << "LOPROC";
            break;
        case PT_HIPROC:
            std::cout << "HIPROC";
            break;
        default:
            std::cerr << "Unknown Program Header type [" << pType << "]" << std::endl;
            exit(-1);
    }
}

void Elf64PhdrPrinter::PrintPhdrFlags(uint32_t flags) {
    std::cout << (PF_R & flags ? "R" : " ") << (PF_W & flags ? "W" : " ")
              << (PF_X & flags ? "E" : " ");
}

}  // namespace elf
}  // namespace lib
