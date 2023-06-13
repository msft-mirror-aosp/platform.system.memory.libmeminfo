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

#include "elf64-binary.h"

#include <elf.h>
#include <stdint.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>

#include "binary-printer.h"
#include "elf64-ehdr-printer.h"
#include "elf64-phdr-printer.h"
#include "elf64-shdr-printer.h"

using namespace std;

namespace lib {
namespace elf {

Elf64Binary::Elf64Binary() {}

Elf64Binary::~Elf64Binary() {
    for (int i = 0; i < phdrs.size(); i++) {
        Elf64_Phdr* phdrPtr = phdrs[i];
        delete phdrPtr;
    }

    for (int i = 0; i < shdrs.size(); i++) {
        Elf64_Shdr* shdrPtr = shdrs[i];
        delete shdrPtr;
    }

    for (int i = 0; i < sections.size(); i++) {
        Elf64_Sc* section = sections[i];
        delete section->data;
        delete section;
    }

    for (int j = 0; j < sectionNames.size(); ++j) {
        delete sectionNames[j];
    }
}

// Prints the ELF64 Executable Header.
void Elf64Binary::PrintEhdr() {
    lib::elf::Elf64EhdrPrinter ehdrPrinter(&ehdr);
    ehdrPrinter.PrintEhdr();
}

// Prints the ELF64 Program Headers.
void Elf64Binary::PrintPhdrs() {
    lib::elf::Elf64PhdrPrinter phdrPrinter(&phdrs);
    phdrPrinter.PrintPhdrs();
}

// Prints the ELF Section Headers.
void Elf64Binary::PrintShdrs() {
    // Get a pointer to the Section header string table.
    Elf64_Sc* strTabSecPtr = sections[ehdr.e_shstrndx];
    lib::elf::Elf64ShdrPrinter shdrPrinter(strTabSecPtr, &shdrs);
    shdrPrinter.PrintShdrs();
}

// Print the ELF64 Section Names.
void Elf64Binary::PrintSectionNames() {
    std::cout << "Section Names" << std::endl;
    for (int i = 0; i < sectionNames.size(); i++) {
        std::cout << std::dec << i << " - " << *sectionNames[i] << std::endl;
    }
}

// Print all the parts of an ELF64 file.
void Elf64Binary::PrintAll() {
    PrintEhdr();
    PrintPhdrs();
    PrintShdrs();

    for (int i = 0; i < sections.size(); i++) {
        Elf64_Shdr* shdrPtr = shdrs[i];
        Elf64_Sc* sc = sections[i];

        std::cout << std::endl
                  << std::endl
                  << "------------------------------------------------------------------------"
                  << std::endl
                  << "     Section: " << sc->name << std::endl
                  << "     Length:  " << std::dec << sc->size << std::endl
                  << "-------------------------------------------------------------------------"
                  << std::endl;

        if (shdrPtr->sh_type != SHT_NOBITS) {
            BinaryPrinter::print(sc->data, sc->size, shdrPtr->sh_addr);
        } else {
            std::cout << "Empty section" << std::endl;
        }
    }

    std::cout << std::endl;
}

}  // namespace elf
}  // namespace lib
