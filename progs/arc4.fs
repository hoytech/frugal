\ HardCore Software's implementation of the arc4 stream cipher.
\ V1.0

\ arc4 is a cryptographically secure stream cipher, which can
\ be used for random number generation.

\ Written for the frugal forth system: www.hcsw.org/frugal/

\ (C) 2002 by HardCore SoftWare  -  www.hcsw.org
\ Distribution and modification privileges granted according to
\ the GNU GPL. See www.gnu.org for details.


\ Cipher usage:
\ arc4-setkey ( key-addr keylen -- )
\ arc4-enc    ( src dest length -- )
\ arc4-dec    ( src dest length -- )

\ Random number generator usage:
\ arc4-seed   ( -- )
\ arc4-rand   ( -- random_num )
\ arc4-choose ( max-num -- random_num )


0 var arc4-keyspace 255 cells allot
0 var arc4-x  \ si during keygen
0 var arc4-y  \ ki during keygen



: arc4-clip ( n -- n-mod-255 )
  255 and
;

: arc4-resetkey ( -- )
  0 arc4-x !
  0 arc4-y !
  arc4-keyspace
  256 0 do i over ! cell+ loop
  drop
;

: arc4-swap-x-with ( i -- )
  cells arc4-keyspace + dup @ swap ( i@ i-a )
  arc4-x @ cells arc4-keyspace + dup rot @ swap !
  !
;

: arc4-set-keychurn ( keylen -- )
  arc4-y
  1 over +!
  @ <= if 0 arc4-y ! then
;

: arc4-xor-data ( src-addr dst-addr i -- )
  tuck
  + rot + @
  arc4-keyspace arc4-x @ cells + @
  arc4-keyspace arc4-y @ cells + @
  + arc4-clip cells arc4-keyspace + @
  xor swap c!
;

: arc4-setkey ( addr-of-key keylen -- )
  arc4-resetkey
  256 0 do
    over arc4-y @ + @           \ key[ki]
    arc4-keyspace i cells + @   \ keyspace[i]
    arc4-x @                    \ si
    + + arc4-clip arc4-x !      \ add em 'n clip
    i arc4-swap-x-with
    dup arc4-set-keychurn
  loop
  0 arc4-x ! 0 arc4-y !
  2drop
;

: arc4-enc ( src-addr dst-addr len )
  0 do
    arc4-x @ 1+ arc4-clip dup arc4-x !
    cells arc4-keyspace + @
    arc4-y @ + arc4-clip
    arc4-y ! 
    arc4-y @ arc4-swap-x-with
    2dup i arc4-xor-data
  loop 2drop
;

: arc4-dec arc4-enc ;

\ Random number generator specific:

0 var arc4-seeded

: arc4-seed ( -- )
  1 arc4-seeded !
  gettime +
  sp 1 cells - 4 arc4-setkey
  drop
;

: arc4-rand ( -- n )
  0 sp 1 cells - dup 4 arc4-enc
;

: arc4-choose ( max -- n )
  dup 2 < if ." CHOOSE DOES NOT ACCEPT NUMBERS BELOW 2" cr abort then
  arc4-seeded @ 0= if arc4-seed then
  arc4-rand swap mod abs
;
