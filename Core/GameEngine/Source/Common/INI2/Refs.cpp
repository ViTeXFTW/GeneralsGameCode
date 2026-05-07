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

// FILE: Refs.cpp /////////////////////////////////////////////////////////////
// Desc:   Out-of-line definitions of RefResolver<T>::resolve / typeName
//         for the nine engine cross-INI reference types.
//
// Each resolve() mirrors the legacy INI::parse*Template behavior in
// Core/Source/Common/INI/INI.cpp:
//   - "None" (case-insensitive)  -> nullptr
//   - Unknown name               -> nullptr (legacy is DEBUG_ASSERTCRASH
//                                   which is non-fatal in release builds)
//   - Null global store          -> DEBUG_CRASH + nullptr (legacy throws
//                                   ERROR_BUG; we soften to nullptr because
//                                   resolve() is also called lazily by
//                                   Ref<T>::operator-> which must not throw)
///////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"  // must be first

#include "Common/INI2/Refs.h"

#include "Common/DamageFX.h"
#include "Common/SpecialPower.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"

#include "GameClient/FXList.h"
#include "GameClient/ParticleSys.h"

#include "GameLogic/Armor.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Weapon.h"

namespace INI2
{

namespace
{
	/// Common pre-flight: returns true if the name is the "None"
	/// placeholder (case-insensitive) or empty, in which case the caller
	/// should return nullptr immediately.
	inline Bool isNoneOrEmpty(const AsciiString& name)
	{
		return name.isEmpty() || stricmp(name.str(), "None") == 0;
	}
}

// ---------------------------------------------------------------------------
// ArmorTemplate
// ---------------------------------------------------------------------------
const ArmorTemplate* RefResolver<ArmorTemplate>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheArmorStore == nullptr) return nullptr;
	return TheArmorStore->findArmorTemplate(name.str());
}
const char* RefResolver<ArmorTemplate>::typeName() { return "Armor"; }

// ---------------------------------------------------------------------------
// DamageFX
// ---------------------------------------------------------------------------
const DamageFX* RefResolver<DamageFX>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheDamageFXStore == nullptr) return nullptr;
	return TheDamageFXStore->findDamageFX(name.str());
}
const char* RefResolver<DamageFX>::typeName() { return "DamageFX"; }

// ---------------------------------------------------------------------------
// FXList
// ---------------------------------------------------------------------------
const FXList* RefResolver<FXList>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheFXListStore == nullptr) return nullptr;
	return TheFXListStore->findFXList(name.str());
}
const char* RefResolver<FXList>::typeName() { return "FXList"; }

// ---------------------------------------------------------------------------
// ObjectCreationList
// ---------------------------------------------------------------------------
const ObjectCreationList* RefResolver<ObjectCreationList>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheObjectCreationListStore == nullptr) return nullptr;
	return TheObjectCreationListStore->findObjectCreationList(name.str());
}
const char* RefResolver<ObjectCreationList>::typeName() { return "OCL"; }

// ---------------------------------------------------------------------------
// ParticleSystemTemplate
// ---------------------------------------------------------------------------
const ParticleSystemTemplate* RefResolver<ParticleSystemTemplate>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheParticleSystemManager == nullptr) return nullptr;
	return TheParticleSystemManager->findTemplate(name);
}
const char* RefResolver<ParticleSystemTemplate>::typeName() { return "ParticleSystem"; }

// ---------------------------------------------------------------------------
// SpecialPowerTemplate
// ---------------------------------------------------------------------------
const SpecialPowerTemplate* RefResolver<SpecialPowerTemplate>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheSpecialPowerStore == nullptr)
	{
		DEBUG_CRASH(("INI2::RefResolver<SpecialPowerTemplate>: TheSpecialPowerStore not inited yet"));
		return nullptr;
	}
	return TheSpecialPowerStore->findSpecialPowerTemplate(name);
}
const char* RefResolver<SpecialPowerTemplate>::typeName() { return "SpecialPower"; }

// ---------------------------------------------------------------------------
// ThingTemplate
// ---------------------------------------------------------------------------
const ThingTemplate* RefResolver<ThingTemplate>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheThingFactory == nullptr)
	{
		DEBUG_CRASH(("INI2::RefResolver<ThingTemplate>: TheThingFactory not inited yet"));
		return nullptr;
	}
	return TheThingFactory->findTemplate(name.str());
}
const char* RefResolver<ThingTemplate>::typeName() { return "Object"; }

// ---------------------------------------------------------------------------
// UpgradeTemplate
// ---------------------------------------------------------------------------
const UpgradeTemplate* RefResolver<UpgradeTemplate>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheUpgradeCenter == nullptr)
	{
		DEBUG_CRASH(("INI2::RefResolver<UpgradeTemplate>: TheUpgradeCenter not inited yet"));
		return nullptr;
	}
	return TheUpgradeCenter->findUpgrade(name.str());
}
const char* RefResolver<UpgradeTemplate>::typeName() { return "Upgrade"; }

// ---------------------------------------------------------------------------
// WeaponTemplate
// ---------------------------------------------------------------------------
const WeaponTemplate* RefResolver<WeaponTemplate>::resolve(const AsciiString& name)
{
	if (isNoneOrEmpty(name)) return nullptr;
	if (TheWeaponStore == nullptr) return nullptr;
	return TheWeaponStore->findWeaponTemplate(name.str());
}
const char* RefResolver<WeaponTemplate>::typeName() { return "Weapon"; }

} // namespace INI2
