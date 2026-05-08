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

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: Water.cpp ////////////////////////////////////////////////////////////////////////////////
// Author: Colin Day, December 2001
// Desc:   Map water settings
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameClient/Water.h"
#include "Common/INI.h"
#include "Common/INIException.h"

#include "Common/INI2/BlockRegistry.h"
#include "Common/INI2/ParseSession.h"
#include "Common/INI2/Schema.h"

// GLOBALS ////////////////////////////////////////////////////////////////////////////////////////
WaterSetting WaterSettings[ TIME_OF_DAY_COUNT ];
OVERRIDE<WaterTransparencySetting> TheWaterTransparency = nullptr;

// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
//
// Phase 6 of the INI2 migration: the WaterSetting field list is now declared
// as a typed Schema<WaterSetting> rather than the legacy hand-typed
// FieldParse[] array. The schema lazily emits an equivalent legacy
// FieldParse[] (matching offsets, member-pointer-derived; matching tokens;
// matching parsers via the legacyTrampoline<WaterSetting> bridge), so the
// existing INI::initFromINI driver consumes it unchanged. Behavior is
// byte-for-byte identical to the prior table.
//
INI2::Schema<WaterSetting>& WaterSetting::schema()
{
	static INI2::Schema<WaterSetting> s = []{
		INI2::Schema<WaterSetting> schema;
		schema.add("SkyTexture",              &WaterSetting::m_skyTextureFile);
		schema.add("WaterTexture",            &WaterSetting::m_waterTextureFile);
		schema.add("Vertex00Color",           &WaterSetting::m_vertex00Diffuse);
		schema.add("Vertex10Color",           &WaterSetting::m_vertex10Diffuse);
		schema.add("Vertex01Color",           &WaterSetting::m_vertex01Diffuse);
		schema.add("Vertex11Color",           &WaterSetting::m_vertex11Diffuse);
		schema.add("DiffuseColor",            &WaterSetting::m_waterDiffuseColor);
		schema.add("TransparentDiffuseColor", &WaterSetting::m_transparentWaterDiffuse);
		schema.add("UScrollPerMS",            &WaterSetting::m_uScrollPerMs);
		schema.add("VScrollPerMS",            &WaterSetting::m_vScrollPerMs);
		schema.add("SkyTexelsPerUnit",        &WaterSetting::m_skyTexelsPerUnit);
		schema.add("WaterRepeatCount",        &WaterSetting::m_waterRepeatCount);
		return schema;
	}();
	return s;
}

const FieldParse* WaterSetting::getFieldParse() const
{
	return schema().asLegacyFieldParse();
}

// Block handler: registered via INI2_REGISTER_BLOCK below. Replaces the
// legacy INI::parseWaterSettingDefinition (formerly in INIWater.cpp). The
// dispatch logic is unchanged: look up the time-of-day index by name,
// then walk the body via initFromINI against the schema-emitted table.
namespace
{
	void parseWaterSettingDefinitionV2(INI2::ParseSession& session)
	{
		INI* ini = session.ini();
		AsciiString name(ini->getNextToken());

		// Find the matching time-of-day index.
		WaterSetting* setting = nullptr;
		const char* const* todName = TimeOfDayNames;
		Int todIdx = 0;
		while (todName != nullptr && *todName != nullptr)
		{
			if (stricmp(*todName, name.str()) == 0)
			{
				setting = &WaterSettings[todIdx];
				break;
			}
			++todName;
			++todIdx;
		}

		if (setting == nullptr)
		{
			DEBUG_CRASH(("[LINE: %d - FILE: '%s'] WaterSet name '%s' is not a valid time-of-day",
				ini->getLineNum(), ini->getFilename().str(), name.str()));
			throw INI_INVALID_DATA;
		}

		ini->initFromINI(setting, WaterSetting::schema().asLegacyFieldParse());
	}

	INI2_REGISTER_BLOCK("WaterSet", &parseWaterSettingDefinitionV2);
}

// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
const FieldParse WaterTransparencySetting::m_waterTransparencySettingFieldParseTable[] =
{

	{ "TransparentWaterDepth",			INI::parseReal,				nullptr,			offsetof( WaterTransparencySetting, m_transparentWaterDepth ) },
	{ "TransparentWaterMinOpacity",	INI::parseReal,				nullptr,			offsetof( WaterTransparencySetting, m_minWaterOpacity ) },
	{ "StandingWaterColor",	INI::parseRGBColor,			nullptr,			offsetof( WaterTransparencySetting, m_standingWaterColor ) },
	{ "StandingWaterTexture",INI::parseAsciiString,		nullptr,			offsetof( WaterTransparencySetting, m_standingWaterTexture ) },
	{ "AdditiveBlending", INI::parseBool,				nullptr,			offsetof( WaterTransparencySetting, m_additiveBlend) },
	{ "RadarWaterColor", INI::parseRGBColor,			nullptr,			offsetof( WaterTransparencySetting, m_radarColor) },
	{ "SkyboxTextureN",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencySetting, m_skyboxTextureN ) },
	{ "SkyboxTextureE",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencySetting, m_skyboxTextureE ) },
	{ "SkyboxTextureS",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencySetting, m_skyboxTextureS ) },
	{ "SkyboxTextureW",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencySetting, m_skyboxTextureW ) },
	{ "SkyboxTextureT",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencySetting, m_skyboxTextureT ) },


	{ nullptr, nullptr, nullptr, 0 },
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WaterSetting::WaterSetting()
{

	m_skyTextureFile.clear();
	m_waterTextureFile.clear();
	m_waterRepeatCount = 0;
	m_skyTexelsPerUnit = 0.0f;

	m_vertex00Diffuse.red = 0;
	m_vertex00Diffuse.green = 0;
	m_vertex00Diffuse.blue = 0;
	m_vertex00Diffuse.alpha = 0;

	m_vertex01Diffuse.red = 0;
	m_vertex01Diffuse.green = 0;
	m_vertex01Diffuse.blue = 0;
	m_vertex01Diffuse.alpha = 0;

	m_vertex10Diffuse.red = 0;
	m_vertex10Diffuse.green = 0;
	m_vertex10Diffuse.blue = 0;
	m_vertex10Diffuse.alpha = 0;

	m_vertex11Diffuse.red = 0;
	m_vertex11Diffuse.green = 0;
	m_vertex11Diffuse.blue = 0;
	m_vertex11Diffuse.alpha = 0;

	m_waterDiffuseColor.red = 0;
	m_waterDiffuseColor.green = 0;
	m_waterDiffuseColor.blue = 0;
	m_waterDiffuseColor.alpha = 0;

	m_transparentWaterDiffuse.red = 0;
	m_transparentWaterDiffuse.green = 0;
	m_transparentWaterDiffuse.blue = 0;
	m_transparentWaterDiffuse.alpha = 0;

	m_uScrollPerMs = 0.0f;
	m_vScrollPerMs = 0.0f;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WaterSetting::~WaterSetting()
{

}


