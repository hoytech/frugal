: \  10 parse drop drop ; immediate
: (  41 parse drop drop ; immediate
: #! 10 parse drop drop ; immediate
\ The above 3 definitions are for comments. DO NOT REMOVE!

\ HardCore SoftWare's FRUGAL system

\ These are the FORTH words that aren't defined in the core, but they
\ are essential language elements.


\ ---------------------- FUNDAMENTALS
: dup 0 pick ;
: over 1 pick ;
: swap 1 roll ;
: rot 2 roll ;
: nip swap drop ;
: tuck dup rot ;
: 2swap 3 roll 3 roll ;
: 2drop drop drop ;
: 2over 3 pick 3 pick ;
: 2dup 1 pick 1 pick ;
: 0= 0 = ;
: <> = 0= ;
: <= > 0= ;
: >= < 0= ;
: i  r> r> tuck >r >r ;
: i' r> r> r> dup 3 roll >r >r >r ;
: j  r> r> r> r> dup 4 roll >r >r >r >r ;
: j' r> r> r> r> r> dup 5 roll >r >r >r >r >r ;



\ ---------------------- TIME STUFF, ETC
: ms 1000 * usleep ;


\ ---------------------- HEAP STUFF, ETC
: here h @ ;
: allot h @ + h ! ;
: constant create 2 ( Hardwired VM_NUMBER ) , , compile-exit ;
: const constant ;
: var 32 parse create-addr here cell-bytes 4 * + dup 2 ( Hardwired VM_NUMBER ) , ,
      compile-exit 0 ,  ! ;
: variable 0 var ;
: 2variable variable 0 , ;
: cell+ cell-bytes + ;
: cells cell-bytes * ;
: char+ 1 + ;
: chars ;
: 2! swap over ! cell+ ! ;
: 2@ dup cell+ @ swap @ ;


\ ---------------------- SYSTEM CONSTANTS
0 constant STDIN
1 constant STDOUT

0 constant READ-ONLY
1 constant WRITE-ONLY
2 constant READ-WRITE

1 constant IMMEDIATE
2 constant COMPILE-ONLY
4 constant UNSEEABLE

1 constant VM_PRIMITIVE
2 constant VM_NUMBER
3 constant VM_BRANCH
4 constant VM_IFBRANCH

32 cell-bytes / 2 + constant HEADER_LEN

0 constant 0
1 constant 1


\ ---------------------- COMPILING WORDS
\ Thanks to onetom on #forth for these next 3:
: #,  VM_NUMBER , ;
: branch  VM_BRANCH , ;
: ?branch  VM_IFBRANCH , ;
: >head HEADER_LEN cells - ;
: head> HEADER_LEN cells + ;
: [ 0 state ! ; immediate
: ] 1 state ! ;
\ compile is a non-traditional forth word.
\ It's another approach to the same thing does> does. :)
: cmp ' #, , ; immediate
: compile ' #, , cmp , , ; immediate
: postpone ' , ; immediate compile-only
\ : exit compile-exit ; compile-only immediate
: exit r> drop ;
: unseeable l @ cell+ @ UNSEEABLE or l @ cell+ ! ;
: isunseeable >head cell+ @ UNSEEABLE and ;


\ ---------------------- CONTROL-FLOW/DECISION WORDS
: if compile 0= ?branch here 0 , ; immediate compile-only
: else branch here 0 , swap here swap ! ; immediate compile-only
: then here swap ! ; immediate compile-only

: begin here ; immediate compile-only
: again branch , ; immediate compile-only
: while ?branch , ; immediate compile-only
: until compile 0= ?branch , ; immediate compile-only
: do compile swap compile >r compile >r here ; immediate compile-only
: +loop
        compile r> compile + compile dup compile i compile <>
        compile swap compile >r
        ?branch , 
        compile r> compile drop compile r> compile drop
        ; immediate compile-only
: loop #, 1 , postpone +loop ; immediate compile-only
: leave r> r> drop r> dup >r 1 - >r >r ;
: recurse l @ head> , ; immediate compile-only
: ;recurse l @ head> branch , ; immediate compile-only
: printnum
  base @ 10 = if qpnum exit then
  dup 0 < if -1 * 45 emit then
  -1 swap begin
    dup base @ mod swap base @ /
  dup while drop
  begin
    dup 9 > if 87 + emit else 48 + emit then
  dup 0 < until drop
;
: . printnum 32 emit ;


\ ---------------------- MATH, ETC
: negate -1 * ;
: 1+ 1 + ;
: 1- 1 - ;
: 2* 2 * ;
: 2/ 2 / ;
: min 2dup < if drop else nip then ;
: max 2dup > if drop else nip then ;
: abs dup 0 < if -1 * then ;
: binary 2 base ! ;
: octal 8 base ! ;
: decimal 10 base ! ;
: hex 16 base ! ;


\ ---------------------- MEMORY, DIRECT STACK MANIPULATION, ETC
: +! dup @ rot rot + swap ! ;
: ? @ . ;
: depth sp s0 - 4 / ;
  ( These next 2 are right out of the ANSI standard. )
: movecell ( addr1-src addr2-dest -- ) swap @ swap ! ;
: move 0 do 2dup i cells + swap i cells + swap movecell loop 2drop ;
: num' 32 parse number drop ;
: unlink ' >head dup l @ = if l @ @ l ! drop exit then
         l begin @ dup @ 2 pick = until swap @ swap ! ;


\ ---------------------- I/O, ETC
: cr 10 emit ;
: bl 32 ;
: space bl emit ;
: spaces dup 0 > if 0 do space loop else drop then ;
: type 0 do dup c@ emit char+ loop drop ;
: char 32 parse drop c@ ;
: char, char #, , ; immediate
: movechar ( caddr1-src caddr2-dest -- ) swap c@ swap c! ;
: cmove 0 do 2dup i chars + swap i chars + swap movechar loop 2drop ;
: ," here 34 parse tuck here swap dup allot cmove align ;
: s" unseeable #, here >r 0 , #, here >r 0 , branch here 0 ,
    ," r> ! r> ! here swap !
  ;  immediate compile-only
: ." postpone s" compile type compile flush ; immediate compile-only
: .( 41 parse type ;

: accept STDIN rot read ;  
: key term-raw-on
      0 ( This 0 will contain the char )
      sp 1 cells -  STDIN swap 1 read drop
      term-raw-off ;
: rawkey 0 ( This 0 will contain the char )
         sp 1 cells -  STDIN swap 1 read drop ;


\ ---------------------- TERM
: emchar #, char , compile emit ; immediate
: emesc 27 emit ;

\ COLOURS
0 constant black
1 constant red
2 constant green
3 constant yellow
4 constant blue
5 constant magenta
6 constant cyan
7 constant white

\ ATTRIBUTES
0 constant restore
1 constant bright
2 constant dim
4 constant underscore
5 constant blink
7 constant reverse
8 constant hidden

: term-fg ( colour -- ) 30 + emesc emchar [ qpnum emchar m ;
: term-bg ( colour -- ) 40 + emesc emchar [ qpnum emchar m ;
: term-attr ( attr -- ) emesc emchar [ qpnum emchar m ;
: term-cls emesc emchar [ emchar 2 emchar J ;
: term-wrap-on emesc emchar [ emchar 7 emchar h ;
: term-wrap-off emesc emchar [ emchar 7 emchar l ;
: term-cur-on emesc emchar [ emchar ? 25 qpnum emchar h ;
: term-cur-off emesc emchar [ emchar ? 25 qpnum emchar l ;
: term-home emesc emchar [ emchar H ;
: term-xy ( x y -- ) emesc emchar [ qpnum emchar ; qpnum emchar H ;
: term-up emesc emchar [ emchar A ;
: term-down emesc emchar [ emchar B ;
: term-forward emesc emchar [ emchar C ;
: term-backward emesc emchar [ emchar D ;
: term-save emesc emchar 7 ;
: term-unsave emesc emchar 8 ;
: term-enable-scroll ( start end -- ) 
       swap emesc emchar [ qpnum emchar ; qpnum emchar r ;
: term-scroll-down emesc emchar [ emchar D ;
: term-scroll-up emesc emchar [ emchar M ;
: term-clear-to-eol emesc emchar [ emchar K ;
: term-clear-to-sol emesc emchar [ emchar 1 emchar K ;
: term-clear-line emesc emchar [ emchar 2 emchar L ;
: term-clear-to-top emesc emchar [ emchar J ;
: term-clear-to-bot emesc emchar [ emchar 1 emchar J ;


\ ---------------------- TOOLS, ETC
\ : abort" ?branch here 0 , compile-exit here swap !
\  #, compile abort compile >r postpone ." ; immediate compile-only
: .s 60 emit space depth . 62 emit space depth 1 < if exit then 
    depth 0 do s0 i cells + ? loop ." [TOP]" ;
: forget-addr >head dup @ l ! h ! ;
: forget ' forget-addr ;
\ print-word Takes an address from the start of the header.
\ 'print from the beginning of the actual code (where ' returns)
: print-word ( addr -- ) cell+ cell+ dup char+ swap c@ type ;
: 'print >head print-word ;
: bytes-and-words ( -- bytes words )
  0 l begin @ swap 1+ swap dup @ 0= until
  here swap -   swap ;
: words ." ( " bytes-and-words . ." LINKED WORDS  - " . ." BYTES )" cr l @
  begin dup print-word space @ dup 0= until drop ;
: procprint ( addr -- nextaddr )
  dup . ." : "
  dup @ VM_NUMBER = if cell+ dup @ ." LITERAL: " . cr cell+ exit then
  dup @ VM_BRANCH = if cell+ dup @ ." VM_BRANCH -> " . cr cell+ exit then
  dup @ VM_IFBRANCH = if cell+ dup @ ." VM_IFBRANCH -> " . cr cell+ exit then
  dup @ 'print cr cell+ ;
: see ' dup isunseeable if ." UNSEEABLE" cr drop exit then 
  dup @ VM_PRIMITIVE = if ." PRIMITIVE" cr drop exit then
  begin procprint dup @ VM_PRIMITIVE = until . ." : EXIT" cr ;
: abort ." RESETING STACKS AND STATE" cr quit ;


\ ---------------------- RANDOM
0 var rand-num
: reseed ( -- ) gettime nip rand-num ! ;
reseed
: rand rand-num @ 31421 * 6927 + dup rand-num ! ;
: choose dup 2 < if ." CHOOSE DOES NOT ACCEPT NUMBERS BELOW 2" cr abort then
         rand swap mod abs ;


\ ---------------------- INDIRECTION
: icreate create compile exit compile-exit ; 
: iset> ' swap ! ;


\ ---------------------- WRAP UP
: bye term-cur-on restore term-attr die ;
: 80x25-bye term-save
              1 25 term-enable-scroll restore term-attr term-cur-on
            term-unsave bye ;
