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

// FILE: ParseSession.h ///////////////////////////////////////////////////////
// Desc:   File-scoped owner of the diagnostic sink and the active LoadSession
//         link, for the duration of one INI file load.
//
// Layering recap:
//   - ParseContext is per-call: lightweight, stack-resident, holds non-
//     owning pointers to INI* and DiagnosticSink. Constructed by the
//     legacy trampoline for every field parsed.
//   - ParseSession is per-file: lives for the duration of one INI::loadV2
//     call, owns the diagnostic sink (or borrows a caller-supplied one),
//     and installs a CurrentSinkGuard so the trampoline can find the sink.
//   - LoadSession is per-load-operation: carries the mode flag and any
//     accumulated override records.
//
// Phase 3 ships this type but does not yet hook it up in the production
// loader. INI::load and friends still take INILoadType. The Phase 4 loader
// (INI::loadV2) will construct a ParseSession at the top of each file and
// hand it down through the existing INI reader.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/AsciiString.h"
#include "Common/GameType.h"

#include "Common/INI2/Diagnostic.h"
#include "Common/INI2/LegacyAdapter.h"
#include "Common/INI2/LoadSession.h"

class INI;

namespace INI2
{

/// Owns the per-file parsing state. RAII over the diagnostic-sink slot:
/// installs a CurrentSinkGuard at construction so the legacy trampoline can
/// emit into our sink, and the guard restores the previous sink on
/// destruction.
class ParseSession
{
public:
	/// Construct with an internal CollectingSink. Callers can query
	/// diagnostics() / hasErrors() afterwards. Most unit tests use this.
	ParseSession(INI* ini, LoadSession* load)
		: m_ini(ini)
		, m_load(load)
		, m_externalSink(nullptr)
		, m_internalSink()
		, m_sinkGuard(&m_internalSink)
	{}

	/// Construct with a caller-supplied sink. Production paths that want
	/// log-and-throw behavior can pass a sink that escalates Severity::Fatal
	/// to a throw so legacy DEBUG_CRASH semantics are preserved.
	ParseSession(INI* ini, LoadSession* load, DiagnosticSink* externalSink)
		: m_ini(ini)
		, m_load(load)
		, m_externalSink(externalSink)
		, m_internalSink()
		, m_sinkGuard(externalSink ? externalSink : &m_internalSink)
	{}

	// Non-copyable, non-movable: the CurrentSinkGuard member would be
	// invalidated by either operation, and a session is intended to be
	// stack-allocated for the lifetime of one file.
	ParseSession(const ParseSession&) = delete;
	ParseSession& operator=(const ParseSession&) = delete;

	INI*            ini()         const { return m_ini; }
	LoadSession*    loadSession() const { return m_load; }

	/// The active sink — external if supplied, else the internal collector.
	DiagnosticSink* sink()        { return m_externalSink ? m_externalSink : &m_internalSink; }

	/// Convenience accessors when using the internal sink. Returns the
	/// records collected during this session. If an external sink was
	/// supplied these will be empty (the records went to the external sink).
	const std::vector<Diagnostic>& diagnostics() const { return m_internalSink.records(); }

	Bool hasErrors() const { return m_internalSink.hasErrors(); }

private:
	INI*            m_ini;
	LoadSession*    m_load;
	DiagnosticSink* m_externalSink;   ///< Non-owning. Null when using internal.
	CollectingSink  m_internalSink;
	CurrentSinkGuard m_sinkGuard;     ///< Must be last: depends on m_internalSink being initialized.
};

} // namespace INI2
