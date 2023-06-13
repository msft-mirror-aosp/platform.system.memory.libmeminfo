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

#include "elf64-shdr-printer.h"

#include <elf.h>
#include <stdint.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "binary-printer.h"
#include "elf64-binary.h"

using namespace std;

namespace lib {
namespace elf {

Elf64ShdrPrinter::Elf64ShdrPrinter(Elf64_Sc* strTabSecPtr, std::vector<Elf64_Shdr*>* shdrsPtr) {
    this->strTabSecPtr = strTabSecPtr;
    this->shdrsPtr = shdrsPtr;
}

Elf64ShdrPrinter::~Elf64ShdrPrinter() {}

// Print the ELF64 section headers.
void Elf64ShdrPrinter::PrintShdrs() {
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "        ELF64 Section Headers" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "There are " << std::dec << shdrsPtr->size() << " section headers " << std::endl
              << std::endl;

    std::cout << "Section Headers:" << std::endl
              << std::setw(7) << std::left << " [Nr]" << std::setw(20) << std::left << "Name"
              << std::setw(18) << std::left << "Type" << std::setw(12) << std::right << "Address"
              << std::setw(14) << std::right << "Offset" << std::setw(14) << std::right << "Size"
              << std::setw(14) << std::right << "EntSize" << std::setw(7) << std::left << "  Flags"
              << std::setw(8) << std::right << "Link" << std::setw(6) << std::right << "Info"
              << std::setw(10) << std::right << "Align" << std::endl;

    for (int i = 0; i < shdrsPtr->size(); i++) {
        std::cout << " [" << std::dec << std::setw(2) << i << "]  ";
        PrintShdr(shdrsPtr->at(i));
    }

    cout << "\nKey to Flags:" << std::endl
         << "W (write), A (alloc), X (execute), M (merge), S (strings)" << std::endl
         << "I (info), L (link order), G (group), T (TLS), E (exclude)" << std::endl
         << "o (OS specific)" << std::endl;
}

void Elf64ShdrPrinter::PrintShdr(Elf64_Shdr* shdrPtr) {
    uint32_t nameIdx = shdrPtr->sh_name;
    std::cout << std::left << std::setw(20) << (char*)&strTabSecPtr->data[nameIdx];
    PrintShdrType(shdrPtr->sh_type);
    BinaryPrinter::printHex(shdrPtr->sh_addr, 10);
    std::cout << "  ";
    BinaryPrinter::printHex(shdrPtr->sh_offset, 10);
    std::cout << "  ";
    BinaryPrinter::printHex(shdrPtr->sh_size, 10);
    std::cout << "  ";
    BinaryPrinter::printHex(shdrPtr->sh_entsize, 10);
    std::cout << "  ";
    PrintShdrFlags(shdrPtr->sh_flags);
    std::cout << "  ";
    BinaryPrinter::printDec(shdrPtr->sh_link, 4);
    std::cout << "  ";
    BinaryPrinter::printDec(shdrPtr->sh_info, 4);
    std::cout << "  ";
    BinaryPrinter::printDec((uint64_t)shdrPtr->sh_addralign, 8);
    std::cout << std::endl;
}

void Elf64ShdrPrinter::PrintShdrType(uint32_t sType) {
    std::cout << std::left << std::setfill(' ') << std::setw(18);

    switch (sType) {
        case SHT_NULL:
            std::cout << "NULL";
            break;
        case SHT_PROGBITS:
            std::cout << "PROGBITS";
            break;
        case SHT_SYMTAB:
            std::cout << "SYMTAB";
            break;
        case SHT_STRTAB:
            std::cout << "STRTAB";
            break;
        case SHT_RELA:
            std::cout << "RELA";
            break;
        case SHT_HASH:
            std::cout << "HASH";
            break;
        case SHT_DYNAMIC:
            std::cout << "DYNAMIC";
            break;
        case SHT_NOTE:
            std::cout << "NOTE";
            break;
        case SHT_NOBITS:
            std::cout << "NOBITS";
            break;
        case SHT_REL:
            std::cout << "REL";
            break;
        case SHT_SHLIB:
            std::cout << "SHLIB";
            break;
        case SHT_DYNSYM:
            std::cout << "DYNSYM";
            break;
        case SHT_INIT_ARRAY:
            std::cout << "SHT_INIT_ARRAY";
            break;
        case SHT_FINI_ARRAY:
            std::cout << "SHT_FINI_ARRAY";
            break;
        case SHT_PREINIT_ARRAY:
            std::cout << "SHT_PREINIT_ARRAY";
            break;
        case SHT_GROUP:
            std::cout << "SHT_GROUP";
            break;
        case SHT_SYMTAB_SHNDX:
            std::cout << "SHT_SYMTAB_SHNDX";
            break;
        case SHT_NUM:
            std::cout << "SHT_NUM";
            break;
        case SHT_LOOS:
            std::cout << "SHT_LOOS";
            break;
        case SHT_GNU_ATTRIBUTES:
            std::cout << "SHT_GNU_ATTRIBUTES";
            break;
        case SHT_GNU_HASH:
            std::cout << "SHT_GNU_HASH";
            break;
        case SHT_GNU_LIBLIST:
            std::cout << "SHT_GNU_LIBLIST";
            break;
        case SHT_LOSUNW:
            std::cout << "SHT_LOSUNW";
            break;
        case SHT_SUNW_COMDAT:
            std::cout << "SHT_SUNW_COMDAT";
            break;
        case SHT_SUNW_syminfo:
            std::cout << "SHT_SUNW_syminfo";
            break;
        case SHT_GNU_verdef:
            std::cout << "SHT_GNU_verdef";
            break;
        case SHT_GNU_verneed:
            std::cout << "SHT_GNU_verneed";
            break;
        case SHT_GNU_versym:
            std::cout << "SHT_GNU_versym";
            break;
        case SHT_LOPROC:
            std::cout << "LOPROC";
            break;
        case SHT_HIPROC:
            std::cout << "HIPROC";
            break;
        case SHT_LOUSER:
            std::cout << "LOUSER";
            break;
        case SHT_HIUSER:
            std::cout << "HIUSER";
            break;
        default:
            std::cout << "Unknown Section Header type [" << std::hex << sType << "]" << std::endl;
            break;
    }
}

void Elf64ShdrPrinter::PrintShdrFlags(uint32_t flags) {
    string allFlags = (SHF_WRITE & flags ? "W" : " ");
    allFlags += (SHF_ALLOC & flags ? "A" : " ");
    allFlags += (SHF_EXECINSTR & flags ? "X" : " ");
    allFlags += (SHF_MERGE & flags ? "M" : "");
    allFlags += (SHF_STRINGS & flags ? "S" : "");
    allFlags += (SHF_INFO_LINK & flags ? "I" : "");
    allFlags += (SHF_LINK_ORDER & flags ? "L" : "");
    allFlags += (SHF_OS_NONCONFORMING & flags ? "o" : "");
    allFlags += (SHF_GROUP & flags ? "G" : "");
    allFlags += (SHF_TLS & flags ? "T" : "");
    allFlags += (SHF_COMPRESSED & flags ? "C" : "");

    std::cout << std::setfill(' ') << std::left << std::setw(7) << allFlags;
}

}  // namespace elf
}  // namespace lib
