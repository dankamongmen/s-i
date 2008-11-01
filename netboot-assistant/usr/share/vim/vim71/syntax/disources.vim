" Vim syntax file
" Language:	Debian di-netboot-assistant's  di-sources.list
" Maintainer:	Frank Lin PIAT <fpiat@klabs.be>
" Last Change:	2008/06/07
" URL: http://wiki.debian.org/DebianInstaller/NetbootAssistant
" $Revision: 1.1 $

" this is a very simple syntax file, based on Matthijs Mohlmann's debsources.vim

" Standard syntax initialization
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" case sensitive
syn case match

" Architectures that support netbooting
"syn match diSourcesArchs        /\(alpha\|amd64\|arm\|armel\|hppa\|i386\|ia64\|mips\|mipsel\|powerpc\|sparc\)/
syn match diSourcesArchs        /\(alpha\|amd64\|hppa\|i386\|ia64\|sparc\(\|32\|64\)\)/

" Architectures that don't support netbooting, and other forbiden characters
syn match diSourcesNoNetbootarch  /\(m68k\|s390\)/

" Match comments
syn match diSourcesComment        /#.*/

" Match uri's
syn match diSourcesUri            +\(http://\|ftp://\|[rs]sh://\|debtorrent://\|\(cdrom\|copy\|file\):\)[^' 	<>"]\++
syn match diSourcesDistrKeyword   +\([[:alnum:]_./]*\)\(sarge\|etch\|lenny\|\(old\)\=stable\|testing\|unstable\|daily\|sid\|experimental\|dapper\|feisty\|gutsy\|hardy\|intrepid\)\([-[:alnum:]_./]*\)+

" Associate our matches and regions with pretty colours
hi def link diSourcesArchs           Statement
hi def link diSourcesNoNetbootarch   Error
hi def link diSourcesDistrKeyword    Type
hi def link diSourcesComment         Comment
hi def link diSourcesUri             Constant

let b:current_syntax = "disources"

" vim: ts=8
