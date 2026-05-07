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

// FILE: SourceLoc.h //////////////////////////////////////////////////////////
// Desc:   Source location for INI2 parser diagnostics and entity definitions.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/AsciiString.h"
#include "Common/GameType.h"

namespace INI2
{

/// Identifies a position in an INI file. Used by Diagnostics and by entity
/// stores to record where each definition lives (for go-to-definition tooling
/// and for "duplicate definition" diagnostics).
///
/// Column is best-effort. The legacy lexer is line-oriented and does not track
/// per-token columns; column == 0 means "unknown" / "whole line".
struct SourceLoc
{
	AsciiString  file;
	UnsignedInt  line   = 0;
	UnsignedInt  column = 0;

	SourceLoc() = default;
	SourceLoc(const AsciiString& f, UnsignedInt ln, UnsignedInt col = 0)
		: file(f), line(ln), column(col) {}

	Bool isValid() const { return line != 0; }
};

} // namespace INI2
