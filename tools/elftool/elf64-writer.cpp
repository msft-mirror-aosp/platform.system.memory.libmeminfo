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

#include "elf64-writer.h"

#include <fstream>
#include <iostream>
#include <string>
#include <stdlib.h>

#include "elf.h"
#include "elf64-binary.h"

namespace lib {
namespace elf {

// Writes the elf64 binary object to a file.
//
// It assumes that the elf file will have these parts in this order:
//
// - Executable header
// - Program headers (only for executables)
// - Sections (.interp, .init, .plt, .text, .rodata, .data, .bss, .shstrtab, etc).
// - Section headers
//
// Note that this assumption is not always true. The Executable header is always
// at the beginning of the elf file and the other parts (program headers,
// sections, section headers) could be in any location.
void Elf64Writer::WriteElfFile(Elf64Binary& elf64Binary, std::string& fileName)
{
	std::cout << "Writing EL64 binary to file " << fileName << std::endl;
	std::ofstream elfFile;
	OpenElfFile(fileName, elfFile);

	// Write Elf header
	Write(elfFile, (char *) &elf64Binary.ehdr, sizeof(elf64Binary.ehdr));

	// Write Program Headers.
	for (int i = 0; i < elf64Binary.phdrs.size(); i++)
	{
		Elf64_Phdr *phdrPtr = elf64Binary.phdrs[i];
		Write(elfFile, (char *) phdrPtr, sizeof(*phdrPtr));
	}

	// Write Sections.
	WriteSections(elfFile, elf64Binary);

	// Write Section Headers.
	for (int i = 0; i < elf64Binary.shdrs.size(); i++)
	{
		Elf64_Shdr *shdrPtr = elf64Binary.shdrs[i];
		Write(elfFile, (char *) shdrPtr, sizeof(*shdrPtr));
	}

	elfFile.close();
}

void Elf64Writer::OpenElfFile(std::string& fileName, std::ofstream& elfFile)
{
	elfFile.open(fileName.c_str(), std::ofstream::out);
	if (!elfFile.is_open())
	{
		std::cerr << "Failed to open the file: " << fileName << std::endl;
		exit(-1);
	}
}

void Elf64Writer::CloseElfFile(std::ofstream& elfFile)
{
	if (elfFile.is_open())
	{
		elfFile.close();
	}
}

void Elf64Writer::WriteSections(std::ofstream& elfFile, Elf64Binary& elf64Binary)
{
	// The content of the first section consists of the ELF header (64 bytes)
	// and the ELF program headers. This content was already written to the file
	// when the ELF header and ELF program headers were written, so we ignore
	// the first section.
	for (int i = 1; i < (elf64Binary.sections.size() - 1); i++)
	{
		Elf64_Shdr *shdrPtr = elf64Binary.shdrs[i];
		Elf64_Shdr *nextShdrPtr = elf64Binary.shdrs[i + 1];

		if (shdrPtr->sh_type == SHT_NOBITS)
		{
			// Skip .bss section because it is empty.
			continue;
		}

		Elf64_Sc *sPtr = elf64Binary.sections[i];
		Write(elfFile, (char *)sPtr->data, sPtr->size);

		// Note that sPtr->size == shdrPtr->sh_size;
		uint64_t padding = nextShdrPtr->sh_offset - (shdrPtr->sh_offset + shdrPtr->sh_size);
		WritePadding(elfFile, padding);
	}

	WriteLastSection(elfFile, elf64Binary);
}

void Elf64Writer::WriteLastSection(std::ofstream& elfFile, Elf64Binary& elf64Binary)
{
	if (elf64Binary.sections.size() > 1)
	{
		Elf64_Shdr *shdrPtr = elf64Binary.shdrs[elf64Binary.sections.size() - 1];
		Elf64_Sc *sPtr = elf64Binary.sections[elf64Binary.sections.size() - 1];
		Write(elfFile, (char *)sPtr->data, sPtr->size);

		// The padding of the last section is calculated using the formula:
		// padding = Elf64_Ehdr.e_shoff - (last Elf64_Shdr.sh_offset + last Elf64_Shdr.sh_size);
		uint64_t padding = elf64Binary.ehdr.e_shoff - (shdrPtr->sh_offset + shdrPtr->sh_size);
		WritePadding(elfFile, padding);
	}
}

void Elf64Writer::Write(std::ofstream& elfFile, char *data, uint64_t size)
{
	elfFile.write(data, size);
	if (!elfFile.good())
	{
		std::cerr << "Failed to write [" << std::dec << size << "] bytes"
			<< std::endl;
		exit(-1);
	}
}

void Elf64Writer::WritePadding(std::ofstream& elfFile, uint64_t size)
{
	// The padding is populated with zeros.
	char *zeroPtr = (char *) calloc(1, size);
	if (zeroPtr == NULL)
	{
		std::cerr << "Failed to allocate [" << std::dec << size << "] bytes";
	}

	Write(elfFile, zeroPtr, size);
	free(zeroPtr);
}

} // elf
} // lib

