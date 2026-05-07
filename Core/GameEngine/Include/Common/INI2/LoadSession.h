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

// FILE: LoadSession.h ////////////////////////////////////////////////////////
// Desc:   Replaces the legacy global INI_LOAD_* mode flag with an explicit,
//         testable session value passed by the loader.
//
// The legacy parser threads the load mode through INI::m_loadType, which is
// effectively a per-INI-instance global. Block handlers branch on
//     ini->getLoadType() == INI_LOAD_CREATE_OVERRIDES
// to decide between creating a fresh template and creating an Overridable
// chained on top of an existing one.
//
// In the new system every load is parameterized by a LoadSession value
// passed to INI::loadV2(...). Block handlers branch on
//     ctx.session().createsOverrides()
// instead. Same semantics, but the session is enumerable: code that creates
// overrides can call session.recordOverride(base, entry) and the caller can
// inspect the resulting list — useful for unit tests, debug HUDs, and
// crash diagnostics ("here's everything that will be reverted on map
// unload").
//
// Phase 3 adds the type but does not yet replace the production loader's
// global flag; that switch happens in Phase 4 when BlockRegistry and
// INI::loadV2 land. Until then, a LoadSession is constructible and
// inspectable in unit-test contexts only.
//
// Per-store reset on map unload (TheThingFactory::reset, etc.) continues
// to call Overridable::deleteOverrides() exactly as today; the new
// recordOverride machinery is additive observability, not a replacement
// for the existing chain teardown.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include "Common/AsciiString.h"
#include "Common/GameType.h"

#include "Common/INI2/SourceLoc.h"

// Forward-declared so this header does not pull in the per-game
// Overridable.h. The store-side code that calls recordOverride will already
// have the full type via its own includes.
class Overridable;

namespace INI2
{

enum class LoadMode
{
	/// First-time load of the base game data tree (Data/INI/...). Fresh
	/// templates are created. Duplicate definitions are an error (matches
	/// the legacy DEBUG_CRASH at ThingFactory.cpp:403).
	Initial,

	/// Continuation of an Initial load across multiple files (e.g. inside
	/// a directory walk). Same template-creation semantics as Initial.
	MultiFile,

	/// Per-map override load (map.ini, solo.ini). Existing templates are
	/// left intact; new Overridable entries are chained on top. Reverted
	/// by the existing per-store deleteOverrides() walk on map unload.
	MapOverride,
};

/// One record in a LoadSession's override list. Lifetime caveat: 'base'
/// and 'overrideEntry' are non-owning pointers into engine memory pools.
/// The session does not delete or otherwise manage them; revert is still
/// driven by the per-store reset pipeline. The records are observability
/// only.
struct OverrideRecord
{
	Overridable* base          = nullptr; ///< Template the override applies to.
	Overridable* overrideEntry = nullptr; ///< The newly-created override.
	SourceLoc    loc;                     ///< Where in the INI tree the override was defined.
};

/// Caller-owned session value. Constructed once per logical "load operation"
/// and passed to INI::loadV2 (Phase 4+). Cheap to copy/move at construction
/// time but typically held by reference once construction is done.
class LoadSession
{
public:
	LoadSession() : m_mode(LoadMode::Initial) {}
	explicit LoadSession(LoadMode mode) : m_mode(mode) {}

	LoadMode mode() const { return m_mode; }

	/// Convenience: true iff this session creates Overridable chains rather
	/// than fresh templates. Block handlers branch on this in the new
	/// design (replaces 'ini->getLoadType() == INI_LOAD_CREATE_OVERRIDES').
	Bool createsOverrides() const { return m_mode == LoadMode::MapOverride; }

	/// Called by store-side override creation logic when an override entry
	/// is produced during this session. Phase 3 only collects these; the
	/// per-store reset pipeline still drives the actual revert.
	///
	/// In Phase 7 ThingFactory / WeaponStore / etc. will be threaded with
	/// the current LoadSession so they can call this directly.
	void recordOverride(Overridable* base, Overridable* overrideEntry, SourceLoc loc = {})
	{
		OverrideRecord rec;
		rec.base          = base;
		rec.overrideEntry = overrideEntry;
		rec.loc           = loc;
		m_overrides.push_back(rec);
	}

	/// Enumerate every override created during this session. Empty unless
	/// mode() == MapOverride. The order is creation order.
	const std::vector<OverrideRecord>& overrides() const { return m_overrides; }

	/// Clear the recorded override list. Does not affect engine memory;
	/// only resets this session's bookkeeping. Useful when reusing a
	/// session across multiple files.
	void clearOverrides() { m_overrides.clear(); }

private:
	LoadMode                    m_mode;
	std::vector<OverrideRecord> m_overrides;
};

} // namespace INI2
