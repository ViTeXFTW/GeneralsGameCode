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

// FILE: BlockRegistry.cpp ////////////////////////////////////////////////////
// Desc:   Implementation of the process-wide BlockRegistry singleton.
///////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"  // must be first

#include "Common/INI2/BlockRegistry.h"

namespace INI2
{

void BlockRegistry::registerBlock(const char* keyword, BlockHandler handler)
{
	m_handlers[std::string(keyword)] = handler;
}

BlockHandler BlockRegistry::find(const char* keyword) const
{
	auto it = m_handlers.find(std::string(keyword));
	if (it == m_handlers.end())
		return nullptr;
	return it->second;
}

std::vector<AsciiString> BlockRegistry::keywords() const
{
	std::vector<AsciiString> out;
	out.reserve(m_handlers.size());
	for (const auto& entry : m_handlers)
		out.push_back(AsciiString(entry.first.c_str()));
	return out;
}

BlockRegistry& theBlockRegistry()
{
	// Meyers singleton. Constructed on first call, destroyed at process
	// exit. Survives the static-init phase so INI2_REGISTER_BLOCK works
	// regardless of translation-unit init order.
	static BlockRegistry registry;
	return registry;
}

} // namespace INI2
