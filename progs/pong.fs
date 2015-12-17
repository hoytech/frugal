\ DOUG'S PONG FOR FRUGAL

\ 2-clause BSD license.

10 const padwidth
padwidth 2/ const padwidth/2
10 const termvel
10 const steptime
 5 const steps
 
 0 var currstep
 0 var level

40 var pad1x
 0 var pad1v
40 var pad2x
 0 var pad2v

40 var bx
12 var by
 0 var bxv
-1 var byv

 0 var aibattle


: init ( -- )
  term-cur-off term-raw-on
;

: restart
  40 pad1x !
   0 pad1v !
  40 pad2x !
   0 pad2v !
  40 bx !
  12 by !
   0 bxv !
  -1 byv !
  2 choose 0= if -1 else 1 then bxv !
;

: menu ( -- choice )
  term-cls magenta term-fg
  10 3 term-xy ." PONG!"
  green term-fg
  10 5 term-xy ." The 'k' and 'l' keys move you left and right."
  10 6 term-xy ." ESC exits game play. Don't even bother with level=4. :)"
  10 8 term-xy ." Please choose your level:"
  yellow term-fg
  10 10 term-xy ." ESC...EXIT"
  cyan term-fg
  10 11 term-xy ." 1.....EASY"
  10 12 term-xy ." 2.....MEDIUM"
  10 13 term-xy ." 3.....HARD"
  red term-fg
  10 14 term-xy ." 4.....IMPOSSIBLE"
  yellow term-fg
  10 16 term-xy ." A.....TOGGLE AI BATTLE="
  aibattle @ if ." ON" else ." OFF" then
  white term-fg
  10 18 term-xy ." CHOICE?"
  white term-fg
  rawkey
;

: draw-paddle ( c x y -- )
  swap padwidth/2 - swap
  padwidth   0 do
                 2dup term-xy 2 pick emit
                 swap 1+ swap
               loop 
  2drop drop
;

: update-screen ( paddle2char paddle1char ballchar -- )
  cyan term-fg bx @ by @ term-xy emit
  red term-fg pad1x @ 0 draw-paddle
  green term-fg pad2x @ 25 draw-paddle
  white term-fg flush
;

: handle-keypress ( -- should-we-exit? )
  rawkey
  aibattle @ 0= if
   dup char, k = if -1 pad2v +! then
   dup char, l = if  1 pad2v +! then
  then
  27 = if 1 else 0 then
;

: padxrange ( -- min max )
  padwidth/2   80 over -
;

: check-bounds
  pad1x @ pad2x @
  padxrange pad1x @ min max pad1x !
  padxrange pad2x @ min max pad2x !
  pad2x @ <> if 0 pad2v ! then
  pad1x @ <> if 0 pad1v ! then
;

: padhit ( player -- hit? ) 
  1 = if
    bx @ pad1x @ padwidth/2 - >=
    bx @ pad1x @ padwidth/2 + <
    and
  else
    bx @ pad2x @ padwidth/2 - >=
    bx @ pad2x @ padwidth/2 + <
    and
  then
;

: newround ( winningplayer -- )
  term-cls 20 5 term-xy
  ." PLAYER " . ." WINS ROUND!"
  20 7 term-xy ." PRESS ANY KEY TO CONTINUE" flush
  aibattle @ if 1000 ms else rawkey drop then restart term-cls
;

: 1aiproc
  bx @ pad1x @ padwidth/2 + > if level @ 2 +        pad1v +! then
  bx @ pad1x @ padwidth/2 - < if level @ 2 + negate pad1v +! then
  bxv @ abs 3 + dup negate swap
  pad1v @ min max pad1v !
  3 choose 1- pad1v +!
  level @ 4 = if 0 bx @ pad1x ! then
;

: 2aiproc
  bx @ pad2x @ padwidth/2 + > if level @ 2 +        pad2v +! then
  bx @ pad2x @ padwidth/2 - < if level @ 2 + negate pad2v +! then
  bxv @ abs 3 + dup negate swap
  pad2v @ min max pad2v !
  3 choose 1- pad2v +!
  level @ 4 = if 0 bx @ pad2x ! then
;

: aiproc 1aiproc aibattle @ if 2aiproc then ;

: proc
  pad1v @ pad1x +!
  pad2v @ pad2x +!
  bxv @ bx +!
  byv @ by +!
  bx @  0 < if  0 bx !  bxv @ negate bxv !  bxv @ 2* bx +! then
  bx @ 80 > if 79 bx !  bxv @ negate bxv !  byv @ 2* negate bx +! then
  by @  2 <= if 1 padhit if pad1v @ bxv +! byv @ negate byv !  2 by ! then then
  by @ 24 >= if 2 padhit if pad2v @ bxv +! byv @ negate byv ! 24 by ! then then
  by @  1 < if  2 newround then
  by @ 25 > if  1 newround then
  termvel dup negate swap bxv @ min max bxv !
;

: play
  term-cls
  begin
    char, ^ char, v char, * update-screen
    steptime ms
    1 currstep +!
    STDIN 0 poll
      dup currstep @ steps >= or if bl bl bl update-screen then
      if handle-keypress else 0 then
    currstep @ steps >= if aiproc proc check-bounds 0 currstep ! then
  until
;

: pong
  init
  begin
    menu
    restart 
    dup char, 1 = if 1 level !  play then
    dup char, 2 = if 2 level !  play then
    dup char, 3 = if 3 level !  play then
    dup char, 4 = if 4 level !  play then
    dup char, a = if aibattle @ 0= aibattle ! then
  27 = until
  term-cls
  0 0 term-xy
  bye
;

pong \ RUN THE PROGRAM!
