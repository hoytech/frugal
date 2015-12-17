: lookup pad 32 parse drop dnstoip
  if pad 16 type else ." NOT FOUND" then cr ;

: rlookup pad 100 32 parse drop iptodns
  if pad 100 type else ." NOT FOUND" then cr ;
