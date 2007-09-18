/*
 * elf.cpp
 *
 * Copyright (C) 2005 Bastian Blank <waldi@debian.org>
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

#include "elf_data.hpp"

#include <stdexcept>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace Elf;

file::~file () throw ()
{
  ::munmap (mem, len);
  for (std::vector<section *>::iterator it = sections.begin (); it != sections.end (); ++it)
    delete *it;
}

file *file::open (const char *filename) throw (std::bad_alloc, std::runtime_error)
{
  struct stat buf;
  int fd;
  void *mem;
  size_t len;
  if ((fd = ::open (filename, O_RDONLY)) == -1)
    throw std::runtime_error ("mapping failed");
  try
  {
    if (::fstat (fd, &buf) == -1)
      throw std::runtime_error ("mapping failed");
    len = buf.st_size;
    if ((mem = ::mmap (0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
      throw std::runtime_error ("mapping failed");

    const uint8_t *buf = static_cast <uint8_t *> (mem);

    switch (buf[EI_CLASS])
    {
      case ELFCLASS32:
        return open_class<file_class_32> (mem, len);
      case ELFCLASS64:
        return open_class<file_class_64> (mem, len);
      default:
        throw std::runtime_error ("Invalid file class");
    }
  }
  catch (...)
  {
    ::close (fd);
    throw;
  }
}

template<typename _class>
file *file::open_class (void * mem, size_t len) throw (std::bad_alloc, std::runtime_error)
{
  const uint8_t *buf = static_cast <uint8_t *> (mem);

  switch (buf[EI_DATA])
  {
    case ELFDATA2LSB:
      return new file_data<_class, file_data_2LSB> (mem, len);
    case ELFDATA2MSB:
      return new file_data<_class, file_data_2MSB> (mem, len);
    default:
      throw std::runtime_error ("Invalid file data");
  }
}

template <typename _class, typename _data>
file_data<_class, _data>::file_data (void *mem, size_t len) throw (std::bad_alloc, std::runtime_error)
: file (mem, len)
{
  uint8_t *buf = static_cast <uint8_t *> (this->mem);
  if (buf[EI_CLASS] != _class::id)
    throw std::runtime_error ("Wrong file class");
  if (buf[EI_DATA] != _data::id)
    throw std::runtime_error ("Wrong data encoding");

  typedef typename _elfdef<_class>::Ehdr Ehdr;
  Ehdr *ehdr = static_cast <Ehdr *> (this->mem);
  this->type      = convert<_data, typeof (ehdr->e_type     )> () (ehdr->e_type);
  this->machine   = convert<_data, typeof (ehdr->e_machine  )> () (ehdr->e_machine);
  this->phoff     = convert<_data, typeof (ehdr->e_phoff    )> () (ehdr->e_phoff);
  this->shoff     = convert<_data, typeof (ehdr->e_shoff    )> () (ehdr->e_shoff);
  this->flags     = convert<_data, typeof (ehdr->e_flags    )> () (ehdr->e_flags);
  this->phentsize = convert<_data, typeof (ehdr->e_phentsize)> () (ehdr->e_phentsize);
  this->phnum     = convert<_data, typeof (ehdr->e_phnum    )> () (ehdr->e_phnum);
  this->shentsize = convert<_data, typeof (ehdr->e_shentsize)> () (ehdr->e_shentsize);
  this->shnum     = convert<_data, typeof (ehdr->e_shnum    )> () (ehdr->e_shnum);
  this->shstrndx  = convert<_data, typeof (ehdr->e_shstrndx )> () (ehdr->e_shstrndx);

  typedef typename _elfdef<_class>::Shdr Shdr;
  Shdr *shdrs = static_cast <Shdr *> (static_cast <void *> (static_cast <char *> (this->mem) + this->shoff));

  this->sections.resize (this->shnum);

  for (unsigned int i = 0; i < this->shnum; i++)
  {
    section *temp;
    switch (convert<_data, typeof (shdrs[i].sh_type)> () (shdrs[i].sh_type))
    {
      case section_type_STRTAB::id:
        temp = new section_real<_class, _data, section_type_STRTAB> (&shdrs[i], this->mem);
        break;
      case section_type_DYNAMIC::id:
        temp = new section_real<_class, _data, section_type_DYNAMIC> (&shdrs[i], this->mem);
        break;
      case section_type_DYNSYM::id:
        temp = new section_real<_class, _data, section_type_DYNSYM> (&shdrs[i], this->mem);
        break;
      case section_type_GNU_VERDEF::id:
        temp = new section_real<_class, _data, section_type_GNU_VERDEF> (&shdrs[i], this->mem);
        break;
      case section_type_GNU_VERSYM::id:
        temp = new section_real<_class, _data, section_type_GNU_VERSYM> (&shdrs[i], this->mem);
        break;
      default:
        temp = new section_real<_class, _data, section_type_UNDEFINED> (&shdrs[i], this->mem);
        break;
    }
    this->sections[i] = temp;
  }

  typedef typename _elfdef<_class>::Phdr Phdr;
  Phdr *phdrs = static_cast <Phdr *> (static_cast <void *> (static_cast <char *> (this->mem) + this->phoff));

  this->segments.resize (this->phnum);

  for (unsigned int i = 0; i < this->phnum; i++)
  {
    segment *temp;
    switch (convert<_data, typeof (phdrs[i].p_type)> () (phdrs[i].p_type))
    {
      case segment_type_INTERP::id:
        temp = new segment_real<_class, _data, segment_type_INTERP> (&phdrs[i], this->mem);
        break;
      default:
        temp = new segment_real<_class, _data, segment_type_UNDEFINED> (&phdrs[i], this->mem);
        break;
    }
    this->segments[i] = temp;
  }

  for (unsigned int i = 0; i < this->shnum; i++)
    this->sections[i]->update (this);
}

void section::update (const file *file) throw (std::bad_alloc)
{
  const section_type<section_type_STRTAB> *section =
    dynamic_cast <const section_type<section_type_STRTAB> *> (file->get_section (file->get_shstrndx ()));
  this->name_string = section->get_string (this->name);
}

template <typename _class, typename _data>
section_data<_class, _data>::section_data (void *header, void *mem) throw ()
{
  typedef typename _elfdef<_class>::Shdr Shdr;
  Shdr *shdr = static_cast <Shdr *> (header);
  this->name   = convert<_data, typeof (shdr->sh_name  )> () (shdr->sh_name  );
  this->type   = convert<_data, typeof (shdr->sh_type  )> () (shdr->sh_type  );
  this->offset = convert<_data, typeof (shdr->sh_offset)> () (shdr->sh_offset);
  this->size   = convert<_data, typeof (shdr->sh_size  )> () (shdr->sh_size  );
  this->link   = convert<_data, typeof (shdr->sh_link  )> () (shdr->sh_link  );
  this->mem = static_cast <void *> (static_cast <char *> (mem) + this->offset);
}

section_type<section_type_DYNAMIC>::~section_type () throw ()
{
  for (std::vector<dynamic *>::iterator it = dynamics.begin (); it != dynamics.end (); ++it)
    delete *it;
}

template <typename _class, typename _data>
section_real<_class, _data, section_type_DYNAMIC>::section_real (void *header, void *mem) throw (std::bad_alloc)
: section_data<_class, _data> (header, mem)
{
  if (this->type != SHT_DYNAMIC)
    throw std::logic_error ("Wrong section type");
  typedef typename _elfdef<_class>::Dyn Dyn;
  Dyn *dyns = static_cast <Dyn *> (this->mem);
  unsigned int max = this->size / sizeof (Dyn);

  this->dynamics.reserve (max);

  for (unsigned int i = 0; i < max; i++)
    this->dynamics.push_back (new dynamic_data<_class, _data> (&dyns[i]));
}

void section_type<section_type_DYNAMIC>::update (const file *file) throw (std::bad_alloc)
{
  section::update (file);

  const section_type<section_type_STRTAB> *section =
    dynamic_cast <const section_type<section_type_STRTAB> *> (file->get_section (link));
  for (unsigned int i = 0; i < dynamics.size (); i++)
    this->dynamics[i]->update (section);
}

section_type<section_type_DYNSYM>::~section_type () throw ()
{
  for (std::vector<symbol *>::iterator it = symbols.begin (); it != symbols.end (); ++it)
    delete *it;
}

void section_type<section_type_DYNSYM>::update (const file *file) throw (std::bad_alloc)
{
  section::update (file);

  const section_type<section_type_STRTAB> *section =
    dynamic_cast <const section_type<section_type_STRTAB> *> (file->get_section (link));

  for (unsigned int i = 0; i < symbols.size (); i++)
    this->symbols[i]->update (section);
}

template <typename _class, typename _data>
section_real<_class, _data, section_type_DYNSYM>::section_real (void *header, void *mem) throw (std::bad_alloc)
: section_data<_class, _data> (header, mem)
{
  if (this->type != SHT_DYNSYM)
    throw std::logic_error ("Wrong section type");
  typedef typename _elfdef<_class>::Sym Sym;
  Sym *syms = static_cast <Sym *> (this->mem);
  unsigned int max = this->size / sizeof (Sym);

  this->symbols.reserve (max);

  for (unsigned int i = 0; i < max; i++)
    this->symbols.push_back (new symbol_data<_class, _data> (&syms[i]));
}

template <typename _class, typename _data>
section_real<_class, _data, section_type_GNU_VERDEF>::section_real (void *header, void *mem) throw (std::bad_alloc)
: section_data<_class, _data> (header, mem)
{
  if (this->type != SHT_GNU_verdef)
    throw std::logic_error ("Wrong section type");

  typedef typename _elfdef<_class>::Verdef Verdef;
  char *act = static_cast<char *> (this->mem);
  uint32_t next = 0;

  // TODO: Use DT_VERDEFNUM!
  do
  {
    Verdef *verdef = reinterpret_cast<Verdef *> (act);
    verdefs.push_back(new version_definition_data<_class, _data> (verdef));
    next = convert<_data, typeof (verdef->vd_next)> () (verdef->vd_next);
    act += next;
  }
  while (next);
}

template <typename _class, typename _data>
section_real<_class, _data, section_type_GNU_VERSYM>::section_real (void *header, void *mem) throw (std::bad_alloc)
: section_data<_class, _data> (header, mem)
{
  if (this->type != SHT_GNU_versym)
    throw std::logic_error ("Wrong section type");
  typedef typename _elfdef<_class>::Versym Versym;
  Versym *versyms = static_cast <Versym *> (this->mem);
  unsigned int max = this->size / sizeof (Versym);

  this->versyms.reserve (max);

  for (unsigned int i = 0; i < max; i++)
    this->versyms.push_back (versyms[i]);
}

template <typename _class, typename _data>
segment_data<_class, _data>::segment_data (void *header, void *mem) throw ()
{
  typedef typename _elfdef<_class>::Phdr Phdr;
  Phdr *phdr = static_cast <Phdr *> (header);
  this->type   = convert<_data, typeof (phdr->p_type  )> () (phdr->p_type);
  this->flags  = convert<_data, typeof (phdr->p_flags )> () (phdr->p_offset);
  this->offset = convert<_data, typeof (phdr->p_offset)> () (phdr->p_offset);
  this->filesz = convert<_data, typeof (phdr->p_filesz)> () (phdr->p_filesz);
  this->mem = static_cast <void *> (static_cast <char *> (mem) + this->offset);
}

template <typename _class, typename _data>
segment_real<_class, _data, segment_type_INTERP>::segment_real (void *header, void *mem) throw (std::bad_alloc)
: segment_data<_class, _data> (header, mem)
{
  if (this->type != PT_INTERP)
    throw std::logic_error ("Wrong segment type");

  this->interp = std::string (static_cast <char *> (mem) + this->offset);
}

template <typename _class, typename _data>
dynamic_data<_class, _data>::dynamic_data (void *mem) throw ()
{
  typedef typename _elfdef<_class>::Dyn Dyn;
  Dyn *dyn = static_cast <Dyn *> (mem);
  this->tag = convert<_data, typeof (dyn->d_tag     )> () (dyn->d_tag);
  this->val = convert<_data, typeof (dyn->d_un.d_val)> () (dyn->d_un.d_val);
  this->ptr = convert<_data, typeof (dyn->d_un.d_ptr)> () (dyn->d_un.d_ptr);
  switch (this->tag)
  {
    case DT_NEEDED:
    case DT_SONAME:
    case DT_RPATH:
      this->is_string = true;
      break;
    default:
      this->is_string = false;
      break;
  };
}

template <typename _class, typename _data>
void dynamic_data<_class, _data>::update (const section_type<section_type_STRTAB> *section) throw (std::bad_alloc)
{
  if (this->is_string)
    this->val_string = section->get_string (this->val);
}

template <typename _class, typename _data>
symbol_data<_class, _data>::symbol_data (void *mem) throw ()
{
  typedef typename _elfdef<_class>::Sym Sym;
  Sym *sym = static_cast <Sym *> (mem);
  this->name  = convert<_data, typeof (sym->st_name )> () (sym->st_name);
  this->info  = convert<_data, typeof (sym->st_info )> () (sym->st_info);
  this->shndx = convert<_data, typeof (sym->st_shndx)> () (sym->st_shndx);
  this->value = convert<_data, typeof (sym->st_value)> () (sym->st_value);
  this->size  = convert<_data, typeof (sym->st_size )> () (sym->st_size);
  this->bind = _elfdef<_class>::st_bind (this->info);
  this->type = _elfdef<_class>::st_type (this->info);
}

template <typename _class, typename _data>
void symbol_data<_class, _data>::update (const section_type<section_type_STRTAB> *section) throw (std::bad_alloc)
{
  this->name_string = section->get_string (this->name);
}

template <typename _class, typename _data>
version_definition_data<_class, _data>::version_definition_data (Verdef *verdef) throw ()
{
  this->ndx    = convert<_data, typeof (verdef->vd_ndx)> () (verdef->vd_ndx);
  uint16_t cnt = convert<_data, typeof (verdef->vd_cnt)> () (verdef->vd_cnt);
  uint32_t aux = convert<_data, typeof (verdef->vd_aux)> () (verdef->vd_aux);

  char *act = reinterpret_cast<char *> (verdef) + aux;

  for (int i = 0; i < cnt; i++)
  {
    Verdaux *verdaux = reinterpret_cast<Verdaux *> (act);
    uint32_t name = convert<_data, typeof (verdaux->vda_name)> () (verdaux->vda_name);
    uint32_t next = convert<_data, typeof (verdaux->vda_next)> () (verdaux->vda_next);
    names.push_back(name);
    act += next;
  }
}

template <typename _class, typename _data>
void version_definition_data<_class, _data>::update (const section_type<section_type_STRTAB> *section) throw (std::bad_alloc)
{
}


