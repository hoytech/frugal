\ tea.f
\
\ Implementation of the TEA algorithm in forth (assuming cell = 4)
\
\ Robert Oestling, 2002-05-18
\ robost@telia.com

\ First of all, you have to set the key. It consists of 4 32-bit
\ integers, key0 is the least significant and key3 the most significant
\ dword. The word tea.set-key ( key0 key1 key2 key3 -- ) is used to set
\ the key.

\ To encrypt a 64-bit block of data, simply push it (most significant
\ dword first, then the least significant dword), and call tea.encrypt
\ Do the same to decrypt, but use tea.decrypt

\ Tested on IsForth 1.05b
\ [Fractal: And Frugal 0.9.7]

\ [Fractal: These next 2 definitions added by me for Frugal]
: u>> urshift ;
: u<< ulshift ;

base @          \ save the current number system base

hex             \ let's use hex

variable key0
variable key1
variable key2
variable key3
variable sum

9e3779b9 constant delta
20 constant iterations

: tea.transform0 ( z -- x )
        dup dup
        4 u<< key0 @ +
        rot
        5 u>> key1 @ +
        rot
        sum @ +
        xor xor
        ;

: tea.transform1 ( y -- x )
        dup dup
        4 u<< key2 @ +
        rot
        5 u>> key3 @ +
        rot
        sum @ +
        xor xor
        ;

: tea.encrypt ( y z -- y z )
        swap
        0 sum !
        iterations 0 do
                delta sum +!
                over tea.transform0 + ( z new-y )
                swap over ( new-y z new-y )
                tea.transform1 + ( new-y new-z )
                swap
        loop swap ;

: tea.decrypt ( y z -- y z )
        delta 5 u<< sum !
        iterations 0 do
                over tea.transform1 - ( y new-z )
                swap over ( new-z y new-z )
                tea.transform0 - ( new-z new-y )
                swap
                sum @ delta - sum !
        loop ;
 
: tea.set-key ( key0 key1 key2 key3 -- )
        key3 !
        key2 !
        key1 !
        key0 !
        ;

base !          \ restore the saved base


