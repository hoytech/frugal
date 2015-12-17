8000 const port
 0 var mainsock
 0 var clientsock


: init
  port openlistener
  dup 0= if ." UNABLE TO BIND TO PORT" die then
  mainsock !
;

: wipepad
    200 0 do 0 pad i + c! loop
;

: isget? ( addr -- B )
  dup c@ char, G = if
  char+ dup c@ char, E = if
  char+ dup c@ char, T = if
    drop 0
  then then then
  0=
;

: getreqinpad? ( -- B )
  pad 1-
  begin
    char+
    dup isget?
    over c@ 0=
  or until
  dup isget?
  if
    4 + pad 2 + 100 cmove 1
    char, . pad c! char, / pad 1+ c!  \ Store "./" in pad
  else drop 0 then
;

: wrcr 10 pad c! clientsock @ pad 1 write drop ;

: sendhead
  clientsock @
  dup s" HTTP/1.1 200 OK" write drop wrcr
  s" Server: FRUGAL FORTH HTTPD" write drop wrcr
  wrcr
;

: sendfile ( fd -- )
  sendhead
  begin
    wipepad
    dup pad 100 read dup
    0= if drop 1 else clientsock @ pad rot rot write 0= then
  until
;

\ FIXME: VITAL! Make these 2 not suck
: send404 ." 404" cr die ;
: any..s? 0 ;

: iswhite? ( c -- B )
  dup dup 0= rot
  10 = swap
  bl =
  or or
;

: nextbl ( addr -- addr+n )
  1- begin
    1+ dup
    c@ iswhite?
  until
;

: procgetreq
  pad nextbl 1- c@ char, / = if
    s" index.html " pad nextbl swap cmove
  then
  pad 100 type cr
  READ-ONLY pad open
  dup 0= if
    send404
  else
    any..s? if send404 else sendfile then
  then
;

: procclient
  begin
    wipepad
    clientsock @ pad 100 read
    getreqinpad? if procgetreq die then
  0= until
  clientsock @ close
  die
;

: procmain
  begin
    -1 mainsock @ poll drop
    mainsock @ acceptconn clientsock !
    fork
      if clientsock @ close else procclient then
  again
;

: setdir
  32 parse drop chdir 0= if ." CAN'T SWITCH TO DIR" cr die then
;

: httpd
  init
  procmain
;
