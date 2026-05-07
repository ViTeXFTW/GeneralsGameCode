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

// FILE: ParseContext.h ///////////////////////////////////////////////////////
// Desc:   Per-call context handed to INI2 parseValue functions.
//
// Wraps the legacy INI lexer so we don't reimplement tokenization, comment
// stripping, tab handling, or Xfer save-CRC plumbing — those continue to be
// exercised by the existing INI::readLine / INI::getNextToken pipeline. The
// new parser only changes how parsed values reach their destination.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/AsciiString.h"
#include "Common/GameType.h"
#include "Common/INI.h"

#include "Common/INI2/Diagnostic.h"
#include "Common/INI2/SourceLoc.h"

namespace INI2
{

class LoadSession;  // forward; defined in LoadSession.h

/// Context handed to every parseValue<V> call.
///
/// Owns nothing; holds non-owning pointers to the underlying INI reader,
/// the DiagnosticSink, and (optionally) the active LoadSession. Cheap to
/// copy/pass by reference.
class ParseContext
{
public:
	ParseContext(INI* ini, DiagnosticSink* sink, LoadSession* load = nullptr)
		: m_ini(ini), m_sink(sink), m_load(load) {}

	// --- Token access ------------------------------------------------------
	// These delegate to the existing INI lexer so behavior — including the
	// '=' / whitespace separators, ';' comment handling, and the Xfer hash
	// stream — is byte-identical to the legacy parser.

	/// Read the next token from the current line. Throws INI_INVALID_DATA
	/// (preserving legacy behavior) if no token is present. For recoverable
	/// failure use nextTokenOrNull() and emit a diagnostic.
	const char* nextToken() { return m_ini->getNextToken(); }
	const char* nextToken(const char* seps) { return m_ini->getNextToken(seps); }

	/// Read the next token, returning nullptr at end-of-line.
	const char* nextTokenOrNull() { return m_ini->getNextTokenOrNull(); }
	const char* nextTokenOrNull(const char* seps) { return m_ini->getNextTokenOrNull(seps); }

	/// Read a "Tag:value" pair, requiring 'expected' as the tag. Used by
	/// composite parsers (Coord3D, RGBColor, RGBAColorInt).
	const char* nextSubToken(const char* expected) { return INI::getNextSubToken(expected); }

	/// Read the next AsciiString, supporting quoted strings.
	AsciiString nextAsciiString() { return m_ini->getNextAsciiString(); }

	/// Read the next AsciiString with the more permissive quoted-string
	/// handling that parseQuotedAsciiString uses.
	AsciiString nextQuotedAsciiString() { return m_ini->getNextQuotedAsciiString(); }

	// --- Source location ---------------------------------------------------

	SourceLoc currentLoc() const
	{
		return SourceLoc(m_ini->getFilename(), m_ini->getLineNum());
	}

	// --- Diagnostics -------------------------------------------------------

	void emit(const Diagnostic& d)
	{
		if (m_sink) m_sink->emit(d);
	}

	/// Convenience: emit an error with the current source location filled in.
	void emitError(DiagCode code, const AsciiString& message)
	{
		Diagnostic d;
		d.severity = Severity::Error;
		d.code = code;
		d.loc = currentLoc();
		d.message = message;
		emit(d);
	}

	// --- Underlying reader (escape hatch for the adapter shim) -------------

	INI* legacy() { return m_ini; }
	const INI* legacy() const { return m_ini; }

	DiagnosticSink* sink() { return m_sink; }

	/// The active LoadSession, or nullptr if the legacy global mode flag is
	/// still in effect. Block handlers and store-side override creators
	/// branch on loadSession() != nullptr && loadSession()->createsOverrides()
	/// in the new design, replacing 'ini->getLoadType() == INI_LOAD_CREATE_OVERRIDES'.
	LoadSession*       loadSession()       { return m_load; }
	const LoadSession* loadSession() const { return m_load; }

private:
	INI*            m_ini;
	DiagnosticSink* m_sink;
	LoadSession*    m_load;   ///< Optional; nullptr when called from the legacy loader path.
};

} // namespace INI2
