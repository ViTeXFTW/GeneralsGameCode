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

// FILE: ParseValue.h /////////////////////////////////////////////////////////
// Desc:   Strongly-typed value parsers for INI2.
//
// parseValue<V>(ParseContext&, V&) is the single extension point for value
// parsing. Adding a new value type means adding one overload here and one
// definition in ParseValue.cpp. The Field/Schema layer (Phase 2) selects
// the right overload at compile time from the field's pointer-to-member.
//
// Behavioral compatibility: every overload below produces a result that is
// bit-identical to the corresponding INI::parse* function in the legacy
// parser, given the same input tokens. The corpus-diff CI gate enforces
// this.
//
// Unit-converting parsers (parseAngleDegrees, parseDurationMsec, etc.) are
// declared as named free functions rather than parseValue<> overloads,
// because the in-memory storage type is plain Real for all of them — the
// distinguishing information is the Unit tag on the field, not the C++
// type of the member.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include "Common/AsciiString.h"
#include "Common/GameType.h"
#include "Lib/BaseType.h"

#include "Common/INI2/ParseContext.h"

namespace INI2
{

// ---------------------------------------------------------------------------
// Primary template — intentionally undefined, so an unknown V fails to link
// with a useful symbol name rather than silently doing the wrong thing.
// ---------------------------------------------------------------------------
template <typename V>
void parseValue(ParseContext& ctx, V& out);

// ---------------------------------------------------------------------------
// Primitive overloads. Behavior mirrors INI::parse* in INI.cpp.
// ---------------------------------------------------------------------------

template <> void parseValue<Byte>          (ParseContext& ctx, Byte& out);
template <> void parseValue<Short>         (ParseContext& ctx, Short& out);
template <> void parseValue<UnsignedShort> (ParseContext& ctx, UnsignedShort& out);
template <> void parseValue<Int>           (ParseContext& ctx, Int& out);
template <> void parseValue<UnsignedInt>   (ParseContext& ctx, UnsignedInt& out);
template <> void parseValue<Real>          (ParseContext& ctx, Real& out);
template <> void parseValue<Bool>          (ParseContext& ctx, Bool& out);

// ---------------------------------------------------------------------------
// String overloads.
//
// parseValue<AsciiString> mirrors INI::parseAsciiString (uses getNextAsciiString,
// which has the legacy quote-handling behavior). The "better quoted-string"
// variant from INI::parseQuotedAsciiString is exposed as a named function so
// fields that explicitly want it can opt in.
// ---------------------------------------------------------------------------

template <> void parseValue<AsciiString>                 (ParseContext& ctx, AsciiString& out);
template <> void parseValue<std::vector<AsciiString> >   (ParseContext& ctx, std::vector<AsciiString>& out);

void parseQuotedAsciiString    (ParseContext& ctx, AsciiString& out);
void parseAsciiStringVectorAppend(ParseContext& ctx, std::vector<AsciiString>& out);

// ---------------------------------------------------------------------------
// Composite overloads (X:val Y:val Z:val syntax, or R:val G:val B:val).
// These call ctx.nextSubToken() to enforce the leading tag.
// ---------------------------------------------------------------------------

template <> void parseValue<Coord3D>      (ParseContext& ctx, Coord3D& out);
template <> void parseValue<Coord2D>      (ParseContext& ctx, Coord2D& out);
template <> void parseValue<ICoord2D>     (ParseContext& ctx, ICoord2D& out);
template <> void parseValue<RGBColor>     (ParseContext& ctx, RGBColor& out);
template <> void parseValue<RGBAColorInt> (ParseContext& ctx, RGBAColorInt& out);

// ---------------------------------------------------------------------------
// Unit-converting parsers.
//
// All of these store into a plain Real (or integer rounding-up) member; the
// distinguishing information is the Unit tag on the field, not the C++ type
// of the storage. Field<T,Real> + Unit::Degrees selects parseAngleDegrees.
// ---------------------------------------------------------------------------

void parseAngleDegrees             (ParseContext& ctx, Real& out);          ///< degrees in, radians out
void parseAngularVelocityDegPerSec (ParseContext& ctx, Real& out);          ///< deg/sec in, rad/frame out
void parseVelocityDistPerSec       (ParseContext& ctx, Real& out);          ///< dist/sec in, dist/frame out
void parseAccelerationDistPerSec2  (ParseContext& ctx, Real& out);          ///< dist/sec^2 in, dist/frame^2 out
void parseDurationMsec             (ParseContext& ctx, Real& out);          ///< msec in, frames (Real) out
void parseDurationMsecToUInt       (ParseContext& ctx, UnsignedInt& out);   ///< msec in, frames (UnsignedInt, ceil) out
void parseDurationMsecToUShort     (ParseContext& ctx, UnsignedShort& out); ///< msec in, frames (UnsignedShort, ceil) out
void parsePositiveNonZeroReal      (ParseContext& ctx, Real& out);          ///< Real, must be > 0
void parsePercentToReal            (ParseContext& ctx, Real& out);          ///< "23%" or "0.23" in, 0.0-1.0 out

// ---------------------------------------------------------------------------
// Color packed into a single Int (legacy INI::parseColorInt).
// ---------------------------------------------------------------------------

void parseColorInt(ParseContext& ctx, Int& out);

} // namespace INI2
