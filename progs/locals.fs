\ This is frugal's not-quite-ANS-compatible locals implementation.
\ It is copyright (C) 2002, by HardCore SoftWare.
\ 2-clause BSD license.

\ See the file docs/LOCALS for more info

variable local-name-space 1020 allot

: rdrop  r> r> drop >r ;
: 0>r  r> 0 >r >r ;

: make-local-frame  r> 0>r 0>r 0>r 0>r 0>r 0>r 0>r 0>r >r ;
: kill-local-frame  r> rdrop rdrop rdrop rdrop rdrop rdrop rdrop rdrop >r ;
: compile-klf  compile kill-local-frame ;

: get-local-addr  compile rp compile + ;


: volatile";"
    s" ;" create-addr here #, ,
    compile here compile swap compile forget-addr compile h compile !
    compile [ compile compile-klf
    compile compile-exit compile-exit
    immediate
; immediate

: new-local ( picklevel c-addr len -- )
  create-addr immediate
  VM_NUMBER #, , compile ,  1+ cells negate  #, , compile ,
  compile get-local-addr compile-exit
;

: {
  here local-name-space h !
  postpone volatile";"
  -1 begin 1+ dup
    32 parse
    2dup 1 = swap c@ char, } = and
      if 2drop drop 1 else new-local 0 then
  until drop
  h !
  compile make-local-frame
; immediate compile-only
