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

// FILE: Field.h //////////////////////////////////////////////////////////////
// Desc:   Strongly-typed field descriptor for INI2 schemas.
//
// Field<T,V> binds an INI token (e.g. "FenceWidth") to a member-pointer
// (V T::*) of the class T. The compiler enforces that the member exists
// and that V matches the declared type, eliminating the offsetof + void*
// pairing that the legacy FieldParse struct uses.
//
// The base class FieldBase<T> exists so a Schema<T> can hold a heterogeneous
// list of Field<T, V1>, Field<T, V2>, ... behind a single virtual entry
// point. The virtual call cost is paid once per parsed line in INI files
// loaded at startup; it is not on any per-frame path.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <type_traits>
#include "Common/AsciiString.h"
#include "Common/GameType.h"
#include "Common/INI.h"          // FieldParse, INIFieldParseProc
#include "Lib/BaseType.h"

#include "Common/INI2/ParseContext.h"
#include "Common/INI2/ParseValue.h"
#include "Common/INI2/ValueTypes.h"

namespace INI2
{

// ===========================================================================
// memberOffset: pointer-to-member -> byte offset within T.
// ===========================================================================
// The legacy parser stores the offset directly via offsetof; we match its
// representation so the legacy adapter can populate FieldParse::offset
// correctly. Using the null-pointer trick mirrors the canonical offsetof
// expansion for non-virtual, standard-layout types — which all INI-bearing
// engine classes are.
template <typename T, typename V>
inline UnsignedInt memberOffset(V T::* m)
{
	// Cast 0 to T*, take address of the member, subtract the original null:
	// the result is the offset, in bytes, of the member within T.
	return static_cast<UnsignedInt>(
		reinterpret_cast<size_t>(
			&(reinterpret_cast<T*>(0)->*m)
		)
	);
}

// ===========================================================================
// FieldBase: type-erased view of a field, keyed on the owning class T only.
// ===========================================================================
template <typename T>
class FieldBase
{
public:
	const char*  token   = nullptr;   ///< INI token, e.g. "FenceWidth". Lifetime is program lifetime (string literal).
	UnsignedInt  offset  = 0;         ///< Byte offset of the storage member within T.
	UnsignedInt  flags   = 0;         ///< FieldFlag bits. Currently informational; consumed by Schema export.
	const char*  doc     = nullptr;   ///< Optional documentation string. May be nullptr.
	Unit         unit    = Unit::None;///< Unit tag (selects unit-aware parser at parse time).

	virtual ~FieldBase() = default;

	/// Parse the next value off the line and store it at 'store'.
	/// 'store' has already had this field's offset applied by the caller
	/// (matching the legacy parser's calling convention at INI.cpp:1536).
	virtual void parse(ParseContext& ctx, void* store) const = 0;

	/// Emit a legacy FieldParse row equivalent to this field. Typed
	/// fields (Field<T,V>) emit rows that route through legacyTrampoline<T>;
	/// raw fields (RawField<T>) emit rows that point directly at a legacy
	/// parser function with userData, bypassing the trampoline. This split
	/// lets a single Schema mix typed primitive fields with legacy-style
	/// fields (enum-name-table-driven parsers, custom parsers) — necessary
	/// for migrating classes that have a few rows the typed system does
	/// not yet cover.
	virtual void emitLegacyRow(FieldParse& row) const = 0;
};

// ===========================================================================
// Field<T,V>: typed field. Compile-time-checked binding to V T::*.
// ===========================================================================
//
// Forward-declared so RawField can reference it before its full definition;
// also lets the trampoline (LegacyAdapter.h) hold a FieldBase<T>* pointer
// stably regardless of which concrete subclass a row uses.
template <typename T> void legacyTrampoline(INI*, void*, void*, const void*);

template <typename T, typename V>
class Field final : public FieldBase<T>
{
public:
	V T::* member;

	Field(const char* tok, V T::* mem, Unit u = Unit::None)
		: member(mem)
	{
		this->token  = tok;
		this->offset = memberOffset(mem);
		this->unit   = u;
	}

	void parse(ParseContext& ctx, void* store) const override
	{
		dispatchByUnit(ctx, *static_cast<V*>(store));
	}

	void emitLegacyRow(FieldParse& row) const override
	{
		row.token    = this->token;
		row.parse    = &legacyTrampoline<T>;   // typed dispatch via FieldBase<T>::parse
		row.userData = this;                    // trampoline casts back to FieldBase<T>*
		row.offset   = static_cast<Int>(this->offset);
	}

private:
	// Dispatch table: (V, Unit) -> parser.
	//
	// Only valid (V, Unit) pairings compile through; unsupported combinations
	// (e.g. Unit::Degrees on a member of type Bool) fall through to the
	// default parseValue<V> overload and the user gets a clear schema-
	// declaration mistake at the first INI load rather than silent data
	// corruption.
	void dispatchByUnit(ParseContext& ctx, V& store) const
	{
		switch (this->unit)
		{
			case Unit::Degrees:
				if constexpr (std::is_same_v<V, Real>) { parseAngleDegrees(ctx, store); return; }
				break;
			case Unit::DegPerSecToRadPerFrame:
				if constexpr (std::is_same_v<V, Real>) { parseAngularVelocityDegPerSec(ctx, store); return; }
				break;
			case Unit::DistPerSecToDistPerFrame:
				if constexpr (std::is_same_v<V, Real>) { parseVelocityDistPerSec(ctx, store); return; }
				break;
			case Unit::DistPerSec2ToDistPerFrame2:
				if constexpr (std::is_same_v<V, Real>) { parseAccelerationDistPerSec2(ctx, store); return; }
				break;
			case Unit::MillisecondsToFrames:
				if constexpr (std::is_same_v<V, Real>)          { parseDurationMsec(ctx, store); return; }
				if constexpr (std::is_same_v<V, UnsignedInt>)   { parseDurationMsecToUInt(ctx, store); return; }
				if constexpr (std::is_same_v<V, UnsignedShort>) { parseDurationMsecToUShort(ctx, store); return; }
				break;
			case Unit::Percent:
				if constexpr (std::is_same_v<V, Real>) { parsePercentToReal(ctx, store); return; }
				break;
			case Unit::None:
			case Unit::WorldUnits:
				// WorldUnits is documentation-only; parses as raw V.
				break;
		}
		// Default: raw parser for V. PositiveOnly flag is enforced after the
		// raw parse to reuse the same numeric scan.
		parseValue<V>(ctx, store);
		if ((this->flags & FieldFlag::PositiveOnly) != 0)
		{
			if constexpr (std::is_same_v<V, Real>)
			{
				if (store <= 0.0f)
				{
					ctx.emitError(Codes::ValueOutOfRange,
						"value must be > 0");
					// Match legacy parsePositiveNonZeroReal: throw to abort
					// the field rather than silently storing.
					throw INI_INVALID_DATA;
				}
			}
		}
	}
};

// ===========================================================================
// RawField<T>: legacy-style field. Holds a legacy parser function pointer
// and a userData payload, emitted as a FieldParse row that bypasses the
// trampoline. Used during migration for fields whose value type the typed
// parseValue<V> system does not yet cover (enum/bit-flag tables driven by
// userData, custom per-class parsers like Locomotor's parseFrictionPerSec).
//
// The compile-time pointer-to-member typing on the constructor still
// guarantees the offset is correct for the named member. Only the parser
// function and its userData are legacy.
// ===========================================================================
template <typename T>
class RawField final : public FieldBase<T>
{
public:
	INIFieldParseProc legacyParser  = nullptr;
	const void*       legacyUserData = nullptr;

	template <typename V>
	RawField(const char* tok, V T::* mem,
	         INIFieldParseProc parser, const void* userData)
		: legacyParser(parser), legacyUserData(userData)
	{
		this->token  = tok;
		this->offset = memberOffset(mem);
	}

	/// Used only if the typed dispatch ever routes here through
	/// legacyTrampoline (it does not: emitLegacyRow points the row directly
	/// at legacyParser, so this method is unreachable in practice). Provided
	/// for completeness so the abstract base class is satisfied.
	void parse(ParseContext& ctx, void* store) const override
	{
		legacyParser(ctx.legacy(), nullptr, store, legacyUserData);
	}

	void emitLegacyRow(FieldParse& row) const override
	{
		row.token    = this->token;
		row.parse    = legacyParser;       // direct legacy parser; no trampoline
		row.userData = legacyUserData;
		row.offset   = static_cast<Int>(this->offset);
	}
};

} // namespace INI2
