# BIG Archive Tool - Requirements & Scoping Document

## 1. Overview

This document scopes the requirements for adding a standalone BIG archive tool to the
GeneralsGameCode project. The tool will read and write EA BIG archive files (`.big`),
starting as a CLI application with a clean architecture that allows future expansion
to a GUI frontend.

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
  [0x0C] 4 bytes  uint32_be - Total header size (big-endian)

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

#### "BigXtractor"

No library named "BigXtractor" was found on GitHub. The name may refer to a removed
or renamed project, or a confusion with one of the alternatives below.

#### Evaluated Alternatives

| Library | Language | License | Read | Write | Notes |
|---|---|---|---|---|---|
| [big4f](https://github.com/withmorten/big4f) | C | Unlicensed | Yes | Yes | Small (~3 files), supports BIGF + BIG4 pack/unpack. No explicit license. |
| [libbig](https://github.com/feliwir/libbig) | C++ | MIT | Yes | No | CMake-based, read-only. Uses Manager class API. |
| [OS BIG Editor](https://ppmforums.com/topic-14270/os-big-editor-information-and-download/) | Delphi | Open Source | Yes | Yes | GUI tool, not a library. Not usable as a dependency. |

#### Recommendation

**Do not adopt any external library as a baseline.** Rationale:

1. **big4f** has no license, making it legally incompatible with this GPL-3.0 project.
2. **libbig** is read-only and adds no value over writing a purpose-built implementation.
3. The BIG format is simple enough (~100-200 lines of core logic) that a clean,
   purpose-built implementation is preferable to importing external code.
4. The existing engine parsing code in `Win32BIGFileSystem.cpp` / `StdBIGFileSystem.cpp`
   serves as a well-tested **reference implementation** for the read path.

## 3. Requirements

### 3.1 Functional Requirements

#### FR-1: Extract (Unpack) a BIG archive

```
bigtool extract <archive.big> [output_directory]
```

- Parse the BIGF/BIG4 header and directory entries.
- Extract all files to the output directory, recreating the internal directory structure.
- If no output directory is specified, extract to a directory named after the archive
  (e.g. `archive/`).
- Preserve original file paths (converting `\` to platform-native separators).

#### FR-2: Create (Pack) a BIG archive

```
bigtool create <input_directory> <archive.big> [--format bigf|big4]
```

- Recursively scan the input directory for all files.
- Build a BIGF (default) or BIG4 archive containing all discovered files.
- File paths in the archive should use `\` separators (matching EA convention).
- Default format: BIGF (the format used by Generals/Zero Hour).

#### FR-3: List contents of a BIG archive

```
bigtool list <archive.big> [--verbose]
```

- Display all files in the archive with their paths and sizes.
- Verbose mode adds: offset, archive header variant, total archive size.

#### FR-4: Extract a single file

```
bigtool extract <archive.big> --file <internal/path> [output_path]
```

- Extract a single named file from the archive.
- If output_path is omitted, extract to current directory preserving the filename.

### 3.2 Non-Functional Requirements

#### NFR-1: Cross-platform

- Must compile and run on Windows (MSVC, MinGW) and Linux (GCC, Clang).
- Use only standard C++ and portable I/O (no Win32 API, no engine dependencies).

#### NFR-2: Separation of library and CLI

The implementation must be split into two layers:

```
Core/Libraries/Source/BIGArchive/   <-- Standalone library (no CLI, no engine deps)
    BIGArchive.h                    <-- Public API header
    BIGArchive.cpp                  <-- Implementation

Core/Tools/BIGTool/                 <-- CLI frontend
    BIGTool.cpp                     <-- main() with argument parsing
    CMakeLists.txt                  <-- Build config
```

This separation enables:
- Future GUI frontends to link the library without the CLI.
- Potential reuse by the game engine itself (replacing the current tightly-coupled code).
- Unit testing of the library independently.

#### NFR-3: Build integration

- Add a new CMake build flag: `RTS_BUILD_BIGTOOL` (default: OFF, or gate behind
  `RTS_BUILD_CORE_EXTRAS`).
- Follow the existing tool pattern: link to `corei_always`, output as `bigtool`
  executable.
- No new external dependencies (pure C++ standard library + existing project infra).

#### NFR-4: File size limit

- Support archives up to 4 GB (uint32 offsets). Files within an archive are limited
  to ~2 GB per entry (consistent with the format and existing tooling).

#### NFR-5: Error handling

- Validate magic bytes on read. Report clear errors for corrupt/unsupported files.
- Handle I/O errors gracefully (missing files, permission denied, disk full).
- Return non-zero exit codes on failure.

### 3.3 Out of Scope (Initial Version)

| Feature | Rationale |
|---|---|
| RefPack decompression | Generals BIG files don't use per-entry compression. Can be added later. |
| In-place modification | Adding/removing files from an existing archive. Rebuild from directory instead. |
| GUI frontend | Planned for future; the library/CLI split in NFR-2 is the preparation. |
| Streaming / memory-mapped I/O | Premature optimization. Standard file I/O is sufficient. |
| Integration with game engine | Replacing `Win32BIGFileSystem` with the new library. Separate effort. |

## 4. Proposed Architecture

### 4.1 Library API (BIGArchive)

```cpp
// BIGArchive.h - Public API

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

enum class BIGFormat {
    BIGF,  // "BIGF" - Generals, C&C3, etc.
    BIG4   // "BIG4" - BFME series
};

struct BIGEntry {
    std::string path;       // Internal path (e.g. "data\\ini\\weapon.ini")
    uint32_t    offset;     // Byte offset of file data in archive
    uint32_t    size;       // File data size in bytes
};

struct BIGArchiveInfo {
    BIGFormat              format;
    uint32_t               archiveSize;
    std::vector<BIGEntry>  entries;
};

// --- Read operations ---

// Parse archive header and directory. Does not read file data.
bool bigReadDirectory(const std::string& archivePath, BIGArchiveInfo& outInfo);

// Extract a single entry to a file on disk.
bool bigExtractEntry(const std::string& archivePath, const BIGEntry& entry,
                     const std::string& outputPath);

// Extract all entries to a directory.
bool bigExtractAll(const std::string& archivePath, const BIGArchiveInfo& info,
                   const std::string& outputDir);

// --- Write operations ---

// Create a new archive from a list of files on disk.
// inputBasePath is stripped from file paths to form archive-internal paths.
bool bigCreate(const std::string& outputPath, const std::string& inputBasePath,
               const std::vector<std::string>& filePaths, BIGFormat format);
```

### 4.2 CLI Frontend (BIGTool)

```
Usage: bigtool <command> [options]

Commands:
  list    <archive.big> [--verbose]           List archive contents
  extract <archive.big> [outdir]              Extract all files
  extract <archive.big> --file <path> [out]   Extract single file
  create  <directory> <archive.big> [--big4]  Create archive from directory

Options:
  --verbose, -v    Show detailed information
  --file, -f       Specify a single file to extract
  --big4           Use BIG4 format instead of BIGF (default)
  --help, -h       Show usage information
```

### 4.3 File Layout

```
Core/
  Libraries/
    Source/
      BIGArchive/
        CMakeLists.txt      # Library target: core_bigarchive
        BIGArchive.h        # Public header
        BIGArchive.cpp      # Implementation
  Tools/
    BIGTool/
      CMakeLists.txt        # Executable target: core_bigtool -> bigtool
      BIGTool.cpp           # CLI entry point
      REQUIREMENTS.md       # This document
```

### 4.4 CMake Integration

**Library** (`Core/Libraries/Source/BIGArchive/CMakeLists.txt`):
```cmake
add_library(core_bigarchive STATIC)
target_sources(core_bigarchive PRIVATE BIGArchive.cpp)
target_include_directories(core_bigarchive PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

**Tool** (`Core/Tools/BIGTool/CMakeLists.txt`):
```cmake
add_executable(core_bigtool)
set_target_properties(core_bigtool PROPERTIES OUTPUT_NAME bigtool)
target_sources(core_bigtool PRIVATE BIGTool.cpp)
target_link_libraries(core_bigtool PRIVATE core_bigarchive corei_always)

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

The library/CLI split explicitly prepares for a future GUI application:

1. **Qt/Dear ImGui frontend** can link `core_bigarchive` directly.
2. The `BIGArchiveInfo` struct provides a ready-made model for tree views.
3. No global state or singletons in the library -- safe for GUI threading.
4. Potential GUI features: drag-and-drop, visual directory tree, preview pane
   for known file types (INI, TGA, W3D).

## 6. Implementation Plan

| Phase | Deliverable | Description |
|---|---|---|
| 1 | `core_bigarchive` library | Read path: parse headers, list entries, extract files. Reference `Win32BIGFileSystem.cpp` for format parsing. |
| 2 | `core_bigtool` CLI (read) | CLI commands: `list` and `extract`. |
| 3 | `core_bigarchive` write path | Pack files into a new BIG archive with correct header/directory layout. |
| 4 | `core_bigtool` CLI (write) | CLI command: `create`. |
| 5 | Testing | Validate round-trip: extract existing game .big -> repack -> binary compare or game-load test. |

## 7. References

- [EA BIG BIGF Archive - Reverse Engineering Wiki](https://rewiki.miraheze.org/wiki/EA_BIG_BIGF_Archive)
- [big4f - CLI packer/unpacker](https://github.com/withmorten/big4f)
- [libbig - C++ read library](https://github.com/feliwir/libbig)
- [OS BIG Editor](https://ppmforums.com/topic-14270/os-big-editor-information-and-download/)
- Existing engine code: `Core/GameEngineDevice/Source/Win32Device/Common/Win32BIGFileSystem.cpp`
- Existing engine code: `Core/GameEngineDevice/Source/StdDevice/Common/StdBIGFileSystem.cpp`
