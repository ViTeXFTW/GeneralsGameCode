# BIG Archive Tool - Requirements & Scoping Document

## 1. Overview

This document scopes the requirements for adding a standalone BIG archive tool to the
GeneralsGameCode project. The tool will read and write EA BIG archive files (`.big`),
starting as a CLI application with a clean architecture that allows future expansion
to a GUI frontend. It will use the [BigXtractor](https://github.com/ViTeXFTW/BigXtractor)
library as its core engine.

## 2. Background

### 2.1 What are BIG files?

BIG files are EA's proprietary archive format used to package game assets (textures,
models, audio, INI configs, etc.) in Command & Conquer: Generals and Zero Hour. They
function similarly to ZIP files but use a simpler, uncompressed directory structure.

### 2.2 BIG File Format (BIGF Variant)

```
Header (16 bytes):
  [0x00] 4 bytes  char[4]   - Magic identifier: "BIGF" or "BIG4"
  [0x04] 4 bytes  uint32_le - Total archive size in bytes (little-endian)
  [0x08] 4 bytes  uint32_be - Number of files (big-endian)
  [0x0C] 4 bytes  uint32_be - Total header size / padding (big-endian)

Directory entries (starting at offset 0x10), repeated per file:
  4 bytes  uint32_be - File data offset (big-endian)
  4 bytes  uint32_be - File data size (big-endian)
  N bytes  char[]    - Null-terminated file path string (e.g. "data\ini\weapon.ini")

After all directory entries:
  Variable padding/metadata, then raw file data at the recorded offsets.
```

Key format notes:
- Mixed endianness: archive size is little-endian, all other integers are big-endian.
- Two header variants exist: `BIGF` (Generals, C&C3) and `BIG4` (BFME series).
  Both share the same structure; only the magic bytes differ.
- Files are stored uncompressed by default. Some archives may use EA's RefPack
  compression on individual entries (out of scope for initial version).

### 2.3 Existing Code in the Project

The game engine already contains BIG file **reading** logic in two platform variants:

| Component | Location |
|---|---|
| `ArchiveFile` (base class) | `Core/GameEngine/Include/Common/ArchiveFile.h` |
| `ArchiveFileSystem` (base class) | `Core/GameEngine/Include/Common/ArchiveFileSystem.h` |
| `Win32BIGFileSystem` | `Core/GameEngineDevice/Source/Win32Device/Common/Win32BIGFileSystem.cpp` |
| `StdBIGFileSystem` | `Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp` |

However, this code is **read-only** (no write/pack support) and **tightly coupled** to
the game engine's subsystems (`TheLocalFileSystem`, `GameMemory`, `GameAudio`, etc.),
making it unsuitable for direct reuse in a standalone tool.

### 2.4 External Library Assessment

#### BigXtractor (Recommended Baseline)

[BigXtractor](https://github.com/ViTeXFTW/BigXtractor) is a modern C++20 library
for reading and writing BIG archives. It is an excellent fit for this project:

| Attribute | Details |
|---|---|
| **Language** | C++20 (matches this project's standard) |
| **License** | MIT (compatible with this project's GPL-3.0) |
| **Build System** | CMake 3.20+ (aligns with this project's CMake 3.25) |
| **Read Support** | Yes -- memory-mapped I/O, zero-copy file access, case-insensitive lookup |
| **Write Support** | Yes -- create archives from disk files or in-memory data |
| **Dependencies** | None (C++ standard library only) |
| **Format Support** | BIGF (Generals, C&C3 era) |
| **Cross-platform** | Windows, Linux, macOS |

**Key API surface:**

```
High-level (Archive class):
  Archive::open(path)              -> std::optional<Archive>   // Open for reading
  Archive::create()                -> Archive                  // Create for writing
  archive.files()                  -> const vector<FileEntry>& // List all entries
  archive.fileCount()              -> size_t
  archive.findFile(path)           -> const FileEntry*         // Case-insensitive
  archive.extract(entry, destPath) -> bool                     // Extract to disk
  archive.extractToMemory(entry)   -> optional<vector<uint8_t>>
  archive.getFileView(entry)       -> span<const uint8_t>      // Zero-copy access
  archive.addFile(sourcePath, archivePath) -> bool             // Add from disk
  archive.addFile(data, archivePath)       -> bool             // Add from memory
  archive.write(destPath)          -> bool                     // Write archive

Low-level (Reader / Writer classes):
  Reader::open(path)               -> std::optional<Reader>    // Direct read access
  Writer with addFile() / write()                              // Direct write access

Types:
  FileEntry { path, lowercasePath, offset, size }
  ArchiveHeader { magic[4], archiveSize, fileCount, padding }
  ParseError (exception)
```

**Why BigXtractor is a good fit:**

1. **Complete feature set** -- Both read and write, which is exactly what's needed.
2. **License compatibility** -- MIT is permissive and can be included in GPL-3.0 projects.
3. **Same language/build stack** -- C++20, CMake. Drops in naturally.
4. **No external dependencies** -- No new transitive dependencies to manage.
5. **Clean API with separation** -- High-level `Archive` class for the CLI, low-level
   `Reader`/`Writer` available if the game engine needs direct access later.
6. **Memory-mapped I/O** -- Efficient for large archives, and the zero-copy `getFileView()`
   is ideal for a future GUI preview pane.
7. **Case-insensitive lookup** -- Matches the original game engine behavior.

#### Other Libraries Evaluated

| Library | Language | License | Read | Write | Verdict |
|---|---|---|---|---|---|
| [big4f](https://github.com/withmorten/big4f) | C | No license | Yes | Yes | Legally incompatible -- no explicit license. |
| [libbig](https://github.com/feliwir/libbig) | C++ | MIT | Yes | No | Read-only. BigXtractor supersedes it. |
| [OS BIG Editor](https://ppmforums.com/topic-14270/os-big-editor-information-and-download/) | Delphi | Open Source | Yes | Yes | GUI app, not a library. |

## 3. Requirements

### 3.1 Functional Requirements

#### FR-1: Extract (Unpack) a BIG archive

```
bigtool extract <archive.big> [output_directory]
```

- Open the archive with `bigx::Archive::open()`.
- Extract all files to the output directory using `archive.extract()`, recreating the
  internal directory structure.
- If no output directory is specified, extract to a directory named after the archive
  (e.g. `archive/`).
- Preserve original file paths (converting `\` to platform-native separators).

#### FR-2: Create (Pack) a BIG archive

```
bigtool create <input_directory> <archive.big>
```

- Recursively scan the input directory for all files.
- Use `bigx::Archive::create()` and `archive.addFile()` to build the archive.
- Write the final archive with `archive.write()`.
- File paths in the archive should use `\` separators (matching EA convention).

#### FR-3: List contents of a BIG archive

```
bigtool list <archive.big> [--verbose]
```

- Open the archive and iterate `archive.files()`.
- Display all files in the archive with their paths and sizes.
- Verbose mode adds: offset, total file count, total archive size.

#### FR-4: Extract a single file

```
bigtool extract <archive.big> --file <internal/path> [output_path]
```

- Use `archive.findFile()` for case-insensitive lookup of the named file.
- Extract the single file with `archive.extract()`.
- If output_path is omitted, extract to current directory preserving the filename.

### 3.2 Non-Functional Requirements

#### NFR-1: Cross-platform

- Must compile and run on Windows (MSVC, MinGW) and Linux (GCC, Clang).
- BigXtractor already supports all these platforms.

#### NFR-2: Separation of library and CLI

BigXtractor is already a standalone library, so the architecture naturally splits into:

```
external/BigXtractor/              <-- Git submodule (BigXtractor library)
    include/bigx/                  <-- Public API headers
    src/                           <-- Implementation
    CMakeLists.txt                 <-- Library build (target: bigx::bigx)

Core/Tools/BIGTool/                <-- CLI frontend
    BIGTool.cpp                    <-- main() with argument parsing
    CMakeLists.txt                 <-- Build config
    REQUIREMENTS.md                <-- This document
```

This separation enables:
- Future GUI frontends to link `bigx::bigx` directly without the CLI.
- Potential reuse by the game engine itself (replacing the current tightly-coupled code).
- Independent versioning -- BigXtractor can be updated via submodule.

#### NFR-3: Build integration

- Add BigXtractor as a **git submodule** under `external/BigXtractor/`.
- Add the submodule via `add_subdirectory(external/BigXtractor)` in the root or a
  dedicated `external/CMakeLists.txt`.
- Gate the BIGTool build behind `RTS_BUILD_CORE_EXTRAS` (following the pattern of
  similar utilities like Compress and CRCDiff).
- The tool links `bigx::bigx` and `corei_always`, outputs as `bigtool` executable.
- Disable BigXtractor's tests/examples when used as a submodule:
  ```cmake
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  add_subdirectory(external/BigXtractor)
  ```

#### NFR-4: File size limit

- Support archives up to 4 GB (uint32 offsets). Files within an archive are limited
  to ~2 GB per entry (consistent with the format and existing tooling).

#### NFR-5: Error handling

- BigXtractor provides error reporting via `std::string* outError` output parameters
  and `ParseError` exceptions. The CLI must translate these into clear user messages.
- Return non-zero exit codes on failure.

### 3.3 Out of Scope (Initial Version)

| Feature | Rationale |
|---|---|
| RefPack decompression | Generals BIG files don't use per-entry compression. Can be added later to BigXtractor. |
| BIG4 format support | BigXtractor currently targets BIGF. BIG4 (BFME) can be added upstream later. |
| In-place modification | Adding/removing files from an existing archive. Rebuild from directory instead. |
| GUI frontend | Planned for future; the library/CLI split in NFR-2 is the preparation. |
| Integration with game engine | Replacing `Win32BIGFileSystem` with BigXtractor. Separate effort. |

## 4. Proposed Architecture

### 4.1 CLI Frontend (BIGTool)

```
Usage: bigtool <command> [options]

Commands:
  list    <archive.big> [--verbose]           List archive contents
  extract <archive.big> [outdir]              Extract all files
  extract <archive.big> --file <path> [out]   Extract single file
  create  <directory> <archive.big>           Create archive from directory

Options:
  --verbose, -v    Show detailed information (file offsets, archive size)
  --file, -f       Specify a single file to extract
  --help, -h       Show usage information
```

### 4.2 CLI Implementation Sketch

```cpp
// BIGTool.cpp - Entry point (simplified)

#include <bigx/bigx.hpp>
#include <filesystem>
#include <iostream>

int cmdList(const std::string& archivePath, bool verbose) {
    std::string error;
    auto archive = bigx::Archive::open(archivePath, &error);
    if (!archive) {
        std::cerr << "Error: " << error << "\n";
        return 1;
    }
    std::cout << "Archive: " << archivePath << "\n";
    std::cout << "Files:   " << archive->fileCount() << "\n\n";
    for (const auto& file : archive->files()) {
        std::cout << "  " << file.path << "  (" << file.size << " bytes)";
        if (verbose) std::cout << "  @ offset " << file.offset;
        std::cout << "\n";
    }
    return 0;
}

int cmdExtract(const std::string& archivePath, const std::string& outDir,
               const std::string& singleFile) {
    std::string error;
    auto archive = bigx::Archive::open(archivePath, &error);
    if (!archive) { std::cerr << "Error: " << error << "\n"; return 1; }

    if (!singleFile.empty()) {
        const auto* entry = archive->findFile(singleFile);
        if (!entry) { std::cerr << "File not found: " << singleFile << "\n"; return 1; }
        auto destPath = outDir.empty()
            ? std::filesystem::path(entry->path).filename()
            : std::filesystem::path(outDir) / entry->path;
        if (!archive->extract(*entry, destPath, &error)) {
            std::cerr << "Extract failed: " << error << "\n"; return 1;
        }
        return 0;
    }

    // Extract all
    namespace fs = std::filesystem;
    fs::path dest = outDir.empty()
        ? fs::path(archivePath).stem()
        : fs::path(outDir);
    int count = 0;
    for (const auto& file : archive->files()) {
        fs::path fileDest = dest / file.path;
        if (!archive->extract(file, fileDest, &error)) {
            std::cerr << "Failed: " << file.path << ": " << error << "\n";
            continue;
        }
        ++count;
    }
    std::cout << "Extracted " << count << " files to " << dest << "\n";
    return 0;
}

int cmdCreate(const std::string& inputDir, const std::string& archivePath) {
    auto archive = bigx::Archive::create();
    std::string error;
    namespace fs = std::filesystem;
    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if (!entry.is_regular_file()) continue;
        auto relative = fs::relative(entry.path(), inputDir).string();
        if (!archive.addFile(entry.path(), relative, &error)) {
            std::cerr << "Failed to add " << relative << ": " << error << "\n";
            continue;
        }
        ++count;
    }
    if (!archive.write(archivePath, &error)) {
        std::cerr << "Failed to write archive: " << error << "\n";
        return 1;
    }
    std::cout << "Created " << archivePath << " with " << count << " files\n";
    return 0;
}

int main(int argc, char* argv[]) {
    // Parse arguments and dispatch to cmdList / cmdExtract / cmdCreate
    // ...
}
```

### 4.3 File Layout

```
GeneralsGameCode/
  external/
    BigXtractor/                 # Git submodule
      include/bigx/              #   Public headers (bigx.hpp, archive.hpp, etc.)
      src/                       #   Library source
      CMakeLists.txt             #   Library build target: bigx::bigx
  Core/
    Tools/
      BIGTool/
        CMakeLists.txt           # Executable target: core_bigtool -> bigtool
        BIGTool.cpp              # CLI entry point
        REQUIREMENTS.md          # This document
```

### 4.4 CMake Integration

**Root or external CMakeLists.txt:**
```cmake
# Disable BigXtractor extras when used as a submodule
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/BigXtractor)
```

**Tool** (`Core/Tools/BIGTool/CMakeLists.txt`):
```cmake
set(BIGTOOL_SRC
    "BIGTool.cpp"
)

add_executable(core_bigtool)
set_target_properties(core_bigtool PROPERTIES OUTPUT_NAME bigtool)

target_sources(core_bigtool PRIVATE ${BIGTOOL_SRC})

target_link_libraries(core_bigtool PRIVATE
    bigx::bigx
    corei_always
)

if(WIN32 OR "${CMAKE_SYSTEM}" MATCHES "Windows")
    target_link_options(core_bigtool PRIVATE /subsystem:console)
endif()
```

**Registration** (`Core/Tools/CMakeLists.txt`):
```cmake
if(RTS_BUILD_CORE_EXTRAS)
    add_subdirectory(BIGTool)
    ...
endif()
```

## 5. GUI Expansion Path

Using BigXtractor as the core library makes GUI expansion straightforward:

1. **Qt/Dear ImGui frontend** links `bigx::bigx` directly -- no CLI code needed.
2. `Archive::files()` returns a vector of `FileEntry` -- ready-made model for tree views.
3. `archive.getFileView()` provides zero-copy access -- ideal for preview panes
   (INI text, TGA images, W3D model info).
4. `archive.extractToMemory()` supports in-memory operations without temp files.
5. No global state or singletons in BigXtractor -- safe for GUI threading.
6. `archive.findFile()` with case-insensitive lookup -- matches search/filter UX needs.

Potential GUI features:
- Drag-and-drop archive opening
- Visual directory tree with file icons by type
- Preview pane for known formats (INI, TGA, W3D)
- Create/rebuild archive from drag-and-drop file selection
- Side-by-side diff of two archives

## 6. Implementation Plan

| Phase | Deliverable | Description |
|---|---|---|
| 1 | Submodule setup | Add BigXtractor as a git submodule under `external/BigXtractor/`. Wire into CMake. |
| 2 | CLI scaffold | Create `Core/Tools/BIGTool/` with argument parsing and `--help`. |
| 3 | `list` command | Implement `bigtool list` using `Archive::open()` and `archive.files()`. |
| 4 | `extract` command | Implement full and single-file extraction using `archive.extract()` / `findFile()`. |
| 5 | `create` command | Implement `bigtool create` using `Archive::create()`, `addFile()`, `write()`. |
| 6 | Testing | Round-trip validation: extract a game `.big` -> repack -> verify the game loads it or binary-compare. |
| 7 | CI integration | Ensure `bigtool` builds on all CI presets (win32, linux-gcc, clang). |

## 7. Integration Checklist

- [ ] `git submodule add https://github.com/ViTeXFTW/BigXtractor.git external/BigXtractor`
- [ ] Add `add_subdirectory(external/BigXtractor)` to CMake
- [ ] Create `Core/Tools/BIGTool/CMakeLists.txt`
- [ ] Create `Core/Tools/BIGTool/BIGTool.cpp`
- [ ] Register in `Core/Tools/CMakeLists.txt` under `RTS_BUILD_CORE_EXTRAS`
- [ ] Test build on win32 preset
- [ ] Test build on linux-gcc preset
- [ ] Validate against real game `.big` files (extract + repack round-trip)

## 8. References

- [BigXtractor library](https://github.com/ViTeXFTW/BigXtractor) -- Recommended baseline
- [BigXtractor integration guide](https://github.com/ViTeXFTW/BigXtractor/blob/main/INTEGRATION.md)
- [EA BIG BIGF Archive - Reverse Engineering Wiki](https://rewiki.miraheze.org/wiki/EA_BIG_BIGF_Archive)
- [big4f - CLI packer/unpacker](https://github.com/withmorten/big4f)
- [libbig - C++ read library](https://github.com/feliwir/libbig)
- Existing engine code: `Core/GameEngineDevice/Source/Win32Device/Common/Win32BIGFileSystem.cpp`
- Existing engine code: `Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp`
