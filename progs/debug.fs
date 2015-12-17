\ This is the frugal forth's powerful debugger.

\ It is (C) 2002 by Doug Hoyte and HardCore SoftWare.
\ 2-clause BSD license.

\ For usage information, see the file DEBUG in the docs/ directory
\ of the frugal distribution.


\ Onetom's memory dumping words.
\ I want to improve upon these in later implementations.
( These next 4 from onetom on #forth. If x is a variable use "x dump-line" )
: h. dup c@ . char+ ;
: c. dup c@ dup 0= if drop 46 then emit char+ ;
: 8h. h. h. h. h. h. h. h. h. ; : 8c. c. c. c. c. c. c. c. c. ;
: dump-line 8h. ." - " 8h. 16 - 3 spaces 8c. ."  - " 8c. drop ;


: debug-procprint ( curr-addr word-addr -- nextaddr )
  2dup = if ." > " else 2 spaces then nip
  dup . ." : "
  dup @ VM_NUMBER = if cell+ dup @ ." LITERAL: " . cr cell+ exit then
  dup @ VM_BRANCH = if cell+ dup @ ." VM_BRANCH -> " . cr cell+ exit then
  dup @ VM_IFBRANCH = if cell+ dup @ ." VM_IFBRANCH -> " . cr cell+ exit then
  dup @ 'print cr cell+
;

: debug-see ( curr-addr word-addr -- )
  dup isunseeable if ." *** UNSEEABLE" cr 2drop exit then 
  begin over swap debug-procprint dup @ VM_PRIMITIVE = until
  2dup = if ." > " else 2 spaces then . ." : EXIT" cr drop
;

: rdepth rp 4 - r0 - 4 / ;
: .rs 60 emit space rdepth . 62 emit space rdepth 1 < if exit then 
    rdepth 0 do r0 i cells + ? loop ." [TOP]" ;

: findcurrword ( addr -- word-addr )
  l begin @ 2dup >= until nip head>
;

: step
  r> r>
  dup @ VM_PRIMITIVE = if ." *** DEBUGGER DONE" cr 2drop exit then
  dup @ VM_NUMBER = if dup cell+ @ rot cell+ cell+ >r >r exit then
  dup @ VM_BRANCH = if cell+ @ >r >r exit then
  dup @ VM_IFBRANCH = if 2 pick if cell+ @ else cell+ cell+ then
                        >r >r drop exit then
  dup cell+ swap @ rot >r >r >r
;

: into
  r> r>
  dup @ 10 < if ." *** DUDE! HOW CAN I STEP INTO THAT INSTRUCTION?" cr
               >r >r exit then
  dup @ @ VM_PRIMITIVE = if ." *** SORRY, THAT INSTRUCTION IS PRIMITIVE." cr
               >r >r exit then
  2dup @ 2swap cell+ >r >r >r >r
;

: run r> drop ;

: fwd
  r> r>
  dup @ VM_PRIMITIVE = if ." *** CAN'T FWD PAST THAT" cr >r >r exit then
  dup @ VM_NUMBER = if cell+ cell+ >r >r exit then
  dup @ VM_BRANCH = if cell+ cell+ >r >r exit then
  dup @ VM_IFBRANCH = if cell+ cell+ >r >r exit then
  cell+ >r >r
;

: bwd
  r> r>
  dup findcurrword over = if ." *** CAN'T BWD PAST THAT" cr >r >r exit then
  dup 8 - @ VM_NUMBER = if 8 - >r >r exit then
  dup 8 - @ VM_BRANCH = if 8 - >r >r exit then
  dup 8 - @ VM_IFBRANCH = if 8 - >r >r exit then
  4 - >r >r
;

: debug
  begin
    ." DEBUGGING: '"
    i dup findcurrword dup 'print ." '" cr
    debug-see
    ." DATA STACK: " .s cr
    ." RTRN STACK: " .rs cr
    query begin
      interpret >in 0 >
    while
  again
;

: isprim? @ VM_PRIMITIVE = if 1 else 0 then ;

: debug> ' dup isprim? if ." SORRY, PRIMITIVE WORD" cr exit then
           >r [ #, ' debug , ] >r ;
