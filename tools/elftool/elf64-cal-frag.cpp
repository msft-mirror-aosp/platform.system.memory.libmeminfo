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

#include <filesystem>
#include <iostream>
#include <string>

#include "elf64-fragmentation.h"

// Calculates the memory fragmentation in ELF 64 shared libraries.
// It searches for the shared libraries in the sub-directories recursively.
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: " << argv[0] << " <directory>" << std::endl;
        return -1;
    }

    std::string rootDir(argv[1]);
    std::filesystem::file_status fs = std::filesystem::status(rootDir);

    if (!std::filesystem::is_directory(fs)) {
        std::cerr << "Provided path is not a directory: " << rootDir << std::endl;
        exit(-1);
    }

    lib::elf::Elf64Fragmentation elf64frag(rootDir);
    elf64frag.CalculateFragmentation();

    return 0;
}
