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

// FILE: Schema.h /////////////////////////////////////////////////////////////
// Desc:   Per-class schema: an ordered, named list of typed fields.
//
// A Schema<T> replaces the legacy hand-typed
//     static const FieldParse T::s_fooFieldParseTable[] = {
//         { "FieldName", INI::parseReal, nullptr, offsetof(T, m_x) },
//         ...
//     };
// pattern with a typed builder that records V T::* member pointers and
// a Unit tag, so the C++ compiler enforces the field-to-member binding.
//
// During the migration a Schema<T> can be exported as a legacy FieldParse[]
// via asLegacyFieldParse(), so already-migrated classes plug into the
// existing INI::initFromINI driver without source-side caller changes.
//
// inherits() chains a legacy parent table into this schema, mirroring
// MultiIniFieldParse's semantics: lookup walks own fields first (in
// declaration order), then each inherited table in inherits() order, with
// first-match-wins. The extraOffset parameter handles by-value-embedded
// parent objects (today's MultiIniFieldParse::add(p, extraOffset)).
//
// Schemas are intended to be constructed once at static-init time (e.g.,
// inside a function-local static getter) and never modified afterwards.
// asLegacyFieldParse() lazily builds a flat FieldParse[] on the first call
// and caches it; subsequent add()/inherits() after that point will trip
// an assertion.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <vector>

#include "Common/AsciiString.h"
#include "Common/GameType.h"
#include "Common/INI.h"

#include "Common/INI2/Field.h"
#include "Common/INI2/LegacyAdapter.h"
#include "Common/INI2/ValueTypes.h"

namespace INI2
{

template <typename T>
class Schema
{
public:
	Schema() = default;

	// Schemas are typically static singletons; copying would re-emit
	// FieldParse arrays at different addresses and invalidate cached
	// pointers held by the legacy parser.
	Schema(const Schema&) = delete;
	Schema& operator=(const Schema&) = delete;
	Schema(Schema&&) = default;
	Schema& operator=(Schema&&) = default;

	// ---------------------------------------------------------------------
	// Builder API
	// ---------------------------------------------------------------------

	/// Add a typed field. The compiler verifies that `member` is a member of
	/// T with type V. Returns *this for fluent chaining.
	template <typename V>
	Schema& add(const char* token, V T::* member)
	{
		assertNotFinalized();
		auto field = std::make_unique<Field<T, V> >(token, member);
		m_fields.push_back(std::move(field));
		return *this;
	}

	/// Add a typed field with a Unit tag. The Unit selects the matching
	/// unit-aware parser at parse time (e.g., Unit::Degrees -> radians).
	template <typename V>
	Schema& add(const char* token, V T::* member, Unit unit)
	{
		assertNotFinalized();
		auto field = std::make_unique<Field<T, V> >(token, member, unit);
		m_fields.push_back(std::move(field));
		return *this;
	}

	/// Set documentation on the most recently added field. Used by Phase 12
	/// schema export.
	Schema& doc(const char* documentation)
	{
		assertNotFinalized();
		if (!m_fields.empty())
			m_fields.back()->doc = documentation;
		return *this;
	}

	/// Set flag bits on the most recently added field. See FieldFlag.
	Schema& flags(UnsignedInt bits)
	{
		assertNotFinalized();
		if (!m_fields.empty())
			m_fields.back()->flags |= bits;
		return *this;
	}

	/// Inherit from a legacy FieldParse[] table. Used during migration so
	/// a child class can carry its own typed schema while the parent class
	/// still publishes a hand-typed legacy table — and vice versa, since
	/// asLegacyFieldParse() emits a legacy table that can be consumed here.
	///
	/// `extraOffset` is added to each inherited row's offset, mirroring
	/// MultiIniFieldParse::add(p, extraOffset). Use 0 for ordinary public-
	/// inheritance composition; nonzero for a by-value embedded base.
	Schema& inherits(const FieldParse* parentTable, UnsignedInt extraOffset = 0)
	{
		assertNotFinalized();
		Inherited entry;
		entry.table       = parentTable;
		entry.extraOffset = extraOffset;
		m_inherited.push_back(entry);
		return *this;
	}

	// ---------------------------------------------------------------------
	// Query API (used by BlockRegistry / ModuleRegistryV2 in later phases)
	// ---------------------------------------------------------------------

	/// Look up a field by token. Walks own fields first (declaration order),
	/// then inherited tables in inherits() order. Returns nullptr if not
	/// found in either. First-match-wins, matching the legacy parser.
	///
	/// For inherited (legacy) tables, returns nullptr because a legacy row
	/// is not a FieldBase<T>; callers that need to act on inherited fields
	/// must walk asLegacyFieldParse() instead. New-code consumers should
	/// only need find() for own fields.
	const FieldBase<T>* find(const char* token) const
	{
		for (const auto& f : m_fields)
		{
			if (strcmp(f->token, token) == 0)
				return f.get();
		}
		return nullptr;
	}

	/// Iterate own fields (excludes inherited).
	const std::vector<std::unique_ptr<FieldBase<T> > >& fields() const
	{
		return m_fields;
	}

	// ---------------------------------------------------------------------
	// Legacy adapter
	// ---------------------------------------------------------------------

	/// Emit a flat legacy FieldParse[] equivalent to this schema, terminated
	/// by the sentinel { nullptr, nullptr, nullptr, 0 }.
	///
	/// Lifetime: the returned array is owned by this Schema and must not be
	/// freed by the caller. The pointer remains valid as long as the Schema
	/// instance lives — typically program lifetime for static singletons.
	///
	/// Lazily built and cached on first call. After this call no further
	/// add() / inherits() calls are permitted.
	const FieldParse* asLegacyFieldParse() const
	{
		if (!m_legacyTable.empty())
			return m_legacyTable.data();

		// Reserve enough for own fields + every inherited row + sentinel.
		size_t inheritedCount = 0;
		for (const Inherited& inh : m_inherited)
			inheritedCount += countParentRows(inh.table);
		m_legacyTable.reserve(m_fields.size() + inheritedCount + 1);

		// Own fields.
		for (const auto& f : m_fields)
		{
			FieldParse row;
			row.token    = f->token;
			row.parse    = &legacyTrampoline<T>;
			row.userData = f.get();
			row.offset   = static_cast<Int>(f->offset);
			m_legacyTable.push_back(row);
		}

		// Inherited rows: copy as-is, with extraOffset baked into 'offset'.
		// The parent's parse function and userData are preserved verbatim,
		// so a parent table written by hand or by another Schema both work.
		for (const Inherited& inh : m_inherited)
		{
			for (const FieldParse* p = inh.table; p->token != nullptr; ++p)
			{
				FieldParse row = *p;
				row.offset = static_cast<Int>(static_cast<UnsignedInt>(p->offset) + inh.extraOffset);
				m_legacyTable.push_back(row);
			}
			// If the parent table has a catch-all sentinel (token==nullptr,
			// parse!=nullptr), forward it. Otherwise drop the sentinel and
			// let our own terminator close the table.
			const FieldParse* sentinel = inh.table;
			while (sentinel->token != nullptr) ++sentinel;
			if (sentinel->parse != nullptr)
			{
				FieldParse row = *sentinel;
				row.offset = static_cast<Int>(static_cast<UnsignedInt>(sentinel->offset) + inh.extraOffset);
				m_legacyTable.push_back(row);
			}
		}

		// Final terminator.
		FieldParse term;
		term.token    = nullptr;
		term.parse    = nullptr;
		term.userData = nullptr;
		term.offset   = 0;
		m_legacyTable.push_back(term);

		m_finalized = true;
		return m_legacyTable.data();
	}

private:
	struct Inherited
	{
		const FieldParse* table = nullptr;
		UnsignedInt       extraOffset = 0;
	};

	void assertNotFinalized() const
	{
		// A Schema must be fully built before asLegacyFieldParse() is called.
		// This is a programming error, not a recoverable condition.
		DEBUG_ASSERTCRASH(!m_finalized,
			("INI2::Schema modified after asLegacyFieldParse() — schemas must be fully built before first parse use."));
	}

	static size_t countParentRows(const FieldParse* table)
	{
		size_t n = 0;
		while (table->token != nullptr) { ++n; ++table; }
		return n;
	}

	std::vector<std::unique_ptr<FieldBase<T> > > m_fields;
	std::vector<Inherited>                       m_inherited;
	mutable std::vector<FieldParse>              m_legacyTable;
	mutable bool                                 m_finalized = false;
};

} // namespace INI2
