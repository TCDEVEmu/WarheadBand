/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AC_SMARTENUM_H
#define AC_SMARTENUM_H

#include "IteratorPair.h"
#include <iterator>

struct EnumText
{
    EnumText(char const* c, char const* t, char const* d) : Constant(c), Title(t), Description(d) { }
    // Enum constant of the value
    char const* const Constant;
    // Human-readable title of the value
    char const* const Title;
    // Human-readable description of the value
    char const* const Description;
};

namespace Warhead::Impl::EnumUtilsImpl
{
    template <typename Enum>
    struct EnumUtils
    {
        static size_t Count();
        static EnumText ToString(Enum value);
        static Enum FromIndex(size_t index);
        static size_t ToIndex(Enum index);
    };
}

class EnumUtils
{
public:
    template <typename Enum>
    static size_t Count() { return Warhead::Impl::EnumUtilsImpl::EnumUtils<Enum>::Count(); }
    template <typename Enum>
    static EnumText ToString(Enum value) { return Warhead::Impl::EnumUtilsImpl::EnumUtils<Enum>::ToString(value); }
    template <typename Enum>
    static Enum FromIndex(size_t index) { return Warhead::Impl::EnumUtilsImpl::EnumUtils<Enum>::FromIndex(index); }
    template <typename Enum>
    static uint32 ToIndex(Enum value) { return Warhead::Impl::EnumUtilsImpl::EnumUtils<Enum>::ToIndex(value);}

    template<typename Enum>
    static bool IsValid(Enum value)
    {
        try
        {
            Warhead::Impl::EnumUtilsImpl::EnumUtils<Enum>::ToIndex(value);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    template<typename Enum>
    static bool IsValid(std::underlying_type_t<Enum> value) { return IsValid(static_cast<Enum>(value)); }

    template <typename Enum>
    class Iterator
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = Enum;
        using pointer = Enum*;
        using reference = Enum&;
        using difference_type = std::ptrdiff_t;

        Iterator() : _index(EnumUtils::Count<Enum>()) {}
        explicit Iterator(size_t index) : _index(index) { }

        bool operator==(const Iterator& other) const { return other._index == _index; }
        bool operator!=(const Iterator& other) const { return !operator==(other); }
        difference_type operator-(Iterator const& other) const { return _index - other._index; }
        bool operator<(const Iterator& other) const { return _index < other._index; }
        bool operator<=(const Iterator& other) const { return _index <= other._index; }
        bool operator>(const Iterator& other) const { return _index > other._index; }
        bool operator>=(const Iterator& other) const { return _index >= other._index; }

        value_type operator[](difference_type d) const { return FromIndex<Enum>(_index + d); }
        value_type operator*() const { return operator[](0); }

        Iterator& operator+=(difference_type d) { _index += d; return *this; }
        Iterator& operator++() { return operator+=(1); }
        Iterator operator++(int) { Iterator i = *this; operator++(); return i; }
        Iterator operator+(difference_type d) const { Iterator i = *this; i += d; return i; }

        Iterator& operator-=(difference_type d) { _index -= d; return *this; }
        Iterator& operator--() { return operator-=(1); }
        Iterator operator--(int) { Iterator i = *this; operator--(); return i; }
        Iterator operator-(difference_type d) const { Iterator i = *this; i -= d; return i; }

    private:
        difference_type _index;
    };

    template <typename Enum>
    static Iterator<Enum> Begin() { return Iterator<Enum>(0); }

    template <typename Enum>
    static Iterator<Enum> End() { return Iterator<Enum>(); }

    template <typename Enum>
    static Warhead::IteratorPair<Iterator<Enum>> Iterate() { return { Begin<Enum>(), End<Enum>() }; }

    template <typename Enum>
    static char const* ToConstant(Enum value) { return ToString(value).Constant; }

    template <typename Enum>
    static char const* ToTitle(Enum value) { return ToString(value).Title; }

    template <typename Enum>
    static char const* ToDescription(Enum value) { return ToString(value).Description; }
};

#endif // AC_SMART_ENUM_H
