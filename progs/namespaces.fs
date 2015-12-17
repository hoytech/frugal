\ This is the frugal forth's namespace code.

\ It is (C) 2002 by Doug Hoyte and HardCore SoftWare.
\ Your rights to modify and use this program are protected
\ under the GNU GPL. See www.gnu.org or the file COPYING for details.

\ For usage information, see the file NAMESPACES in the docs/ directory
\ of the frugal distribution.


: get-marker s" ->namespace<-" '-addr >head ;

: namespace{   s" ->namespace<-" create-addr compile-exit ;
: }namespace   get-marker @ l ! ;

: export-name
  ( Unlink last entry, leave its addr on stack: )       l @ dup @ l !
  ( Link marker to last entry, leave prev on stack: )   get-marker dup @ rot !
  ( Link last to prev: )                                get-marker @ !
;

unlink get-marker
