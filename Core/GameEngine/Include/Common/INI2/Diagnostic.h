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

// FILE: Diagnostic.h /////////////////////////////////////////////////////////
// Desc:   Structured diagnostic record produced by the INI2 parser.
//
// Replaces the legacy parser's DEBUG_CRASH-on-bad-input behavior with a
// data-driven record that downstream consumers can choose how to handle:
//   - The shipping engine wires a sink that logs and (for fatal severity)
//     throws, preserving today's behavior.
//   - Tools, modders' validators, and language servers wire a sink that just
//     collects the records.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include "Common/AsciiString.h"
#include "Common/GameType.h"

#include "Common/INI2/SourceLoc.h"

namespace INI2
{

enum class Severity : Int
{
	Hint    = 0,
	Info    = 1,
	Warning = 2,
	Error   = 3,
	Fatal   = 4,
};

/// Stable string code for the diagnostic class. Keep these strings short and
/// machine-greppable; downstream tooling will key off them.
///
/// Codes follow the form "INI" + four-digit number.
struct DiagCode
{
	const char* value;

	constexpr DiagCode(const char* v) : value(v) {}

	Bool operator==(const DiagCode& other) const
	{
		// Pointer equality is sufficient because all codes are string literals
		// from this header (interned by the compiler).
		return value == other.value;
	}
};

namespace Codes
{
	constexpr DiagCode UnknownBlock          {"INI0001"};
	constexpr DiagCode UnknownField          {"INI0002"};
	constexpr DiagCode MissingEndToken       {"INI0003"};
	constexpr DiagCode DuplicateDefinition   {"INI0004"};
	constexpr DiagCode InvalidValue          {"INI0005"};
	constexpr DiagCode ValueOutOfRange       {"INI0006"};
	constexpr DiagCode UnknownEnumValue      {"INI0007"};
	constexpr DiagCode UnknownReference      {"INI0008"};
	constexpr DiagCode MissingToken          {"INI0009"};
	constexpr DiagCode UnexpectedToken       {"INI0010"};
	constexpr DiagCode FileNotFound          {"INI0011"};
	constexpr DiagCode InternalError         {"INI0099"};
}

/// One diagnostic record. Cheap to copy; held by value in DiagnosticSink.
struct Diagnostic
{
	Severity     severity = Severity::Error;
	DiagCode     code     = Codes::InternalError;
	SourceLoc    loc;
	AsciiString  blockKeyword;  ///< e.g. "Object" — empty if not yet known
	AsciiString  blockInstance; ///< e.g. "AmericaTankCrusader"
	AsciiString  field;         ///< the field token if applicable
	AsciiString  message;       ///< human-readable text
};

/// Consumer interface for diagnostics. The parser never crashes directly;
/// it forwards every recoverable issue here. Sinks decide what to do.
class DiagnosticSink
{
public:
	virtual ~DiagnosticSink() = default;

	/// Called for every diagnostic produced. May throw to abort parsing
	/// (e.g. the throwing-sink mimics legacy DEBUG_CRASH behavior on Fatal).
	virtual void emit(const Diagnostic& d) = 0;
};

/// Default sink: collect every diagnostic in a vector. Useful for tests,
/// validators, and the language-server schema-consumer pipeline.
class CollectingSink : public DiagnosticSink
{
public:
	void emit(const Diagnostic& d) override { m_records.push_back(d); }

	const std::vector<Diagnostic>& records() const { return m_records; }
	std::vector<Diagnostic>& records() { return m_records; }
	void clear() { m_records.clear(); }

	Bool hasErrors() const
	{
		for (const Diagnostic& d : m_records)
		{
			if (d.severity >= Severity::Error)
				return TRUE;
		}
		return FALSE;
	}

private:
	std::vector<Diagnostic> m_records;
};

} // namespace INI2
