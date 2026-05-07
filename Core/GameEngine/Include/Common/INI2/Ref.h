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

// FILE: Ref.h ////////////////////////////////////////////////////////////////
// Desc:   Cross-INI reference template.
//
// In INI text, fields like
//     PrimaryWeapon  = TankCannon
//     ArmorSet       = Conditions = NONE  Armor = TankArmor  ...
// reference another defined entity by name. The legacy parser has 9 nearly-
// identical parse*Template functions (parseWeaponTemplate, parseArmorTemplate,
// parseFXList, parseParticleSystemTemplate, parseObjectCreationList,
// parseThingTemplate, parseUpgradeTemplate, parseSpecialPowerTemplate,
// parseDamageFX), each calling its own store's findX(name) and storing a
// raw pointer. Each handles "None" slightly differently and has its own
// null-store guard.
//
// Ref<T> is a strong typedef wrapping {name, source-location, cached-pointer}
// that schemas can use as a typed field. Resolution is dispatched through
// the RefResolver<T> customization point — specialized once per engine
// reference type in Refs.h / Refs.cpp.
//
// Phase 5 introduces Ref<T> alongside the legacy parsers; classes migrate
// to it incrementally in later phases. The legacy INI::parse*Template
// symbols are left untouched in this phase to avoid risk before the
// corpus-diff CI gate (Phase 6+) is in place to prove byte-for-byte
// equivalence after refactoring.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstring>

#include "Common/AsciiString.h"
#include "Common/GameType.h"

#include "Common/INI2/ParseContext.h"
#include "Common/INI2/SourceLoc.h"

namespace INI2
{

// ===========================================================================
// RefResolver<T>: customization point. Specialized per engine reference
// type in Refs.h / Refs.cpp.
// ===========================================================================
//
// A specialization must provide:
//     static const T* resolve(const AsciiString& name);
//     static const char* typeName();   // schema-export friendly, e.g. "Weapon"
//
// 'resolve' returns nullptr for the special name "None" or for unknown
// names — matching the legacy parsers' behavior, so byte-for-byte
// compatibility holds when a class migrates to Ref<T>.
template <typename T>
struct RefResolver;

// ===========================================================================
// Ref<T>: holds a named cross-reference plus its lazily-resolved pointer.
// ===========================================================================
template <typename T>
class Ref
{
public:
	Ref() = default;

	explicit Ref(const AsciiString& name, SourceLoc loc = {})
		: m_name(name), m_definedAt(loc) {}

	const AsciiString& name() const       { return m_name; }
	SourceLoc          definedAt() const  { return m_definedAt; }
	Bool               isEmpty() const    { return m_name.isEmpty(); }

	/// True if the parsed name was the special placeholder "None"
	/// (case-insensitive). Useful for callers that want to distinguish
	/// "explicitly unset" from "never parsed".
	Bool isNone() const
	{
		return !m_name.isEmpty() && stricmp(m_name.str(), "None") == 0;
	}

	/// Resolve via RefResolver<T>::resolve. Cached on first call. Returns
	/// nullptr if the name is empty, "None", or unknown to the registry.
	const T* resolve() const;

	/// Reset to empty, dropping the cached resolution.
	void clear()
	{
		m_name.clear();
		m_definedAt = SourceLoc();
		m_cached = nullptr;
	}

	/// Set the name + source location. Drops any cached resolution.
	void assign(const AsciiString& name, SourceLoc loc = SourceLoc())
	{
		m_name      = name;
		m_definedAt = loc;
		m_cached    = nullptr;
	}

	/// Implicit conversions for compatibility with code that expects a raw
	/// pointer. Equivalent to resolve(); cached after first call.
	operator const T*() const { return resolve(); }
	const T* operator->() const { return resolve(); }

private:
	AsciiString      m_name;
	SourceLoc        m_definedAt;
	mutable const T* m_cached = nullptr;
};

template <typename T>
inline const T* Ref<T>::resolve() const
{
	if (m_cached != nullptr)
		return m_cached;
	if (m_name.isEmpty())
		return nullptr;
	m_cached = RefResolver<T>::resolve(m_name);
	return m_cached;
}

// ===========================================================================
// parseValue overload for Ref<T>
// ===========================================================================
//
// Reads the next token, stores it as the reference name with the current
// source location, and eagerly resolves so the cached pointer is populated
// when the call returns. Eager resolution matches the legacy parsers,
// which store a resolved pointer at parse time.
//
// This is a function-template overload (not a parseValue<V> specialization)
// so it coexists with the primitive specializations in ParseValue.h. The
// compiler picks it when V deduces to Ref<T> via ordinary overload
// resolution.
template <typename T>
void parseValue(ParseContext& ctx, Ref<T>& out)
{
	const char* token = ctx.nextToken();
	if (token == nullptr)
	{
		out.clear();
		return;
	}
	out.assign(AsciiString(token), ctx.currentLoc());
	(void)out.resolve();  // eager
}

} // namespace INI2
