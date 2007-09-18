/*
 * elf_data.hpp
 *
 * Copyright (C) 2007 Bastian Blank <waldi@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef ELF_DATA_HPP
#define ELF_DATA_HPP

#include "elf.hpp"
#include "elf_endian.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include <stdint.h>

namespace Elf
{
  template <typename _class, typename _data>
    class file_data : public file
    {
      public:
        file_data (const char *) throw (std::bad_alloc, std::runtime_error);
        file_data (const char *, void *, size_t len) throw (std::bad_alloc, std::runtime_error);

        const uint8_t get_class () const throw () { return _class::id; }
        const uint8_t get_data () const throw () { return _data::id; }

      private:
        void construct () throw (std::bad_alloc, std::runtime_error);
    };

  template <typename _class, typename _data>
    class section_data : public virtual section
    {
      public:
        section_data (void *, void *) throw ();
    };

  template <typename _class, typename _data, typename _type>
    class section_real : public section_data<_class, _data>, public section_type<_type>
    {
      public:
        section_real (void *a, void *b) throw () : section_data<_class, _data> (a, b) { }
    };

  template <typename _class, typename _data>
    class section_real<_class, _data, section_type_DYNAMIC> : public section_data<_class, _data>, public section_type<section_type_DYNAMIC>
    {
      public:
        section_real (void *, void *) throw (std::bad_alloc);
    };

  template <typename _class, typename _data>
    class section_real<_class, _data, section_type_DYNSYM> : public section_data<_class, _data>, public section_type<section_type_DYNSYM>
    {
      public:
        section_real (void *, void *) throw (std::bad_alloc);
    };

  template <typename _class, typename _data>
    class segment_data : public virtual segment
    {
      public:
        segment_data (void *, void *) throw ();
    };

  template <typename _class, typename _data, typename _type>
    class segment_real : public segment_data<_class, _data>, public segment_type<_type>
    {
      public:
        segment_real (void *a, void *b) throw () : segment_data<_class, _data> (a, b) { }
    };

  template <typename _class, typename _data>
    class segment_real<_class, _data, segment_type_INTERP>
    : public segment_data<_class, _data>, public segment_type<segment_type_INTERP>
    {
      public:
        segment_real (void *, void *) throw (std::bad_alloc);
    };

  template <typename _class, typename _data>
    class dynamic_data : public dynamic
    {
      public:
        dynamic_data (void *) throw ();

        void update_string_table (file *, uint16_t) throw (std::bad_alloc);
    };

  template <typename _class, typename _data>
    class symbol_data : public symbol
    {
      public:
        symbol_data (void *) throw ();

        void update_string_table (file *, uint16_t) throw (std::bad_alloc);
    };
}

#endif
