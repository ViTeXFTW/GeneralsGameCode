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

// FILE: LegacyAdapter.h //////////////////////////////////////////////////////
// Desc:   Bridge between the new Schema/Field design and the legacy
//         FieldParse[] / INIFieldParseProc interface.
//
// The legacy parser (INI::initFromINIMulti at INI.cpp:1497) walks lines and
// dispatches each field through a FieldParse row that points at an
// INIFieldParseProc with signature
//     void (*)(INI*, void* instance, void* store, const void* userData)
//
// To let migrated classes plug in to the legacy parser without changing it,
// every Schema<T> emits a FieldParse[] in which:
//   - parse    = legacyTrampoline<T>
//   - userData = the FieldBase<T>* for that field
//   - offset   = field's byte offset within T (already includes any extra
//                offset from inherited tables)
//   - token    = the field's token string (literal lifetime)
//
// At parse time the legacy code calls
//     legacyTrampoline<T>(ini, what, store, userData);
// The trampoline reconstitutes a ParseContext from the INI* and forwards to
// the typed field's virtual parse() method.
//
// The current-sink slot is a thread_local pointer that LoadSession (Phase 3)
// will populate. In Phase 2 it remains nullptr and parseValue calls fall
// through with a null sink — diagnostics are silently dropped, but throws
// (matching legacy DEBUG_CRASH-then-throw behavior) still propagate.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/INI.h"

#include "Common/INI2/Diagnostic.h"
#include "Common/INI2/Field.h"
#include "Common/INI2/ParseContext.h"

namespace INI2
{

namespace detail
{

	/// The current diagnostic sink. Set by LoadSession at the start of a
	/// load and cleared at the end. Read by legacyTrampoline. Thread-local
	/// so unit tests and parallel tools can set their own without races.
	inline DiagnosticSink*& currentSink()
	{
		thread_local DiagnosticSink* slot = nullptr;
		return slot;
	}

} // namespace detail

/// Scoped guard for setting the current sink for the lifetime of a load.
/// LoadSession will RAII-own one of these in Phase 3.
class CurrentSinkGuard
{
public:
	explicit CurrentSinkGuard(DiagnosticSink* sink)
		: m_previous(detail::currentSink())
	{
		detail::currentSink() = sink;
	}

	~CurrentSinkGuard()
	{
		detail::currentSink() = m_previous;
	}

	CurrentSinkGuard(const CurrentSinkGuard&) = delete;
	CurrentSinkGuard& operator=(const CurrentSinkGuard&) = delete;

private:
	DiagnosticSink* m_previous;
};

/// Trampoline called by the legacy parser. One instantiation per owning
/// class T. Both inline and template, so the address is a weak symbol —
/// stable across translation units, which the legacy parser requires (it
/// holds the function pointer in static FieldParse rows).
template <typename T>
void legacyTrampoline(INI* ini, void* /*instance*/, void* store, const void* userData)
{
	const FieldBase<T>* field = static_cast<const FieldBase<T>*>(userData);
	ParseContext ctx(ini, detail::currentSink());
	field->parse(ctx, store);
}

} // namespace INI2
