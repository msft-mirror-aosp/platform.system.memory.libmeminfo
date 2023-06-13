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

#include "elf64-ehdr-printer.h"

#include <elf.h>
#include <stdint.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>

using namespace std;

#define DESC_WIDTH 40

namespace lib {
namespace elf {

Elf64EhdrPrinter::Elf64EhdrPrinter(Elf64_Ehdr* ehdrPtr) {
    this->ehdrPtr = ehdrPtr;
}

Elf64EhdrPrinter::~Elf64EhdrPrinter() {}

// Print the ELF64 Executable Header.
void Elf64EhdrPrinter::PrintEhdr() {
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "        ELF64 Executable Header" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    PrintElfIdent();

    std::cout << std::left << setw(DESC_WIDTH) << "File version:" << std::dec << ehdrPtr->e_version
              << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Entry Point VA:" << std::hex << "0x"
              << ehdrPtr->e_entry << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Program Header table offset:" << std::dec
              << ehdrPtr->e_phoff << " (bytes into file)" << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Section Header table offset:" << std::dec
              << ehdrPtr->e_shoff << " (bytes into file)" << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Processor-specific flags:" << std::hex << "0x"
              << ehdrPtr->e_flags << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "ELF header size:" << std::dec
              << ehdrPtr->e_ehsize << " bytes" << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Program header table entry size:" << std::dec
              << ehdrPtr->e_phentsize << " bytes" << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Program header table entry count:" << std::dec
              << ehdrPtr->e_phnum << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Section header table entry size:" << std::dec
              << ehdrPtr->e_shentsize << " bytes" << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Section header table entry count:" << std::dec
              << ehdrPtr->e_shnum << std::endl;
    std::cout << std::left << setw(DESC_WIDTH) << "Section header string table index:" << std::dec
              << ehdrPtr->e_shstrndx << std::endl;
}

void Elf64EhdrPrinter::PrintElfIdent() {
    std::cout << "Magic number: ";
    for (int i = 0; i < EI_NIDENT; i++) {
        std::cout << std::hex << (int)ehdrPtr->e_ident[i] << " ";
    }

    // Print the first 4 bytes of the magic number.
    std::cout << std::endl << "              " << std::hex << (int)ehdrPtr->e_ident[0];
    std::cout << "  " << ehdrPtr->e_ident[1] << "  " << ehdrPtr->e_ident[2] << "  "
              << ehdrPtr->e_ident[3] << std::endl;

    PrintElfClass();
    PrintElfData();
    PrintElfVersion();
    PrintElfOsAbi();
    PrintElfFileType();
    PrintMachineArch();
}

void Elf64EhdrPrinter::PrintElfClass() {
    char elfClas = ehdrPtr->e_ident[EI_CLASS];
    std::cout << std::left << setw(DESC_WIDTH) << "Class:";

    switch (elfClas) {
        case ELFCLASSNONE:
            // This is an invalid ELF class.
            std::cout << "NONE CLASS" << std::endl;
            break;
        case ELFCLASS32:
            std::cout << "ELF 32" << std::endl;
            break;
        case ELFCLASS64:
            std::cout << "ELF 64" << std::endl;
            break;
    }
}

void Elf64EhdrPrinter::PrintElfData() {
    char elfData = ehdrPtr->e_ident[EI_DATA];

    std::cout << std::left << setw(DESC_WIDTH) << "Data:";

    switch (elfData) {
        case ELFDATANONE:
            std::cout << "None" << std::endl;
            break;
        case ELFDATA2LSB:
            std::cout << "2's complement, little endian" << std::endl;
            break;
        case ELFDATA2MSB:
            std::cout << "2's complement, big endian" << std::endl;
            break;
    }
}

void Elf64EhdrPrinter::PrintElfVersion() {
    char elfVersion = ehdrPtr->e_ident[EI_VERSION];

    std::cout << std::left << setw(DESC_WIDTH) << "Version:";

    switch (elfVersion) {
        case EV_NONE:
            std::cout << "None" << std::endl;
            break;
        case EV_CURRENT:
            std::cout << "Current" << std::endl;
            break;
        default:
            std::cerr << "Invalid ELF64 version [" << (int)elfVersion << "]" << std::endl;
            exit(-1);
    }
}

void Elf64EhdrPrinter::PrintElfOsAbi() {
    char elfOsAbi = ehdrPtr->e_ident[EI_OSABI];

    std::cout << std::left << setw(DESC_WIDTH) << "OS/ABI:";

    switch (elfOsAbi) {
        case ELFOSABI_NONE:
            std::cout << "Unix - System V" << std::endl;
            break;
        case ELFOSABI_LINUX:
            std::cout << "Linux" << std::endl;
            break;
        default:
            // TODO: Include all the EM_* OS available in elf.h.
            std::cout << "Other OS" << std::endl;
            break;
    }
}

void Elf64EhdrPrinter::PrintElfFileType() {
    uint16_t elfFileType = ehdrPtr->e_type;

    std::cout << std::left << setw(DESC_WIDTH) << "ELF file type:";

    switch (elfFileType) {
        case ET_NONE:
            std::cout << "None" << std::endl;
            break;
        case ET_REL:
            std::cout << "Relocatable" << std::endl;
            break;
        case ET_EXEC:
            std::cout << "Executable" << std::endl;
            break;
        case ET_DYN:
            std::cout << "DYN (Shared object file)" << std::endl;
            break;
        case ET_CORE:
            std::cout << "Core" << std::endl;
            break;
        default:
            std::cerr << "Unknown Executable file type [" << elfFileType << "]" << std::endl;
            exit(-1);
    }
}

void Elf64EhdrPrinter::PrintMachineArch() {
    uint16_t machine = ehdrPtr->e_machine;

    std::cout << std::left << setw(DESC_WIDTH) << "Machine:";

    switch (machine) {
        case EM_X86_64:
            std::cout << "AMD x86-64 " << std::endl;
            break;
        case EM_AARCH64:
            std::cout << "ARM Arch64" << std::endl;
            break;
        default:
            std::cout << "Other" << std::endl;
            break;
    }
}

}  // namespace elf
}  // namespace lib
