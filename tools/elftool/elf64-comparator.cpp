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

#include "elf64-comparator.h"

#include <elf.h>
#include <cstring>
#include <iostream>
#include <vector>

#include "elf64-binary.h"

namespace lib {
namespace elf {

// Compares the ELF64 Executable Headers.
//
// @return true if equals, otherwise false.
bool Elf64Comparator::AreEhdrsEqual(Elf64_Ehdr* ehdrPtr1, Elf64_Ehdr* ehdrPtr2) {
    std::cout << "------------------------------------" << std::endl;
    std::cout << "  Comparing ELF64 Executable Headers" << std::endl;
    std::cout << "------------------------------------" << std::endl;

    bool equal = true;

    // Comparing magic number and other info.
    for (int i = 0; i < EI_NIDENT; i++) {
        if (ehdrPtr1->e_ident[i] != ehdrPtr2->e_ident[i]) {
            std::cout << "e_ident[" << std::dec << i << "] is different" << std::endl;
            equal = false;
        }
    }

    if (ehdrPtr1->e_type != ehdrPtr2->e_type) {
        std::cout << "e_type are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_machine != ehdrPtr2->e_machine) {
        std::cout << "e_machine are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_version != ehdrPtr2->e_version) {
        std::cout << "e_version are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_entry != ehdrPtr2->e_entry) {
        std::cout << "e_entry are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_phoff != ehdrPtr2->e_phoff) {
        std::cout << "e_phoff are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_shoff != ehdrPtr2->e_shoff) {
        std::cout << "e_shoff are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_flags != ehdrPtr2->e_flags) {
        std::cout << "e_flags are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_ehsize != ehdrPtr2->e_ehsize) {
        std::cout << "e_ehsize are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_phentsize != ehdrPtr2->e_phentsize) {
        std::cout << "e_phentsize are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_phnum != ehdrPtr2->e_phnum) {
        std::cout << "e_phnum are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_shentsize != ehdrPtr2->e_shentsize) {
        std::cout << "e_shentsize are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_shnum != ehdrPtr2->e_shnum) {
        std::cout << "e_shnum are different" << std::endl;
        equal = false;
    }

    if (ehdrPtr1->e_shstrndx != ehdrPtr2->e_shstrndx) {
        std::cout << "e_shstrndx are different" << std::endl;
        equal = false;
    }

    return equal;
}

// Compares the ELF64 Program (Segment) Headers.
//
// @return true if equals, otherwise false.
bool Elf64Comparator::ArePhdrsEqual(std::vector<Elf64_Phdr*>* phdrsPtr1,
                                    std::vector<Elf64_Phdr*>* phdrsPtr2) {
    std::cout << "------------------------------------" << std::endl;
    std::cout << "   Comparing ELF64 Program Headers" << std::endl;
    std::cout << "------------------------------------" << std::endl;

    bool equal = true;

    if (phdrsPtr1->size() != phdrsPtr2->size()) {
        std::cout << "Different number of Program Headers" << std::endl;
        return false;
    }

    for (int i = 0; i < phdrsPtr1->size(); i++) {
        Elf64_Phdr* phdrPtr1 = phdrsPtr1->at(i);
        Elf64_Phdr* phdrPtr2 = phdrsPtr2->at(i);

        if (phdrPtr1->p_type != phdrPtr2->p_type) {
            std::cout << "phdr1[" << std::dec << i << "].ptype = 0x" << std::hex << phdrPtr1->p_type
                      << std::endl
                      << "phdr2[" << std::dec << i << "].ptype = 0x" << std::hex << phdrPtr2->p_type
                      << std::endl;
            equal = false;
        }

        if (phdrPtr1->p_flags != phdrPtr2->p_flags) {
            std::cout << "phdr1[" << std::dec << i << "].p_flags = 0x" << std::hex
                      << phdrPtr1->p_flags << std::endl
                      << "phdr2[" << std::dec << i << "].p_flags = 0x" << std::hex
                      << phdrPtr2->p_flags << std::endl;
            equal = false;
        }

        if (phdrPtr1->p_offset != phdrPtr2->p_offset) {
            std::cout << "phdr1[" << std::dec << i << "].p_offset = 0x" << std::hex
                      << phdrPtr1->p_offset << std::endl
                      << "phdr2[" << std::dec << i << "].p_offset = 0x" << std::hex
                      << phdrPtr2->p_offset << std::endl;
            equal = false;
        }

        if (phdrPtr1->p_vaddr != phdrPtr2->p_vaddr) {
            std::cout << "phdr1[" << std::dec << i << "].p_vaddr = 0x" << std::hex
                      << phdrPtr1->p_vaddr << std::endl
                      << "phdr2[" << std::dec << i << "].p_vaddr = 0x" << std::hex
                      << phdrPtr2->p_vaddr << std::endl;
            equal = false;
        }

        if (phdrPtr1->p_paddr != phdrPtr2->p_paddr) {
            std::cout << "phdr1[" << std::dec << i << "].p_paddr = 0x" << std::hex
                      << phdrPtr1->p_paddr << std::endl
                      << "phdr2[" << std::dec << i << "].p_paddr = 0x" << std::hex
                      << phdrPtr2->p_paddr << std::endl;
            equal = false;
        }

        if (phdrPtr1->p_filesz != phdrPtr2->p_filesz) {
            std::cout << "phdr1[" << std::dec << i << "].p_filesz = 0x" << std::hex
                      << phdrPtr1->p_filesz << std::endl
                      << "phdr2[" << std::dec << i << "].p_filesz = 0x" << std::hex
                      << phdrPtr2->p_filesz << std::endl;
            equal = false;
        }

        if (phdrPtr1->p_memsz != phdrPtr2->p_memsz) {
            std::cout << "phdr1[" << std::dec << i << "].p_memsz = 0x" << std::hex
                      << phdrPtr1->p_memsz << std::endl
                      << "phdr2[" << std::dec << i << "].p_memsz = 0x" << std::hex
                      << phdrPtr2->p_memsz << std::endl;
            equal = false;
        }

        if (phdrPtr1->p_align != phdrPtr2->p_align) {
            std::cout << "phdr1[" << std::dec << i << "].p_align = 0x" << std::hex
                      << phdrPtr1->p_align << std::endl
                      << "phdr2[" << std::dec << i << "].p_align = 0x" << std::hex
                      << phdrPtr2->p_align << std::endl;
            equal = false;
        }
    }

    return equal;
}

// Compares the ELF64 Section Headers.
//
// @return true if equals, otherwise false.
bool Elf64Comparator::AreShdrsEqual(std::vector<Elf64_Shdr*>* shdrsPtr1,
                                    std::vector<Elf64_Shdr*>* shdrsPtr2) {
    std::cout << "------------------------------------" << std::endl;
    std::cout << "    Comparing ELF64 Section Headers" << std::endl;
    std::cout << "------------------------------------" << std::endl;

    bool equal = true;

    if (shdrsPtr1->size() != shdrsPtr2->size()) {
        std::cout << "Different number of Section Headers" << std::endl;
        return false;
    }

    for (int i = 0; i < shdrsPtr1->size(); i++) {
        Elf64_Shdr* shdrPtr1 = shdrsPtr1->at(i);
        Elf64_Shdr* shdrPtr2 = shdrsPtr2->at(i);

        if (shdrPtr1->sh_name != shdrPtr2->sh_name) {
            std::cout << "shdr1[" << std::dec << i << "].sh_name = 0x" << std::hex
                      << shdrPtr1->sh_name << std::endl
                      << "shdr2[" << std::dec << i << "].sh_name = 0x" << std::hex
                      << shdrPtr2->sh_name << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_type != shdrPtr2->sh_type) {
            std::cout << "shdr1[" << std::dec << i << "].sh_type = 0x" << std::hex
                      << shdrPtr1->sh_type << std::endl
                      << "shdr2[" << std::dec << i << "].sh_type = 0x" << std::hex
                      << shdrPtr2->sh_type << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_flags != shdrPtr2->sh_flags) {
            std::cout << "shdr1[" << std::dec << i << "].sh_flags = 0x" << std::hex
                      << shdrPtr1->sh_flags << std::endl
                      << "shdr2[" << std::dec << i << "].sh_flags = 0x" << std::hex
                      << shdrPtr2->sh_flags << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_addr != shdrPtr2->sh_addr) {
            std::cout << "shdr1[" << std::dec << i << "].sh_addr = 0x" << std::hex
                      << shdrPtr1->sh_addr << std::endl
                      << "shdr2[" << std::dec << i << "].sh_addr = 0x" << std::hex
                      << shdrPtr2->sh_addr << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_offset != shdrPtr2->sh_offset) {
            std::cout << "shdr1[" << std::dec << i << "].sh_offset = 0x" << std::hex
                      << shdrPtr1->sh_offset << std::endl
                      << "shdr2[" << std::dec << i << "].sh_offset = 0x" << std::hex
                      << shdrPtr2->sh_offset << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_size != shdrPtr2->sh_size) {
            std::cout << "shdr1[" << std::dec << i << "].sh_size = 0x" << std::hex
                      << shdrPtr1->sh_size << std::endl
                      << "shdr2[" << std::dec << i << "].sh_size = 0x" << std::hex
                      << shdrPtr2->sh_size << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_link != shdrPtr2->sh_link) {
            std::cout << "shdr1[" << std::dec << i << "].sh_link = 0x" << std::hex
                      << shdrPtr1->sh_link << std::endl
                      << "shdr2[" << std::dec << i << "].sh_link = 0x" << std::hex
                      << shdrPtr2->sh_link << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_info != shdrPtr2->sh_info) {
            std::cout << "shdr1[" << std::dec << i << "].sh_info = 0x" << std::hex
                      << shdrPtr1->sh_info << std::endl
                      << "shdr2[" << std::dec << i << "].sh_info = 0x" << std::hex
                      << shdrPtr2->sh_info << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_addralign != shdrPtr2->sh_addralign) {
            std::cout << "shdr1[" << std::dec << i << "].sh_addralign = 0x" << std::hex
                      << shdrPtr1->sh_addralign << std::endl
                      << "shdr2[" << std::dec << i << "].sh_addralign = 0x" << std::hex
                      << shdrPtr2->sh_addralign << std::endl;
            equal = false;
        }

        if (shdrPtr1->sh_entsize != shdrPtr2->sh_entsize) {
            std::cout << "shdr1[" << std::dec << i << "].sh_entsize = 0x" << std::hex
                      << shdrPtr1->sh_entsize << std::endl
                      << "shdr2[" << std::dec << i << "].sh_entsize = 0x" << std::hex
                      << shdrPtr2->sh_entsize << std::endl;
            equal = false;
        }
    }

    return equal;
}

// Compares the ELF64 Section data.
//
// @return true if equals, otherwise false.
bool Elf64Comparator::AreSdEqual(std::vector<Elf64_Sc*>* sectionsPtr1,
                                 std::vector<Elf64_Sc*>* sectionsPtr2) {
    std::cout << "------------------------------------" << std::endl;
    std::cout << "  Comparing ELF64 Sections (content)" << std::endl;
    std::cout << "------------------------------------" << std::endl;

    bool equal = true;

    if (sectionsPtr1->size() != sectionsPtr2->size()) {
        std::cout << "Different number of Sections" << std::endl;
        return false;
    }

    for (int i = 0; i < sectionsPtr1->size(); i++) {
        Elf64_Sc* sectionPtr1 = sectionsPtr1->at(i);
        Elf64_Sc* sectionPtr2 = sectionsPtr2->at(i);

        if (sectionPtr1->size != sectionPtr2->size) {
            std::cout << "section1[" << std::dec << i << "].size = 0x" << std::dec
                      << sectionPtr1->size << std::endl
                      << "section2[" << std::dec << i << "].size = 0x" << std::dec
                      << sectionPtr2->size << std::endl;
            equal = false;
            // If size is different, data is not compared.
            continue;
        }

        if (sectionPtr1->data == NULL && sectionPtr2->data == NULL) {
            // The .bss section is empty.
            continue;
        }

        if (sectionPtr1->data == NULL || sectionPtr2->data == NULL) {
            // The index of the .bss section is different for both files.
            equal = false;
            continue;
        }

        if (memcmp((const void*)sectionPtr1->data, (const void*)sectionPtr2->data,
                   sectionPtr1->size)) {
            std::cout << "Section '" << sectionPtr1->name << "' is different" << std::endl;
            std::cout << "section1[" << std::dec << i << "].data != "
                      << "section2[" << std::dec << i << "].data" << std::endl;

            equal = false;
        }
    }

    return equal;
}

}  // namespace elf
}  // namespace lib
