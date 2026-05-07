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

// FILE: ParseValue.cpp ///////////////////////////////////////////////////////
// Desc:   Implementations of parseValue<V> for primitive types.
//
// Each implementation mirrors the corresponding INI::parse* function in
// Core/GameEngine/Source/Common/INI/INI.cpp. Where possible we delegate to
// the existing INI::scan* helpers so the underlying numeric parsing — and
// therefore the resulting bit pattern — is shared with the legacy parser.
// This is the primary mechanism by which byte-for-byte compatibility is
// achieved for free; see Core/GameEngine/Tests/INI2 (Phase 1 finalization)
// for the parity tests that enforce it.
///////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"  // must be first

#include <cmath>

#include "Common/INI.h"
#include "Common/GameCommon.h"      // ConvertDuration*, ConvertVelocity*, etc.
#include "GameClient/Color.h"        // Color, GameMakeColor

#include "Common/INI2/Diagnostic.h"
#include "Common/INI2/ParseContext.h"
#include "Common/INI2/ParseValue.h"

namespace INI2
{

// ===========================================================================
// Integer / Real / Bool primitives
// ===========================================================================
// All numeric parsing is delegated to INI::scanInt / scanUnsignedInt /
// scanReal so the new and legacy parsers share the exact same conversion
// (including locale-independence and the std::from_chars fast path on
// C++17+ compilers).

template <>
void parseValue<Byte>(ParseContext& ctx, Byte& out)
{
	const char* token = ctx.nextToken();
	Int value = INI::scanInt(token);
	if (value < 0 || value > 255)
	{
		ctx.emitError(Codes::ValueOutOfRange,
			"value out of range for UnsignedByte (expected 0..255)");
		throw INI_INVALID_DATA;
	}
	out = (Byte)value;
}

template <>
void parseValue<Short>(ParseContext& ctx, Short& out)
{
	const char* token = ctx.nextToken();
	Int value = INI::scanInt(token);
	if (value < -32768 || value > 32767)
	{
		ctx.emitError(Codes::ValueOutOfRange,
			"value out of range for Short (expected -32768..32767)");
		throw INI_INVALID_DATA;
	}
	out = (Short)value;
}

template <>
void parseValue<UnsignedShort>(ParseContext& ctx, UnsignedShort& out)
{
	const char* token = ctx.nextToken();
	Int value = INI::scanInt(token);
	if (value < 0 || value > 65535)
	{
		ctx.emitError(Codes::ValueOutOfRange,
			"value out of range for UnsignedShort (expected 0..65535)");
		throw INI_INVALID_DATA;
	}
	out = (UnsignedShort)value;
}

template <>
void parseValue<Int>(ParseContext& ctx, Int& out)
{
	out = INI::scanInt(ctx.nextToken());
}

template <>
void parseValue<UnsignedInt>(ParseContext& ctx, UnsignedInt& out)
{
	out = INI::scanUnsignedInt(ctx.nextToken());
}

template <>
void parseValue<Real>(ParseContext& ctx, Real& out)
{
	out = INI::scanReal(ctx.nextToken());
}

template <>
void parseValue<Bool>(ParseContext& ctx, Bool& out)
{
	out = INI::scanBool(ctx.nextToken());
}

// ===========================================================================
// String primitives
// ===========================================================================

template <>
void parseValue<AsciiString>(ParseContext& ctx, AsciiString& out)
{
	out = ctx.nextAsciiString();
}

template <>
void parseValue<std::vector<AsciiString> >(ParseContext& ctx, std::vector<AsciiString>& out)
{
	out.clear();
	for (const char* token = ctx.nextTokenOrNull(); token != nullptr; token = ctx.nextTokenOrNull())
	{
		out.push_back(AsciiString(token));
	}
}

void parseQuotedAsciiString(ParseContext& ctx, AsciiString& out)
{
	out = ctx.nextQuotedAsciiString();
}

void parseAsciiStringVectorAppend(ParseContext& ctx, std::vector<AsciiString>& out)
{
	// Intentionally does not clear() — see INI::parseAsciiStringVectorAppend.
	for (const char* token = ctx.nextTokenOrNull(); token != nullptr; token = ctx.nextTokenOrNull())
	{
		out.push_back(AsciiString(token));
	}
}

// ===========================================================================
// Composite primitives (X:val Y:val ..., R:val G:val B:val [A:val])
// ===========================================================================

template <>
void parseValue<Coord3D>(ParseContext& ctx, Coord3D& out)
{
	out.x = INI::scanReal(ctx.nextSubToken("X"));
	out.y = INI::scanReal(ctx.nextSubToken("Y"));
	out.z = INI::scanReal(ctx.nextSubToken("Z"));
}

template <>
void parseValue<Coord2D>(ParseContext& ctx, Coord2D& out)
{
	out.x = INI::scanReal(ctx.nextSubToken("X"));
	out.y = INI::scanReal(ctx.nextSubToken("Y"));
}

template <>
void parseValue<ICoord2D>(ParseContext& ctx, ICoord2D& out)
{
	out.x = INI::scanInt(ctx.nextSubToken("X"));
	out.y = INI::scanInt(ctx.nextSubToken("Y"));
}

template <>
void parseValue<RGBColor>(ParseContext& ctx, RGBColor& out)
{
	const char* names[3] = { "R", "G", "B" };
	Int colors[3];
	for (Int i = 0; i < 3; ++i)
	{
		colors[i] = INI::scanInt(ctx.nextSubToken(names[i]));
		if (colors[i] < 0 || colors[i] > 255)
		{
			ctx.emitError(Codes::ValueOutOfRange,
				"RGBColor channel out of range (expected 0..255)");
			throw INI_INVALID_DATA;
		}
	}
	out.red   = (Real)colors[0] / 255.0f;
	out.green = (Real)colors[1] / 255.0f;
	out.blue  = (Real)colors[2] / 255.0f;
}

namespace
{
	// Shared between parseValue<RGBAColorInt> and parseColorInt: the legacy
	// parsers use identical lexical layout (R:val G:val B:val [A:val] with
	// SepsColon as the token separator) and identical validation rules, but
	// store into different destination types.
	void scanRGBAColorComponents(ParseContext& ctx, Int (&colors)[4])
	{
		const char* names[4] = { "R", "G", "B", "A" };
		for (Int i = 0; i < 4; ++i)
		{
			const char* token = ctx.nextTokenOrNull(INI::getSepsColon());
			if (token == nullptr)
			{
				if (i < 3)
				{
					ctx.emitError(Codes::MissingToken,
						"RGBA color is missing required component");
					throw INI_INVALID_DATA;
				}
				// A is optional; legacy default is fully opaque.
				colors[i] = 255;
			}
			else
			{
				if (stricmp(token, names[i]) != 0)
				{
					ctx.emitError(Codes::UnexpectedToken,
						"RGBA color component label did not match expected R/G/B/A");
					throw INI_INVALID_DATA;
				}
				colors[i] = INI::scanInt(ctx.nextToken(INI::getSepsColon()));
			}
			if (colors[i] < 0 || colors[i] > 255)
			{
				ctx.emitError(Codes::ValueOutOfRange,
					"RGBA channel out of range (expected 0..255)");
				throw INI_INVALID_DATA;
			}
		}
	}
}

template <>
void parseValue<RGBAColorInt>(ParseContext& ctx, RGBAColorInt& out)
{
	Int colors[4];
	scanRGBAColorComponents(ctx, colors);
	out.red   = colors[0];
	out.green = colors[1];
	out.blue  = colors[2];
	out.alpha = colors[3];
}

void parseColorInt(ParseContext& ctx, Int& out)
{
	Int colors[4];
	scanRGBAColorComponents(ctx, colors);
	out = (Int)GameMakeColor(colors[0], colors[1], colors[2], colors[3]);
}

// ===========================================================================
// Unit-converting parsers
// ===========================================================================
// Each delegates to the same Convert*FromMsecsToFrames / ConvertVelocity* /
// ConvertAcceleration* / ConvertAngularVelocity* helpers used by the legacy
// parser, so the resulting bit patterns are identical.

void parseAngleDegrees(ParseContext& ctx, Real& out)
{
	const Real RADS_PER_DEGREE = PI / 180.0f;
	out = INI::scanReal(ctx.nextToken()) * RADS_PER_DEGREE;
}

void parseAngularVelocityDegPerSec(ParseContext& ctx, Real& out)
{
	out = ConvertAngularVelocityInDegreesPerSecToRadsPerFrame(
		INI::scanReal(ctx.nextToken()));
}

void parseVelocityDistPerSec(ParseContext& ctx, Real& out)
{
	out = ConvertVelocityInSecsToFrames(INI::scanReal(ctx.nextToken()));
}

void parseAccelerationDistPerSec2(ParseContext& ctx, Real& out)
{
	out = ConvertAccelerationInSecsToFrames(INI::scanReal(ctx.nextToken()));
}

void parseDurationMsec(ParseContext& ctx, Real& out)
{
	Real msec = INI::scanReal(ctx.nextToken());
	out = ConvertDurationFromMsecsToFrames(msec);
}

void parseDurationMsecToUInt(ParseContext& ctx, UnsignedInt& out)
{
	UnsignedInt msec = INI::scanUnsignedInt(ctx.nextToken());
	out = (UnsignedInt)ceilf(ConvertDurationFromMsecsToFrames((Real)msec));
}

void parseDurationMsecToUShort(ParseContext& ctx, UnsignedShort& out)
{
	UnsignedInt msec = INI::scanUnsignedInt(ctx.nextToken());
	out = (UnsignedShort)ceilf(ConvertDurationFromMsecsToFrames((Real)msec));
}

void parsePositiveNonZeroReal(ParseContext& ctx, Real& out)
{
	out = INI::scanReal(ctx.nextToken());
	if (out <= 0.0f)
	{
		ctx.emitError(Codes::ValueOutOfRange,
			"value must be > 0");
		throw INI_INVALID_DATA;
	}
}

void parsePercentToReal(ParseContext& ctx, Real& out)
{
	const char* token = ctx.nextToken(INI::getSepsPercent());
	out = INI::scanPercentToReal(token);
}

} // namespace INI2
