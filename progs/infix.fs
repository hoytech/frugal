\ This is Frugal Forth's infix notation system, primarily to
\ demonstrate that, yes, infix is possible in forth: Forth programmers
\ just choose not to use it. :) 

\ Notice that these words work only in compilation mode, and only when the
\ second argument is a literal.

\ Example:
\ : my-word 4 <+> 8 ;
\ my-word .
\ 12

\ : double <*> 2 ;
\ 32 double
\ 64

: do-infix num' #, , ;

: <+> do-infix [ compile + ] ; immediate compile-only
: <-> do-infix [ compile - ] ; immediate compile-only
: <*> do-infix [ compile * ] ; immediate compile-only
: </> do-infix [ compile / ] ; immediate compile-only
