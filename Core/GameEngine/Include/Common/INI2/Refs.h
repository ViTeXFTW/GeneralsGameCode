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

// FILE: Refs.h ///////////////////////////////////////////////////////////////
// Desc:   RefResolver<T> specializations for the engine's nine cross-INI
//         reference types.
//
// Forward-declares each engine type and declares its RefResolver<T>
// specialization. Definitions live in Refs.cpp where the corresponding
// store header (Common/Thing/ThingFactory.h, GameLogic/Weapon.h, etc.) is
// available.
//
// Including this header gives you Ref<WeaponTemplate>, Ref<ArmorTemplate>,
// etc., without pulling the per-store implementation headers into every
// consumer.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/INI2/Ref.h"

// Forward declarations for the engine reference types. The full types are
// only required by Refs.cpp, not by callers who hold Ref<T>s.
class ArmorTemplate;
class DamageFX;
class FXList;
class ObjectCreationList;
class ParticleSystemTemplate;
class SpecialPowerTemplate;
class ThingTemplate;
class UpgradeTemplate;
class WeaponTemplate;

namespace INI2
{

// One specialization per engine reference type. Mirrors the legacy
// INI::parse*Template behavior exactly:
//   - Returns nullptr for the special name "None" (case-insensitive).
//   - Returns nullptr for unknown names (matching legacy DEBUG_ASSERTCRASH
//     which is a non-fatal warning in release builds).
//   - Logs a DEBUG_CRASH if the corresponding global store is null at the
//     time of resolution (mirrors the few legacy parsers that guard).
//
// typeName() returns the schema-friendly type label used by Phase 12
// schema export and by diagnostic messages.

template <> struct RefResolver<ArmorTemplate> {
	static const ArmorTemplate* resolve(const AsciiString& name);
	static const char* typeName();
};

template <> struct RefResolver<DamageFX> {
	static const DamageFX* resolve(const AsciiString& name);
	static const char* typeName();
};

template <> struct RefResolver<FXList> {
	static const FXList* resolve(const AsciiString& name);
	static const char* typeName();
};

template <> struct RefResolver<ObjectCreationList> {
	static const ObjectCreationList* resolve(const AsciiString& name);
	static const char* typeName();
};

template <> struct RefResolver<ParticleSystemTemplate> {
	static const ParticleSystemTemplate* resolve(const AsciiString& name);
	static const char* typeName();
};

template <> struct RefResolver<SpecialPowerTemplate> {
	static const SpecialPowerTemplate* resolve(const AsciiString& name);
	static const char* typeName();
};

template <> struct RefResolver<ThingTemplate> {
	static const ThingTemplate* resolve(const AsciiString& name);
	static const char* typeName();
};

template <> struct RefResolver<UpgradeTemplate> {
	static const UpgradeTemplate* resolve(const AsciiString& name);
	static const char* typeName();
};

template <> struct RefResolver<WeaponTemplate> {
	static const WeaponTemplate* resolve(const AsciiString& name);
	static const char* typeName();
};

} // namespace INI2
