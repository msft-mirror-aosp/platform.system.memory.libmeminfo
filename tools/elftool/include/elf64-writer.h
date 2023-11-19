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

#include <fstream>
#include <stdint.h>
#include <string>

#include "elf.h"
#include "elf64-binary.h"

namespace lib {
namespace elf {

// Class to write an Elf64Binary object to file.

class Elf64Writer {
public:
	// Writes the elf64 binary object to a file.
	static void WriteElfFile(Elf64Binary& elf64Binary, std::string& fileName);

private:
	static void OpenElfFile(std::string& fileName, std::ofstream& elfFile);
	static void CloseElfFile(std::ofstream& elfFile);
	static void WriteSections(std::ofstream& elfFile, Elf64Binary& elf64Binary);
	static void WriteLastSection(std::ofstream& elfFile, Elf64Binary& elf64Binary);
	static void Write(std::ofstream& elfFile, char *data, uint64_t size);
	static void WritePadding(std::ofstream& elfFile, uint64_t size);
};

} // namespace elf
} // namespace lib


