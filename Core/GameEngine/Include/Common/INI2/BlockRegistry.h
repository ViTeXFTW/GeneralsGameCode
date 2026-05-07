/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// FILE: BlockRegistry.h //////////////////////////////////////////////////////
// Desc:   Replaces theTypeTable[] (INI.cpp:85-150) with a runtime-populated
//         registry of block-keyword -> handler mappings.
//
// In the legacy parser every top-level INI block keyword ("Object", "Weapon",
// "Armor", "Science", ...50+ entries) lives in a single hand-typed static
// array. Adding a new block type means editing that array. In the new
// design each subsystem registers its block at static-init time:
//
//     INI2_REGISTER_BLOCK("Object", &parseObjectV2);
//
// The BlockRegistry is global. Lookups happen during INI::loadV2's per-line
// dispatch loop. If a keyword is not found in the registry, loadV2 falls
// back to findBlockParse() against the legacy theTypeTable[]. This lets
// keywords migrate one at a time without flag days.
//
// Thread-safety: registration runs at static-init (single-threaded by
// definition). Lookups run during INI loading (also single-threaded by
// engine convention). No mutex.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "Common/AsciiString.h"
#include "Common/GameType.h"

namespace INI2
{

class ParseSession;

/// Block handler signature. The ParseSession exposes:
///   - session.ini()         -- the INI reader for token consumption and
///                              for calling the legacy initFromINI driver
///                              against a Schema's asLegacyFieldParse().
///   - session.loadSession() -- LoadMode + override-tracking. Branch on
///                              loadSession()->createsOverrides() in place
///                              of the legacy ini->getLoadType() check.
///   - session.sink()        -- diagnostic sink for recoverable errors.
///
/// The handler is called after the keyword token has been consumed by the
/// loader; it is responsible for reading any trailing arguments (e.g. the
/// instance name for "Object FooName") via session.ini()->getNextToken(),
/// finding-or-creating the target instance, and walking the body via the
/// existing initFromINI machinery.
typedef void (*BlockHandler)(ParseSession& session);

/// Process-wide registry. Access via theBlockRegistry().
class BlockRegistry
{
public:
	/// Register a handler for a block keyword. Re-registering the same
	/// keyword replaces the existing handler (useful in tests; in
	/// production registration happens once at static-init).
	void registerBlock(const char* keyword, BlockHandler handler);

	/// Look up a handler. Returns nullptr if no handler is registered for
	/// this keyword. Lookup is case-sensitive (matching the legacy
	/// findBlockParse strcmp behavior at INI.cpp:391).
	BlockHandler find(const char* keyword) const;

	/// Number of registered keywords. Useful for asserting registration in
	/// tests and for sizing schema-export output.
	size_t size() const { return m_handlers.size(); }

	/// Enumerate registered keywords. Order is unspecified.
	std::vector<AsciiString> keywords() const;

private:
	std::unordered_map<std::string, BlockHandler> m_handlers;
};

/// Process-wide BlockRegistry singleton. Survives for program lifetime.
BlockRegistry& theBlockRegistry();

/// Helper used by INI2_REGISTER_BLOCK. Constructing one of these at
/// namespace scope registers the handler at static-init time.
class BlockRegistration
{
public:
	BlockRegistration(const char* keyword, BlockHandler handler)
	{
		theBlockRegistry().registerBlock(keyword, handler);
	}
};

#define INI2_PASTE_INNER(a, b) a##b
#define INI2_PASTE(a, b) INI2_PASTE_INNER(a, b)

/// Static-init registration macro. Place at file scope:
///     INI2_REGISTER_BLOCK("Object", &parseObjectV2);
#define INI2_REGISTER_BLOCK(keyword, handler) \
	static const ::INI2::BlockRegistration \
		INI2_PASTE(ini2_block_reg_, __LINE__)((keyword), (handler))

} // namespace INI2
