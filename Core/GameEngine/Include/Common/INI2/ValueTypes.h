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

// FILE: ValueTypes.h /////////////////////////////////////////////////////////
// Desc:   Unit tags and constraint metadata for INI2 fields.
//
// In Phase 1 these tags are consumed by the named unit-aware parse functions
// declared in ParseValue.h. In Phase 2 they become Field metadata used both
// by the parser and by the schema dumper.
//
// The legacy parser collapses several distinct semantic types into "Real":
//   - parseAngleReal stores radians from a degrees input.
//   - parseDurationReal stores frames from a milliseconds input.
//   - parseVelocityReal stores dist/frame from dist/sec input.
//   - parseAccelerationReal stores dist/frame^2 from dist/sec^2 input.
//   - parseAngularVelocityReal stores rads/frame from deg/sec input.
//   - parsePercentToReal stores 0.0-1.0 from "23%" or "0.23" input.
//
// Rather than introducing strong typedefs that would require changing
// hundreds of struct member declarations across the engine, INI2 keeps the
// in-memory storage type unchanged (typically Real) and uses these Unit tags
// as field metadata. The schema selects the matching parser, which performs
// the conversion before storing.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/GameType.h"

namespace INI2
{

/// Unit / interpretation tag for numeric fields. The default is None, which
/// means "store the parsed value as-is".
///
/// A field's Unit selects the matching named parse function in ParseValue.h.
/// Adding a new unit means: extend this enum, add a parse function, wire it
/// in the Field-to-parser dispatch (Phase 2).
enum class Unit
{
	None,                       ///< No conversion. Store raw.
	WorldUnits,                 ///< Distance in world units. Documentation only.
	Degrees,                    ///< Input degrees, store as radians.
	MillisecondsToFrames,       ///< Input msec, store as frames (Real or rounded-up integer).
	DistPerSecToDistPerFrame,   ///< Input dist/sec, store as dist/frame.
	DistPerSec2ToDistPerFrame2, ///< Input dist/sec^2, store as dist/frame^2.
	DegPerSecToRadPerFrame,     ///< Input deg/sec, store as rad/frame.
	Percent,                    ///< Input "23%" or "0.23", store as 0.0-1.0.
};

/// Numeric range constraint. Optional metadata; today the legacy parser hard-
/// codes ranges inline (e.g. parseUnsignedByte rejects values outside [0,255]).
/// In Phase 2 fields can attach a Range to surface constraints in diagnostics
/// and in schema export. Phase 1 ignores it.
struct Range
{
	Real min = 0.0f;
	Real max = 0.0f;
	Bool hasMin = FALSE;
	Bool hasMax = FALSE;

	static Range atLeast(Real m)        { Range r; r.min = m; r.hasMin = TRUE; return r; }
	static Range atMost(Real m)         { Range r; r.max = m; r.hasMax = TRUE; return r; }
	static Range between(Real lo, Real hi)
	{
		Range r;
		r.min = lo; r.max = hi;
		r.hasMin = TRUE; r.hasMax = TRUE;
		return r;
	}
};

/// Optional per-field flags. Used by Field metadata in Phase 2.
namespace FieldFlag
{
	enum Bits : UnsignedInt
	{
		None         = 0,
		Required     = 1u << 0,  ///< Field must be present in every block.
		Deprecated   = 1u << 1,  ///< Warn on use; still parsed.
		ListAppend   = 1u << 2,  ///< For list fields: append rather than replace.
		ReskinSafe   = 1u << 3,  ///< May be set by an ObjectReskin block (replaces s_objectReskinFieldParseTable).
		Internal     = 1u << 4,  ///< Hidden from schema export; engine-internal use.
		PositiveOnly = 1u << 5,  ///< Reject zero or negative values.
	};
}

} // namespace INI2
