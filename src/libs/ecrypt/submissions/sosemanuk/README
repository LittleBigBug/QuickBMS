SOSEMANUK stream cipher reference implementation.

The SOSEMANUK stream cipher has been invented by Come Berbain, Olivier
Billet, Nicolas Courtois, Henri Gilbert, Louis Goubin, Aline Gouget,
Louis Granboulan, Cedric Lauradoux, Marine Minier, Thomas Pornin
and Herve Sibert. The reference implementation is intended to:
-- validate third-party implementations through detailed test
vectors;
-- allow easy cipher structure investigation;
-- demonstrate the implementation strategies which are considered
as appropriate by the inventors.

The reference implementation is free software; its license is as close
to Public Domain as any software license can be under French law.


The implementation consists of the following files:

ecrypt-config.h
ecrypt-machine.h
ecrypt-portable.h
ecrypt-sync.c
ecrypt-sync.h
sosemanuk.h
sosemanuk.c 

The first five files come from the API published by the ECRYPT NoE
for their call for stream cipher candidates. Only ecrypt-sync.h has
been slightly modified to match the SOSEMANUK implementation, as is
prescribed by that API.


The sosemanuk.h file contains a definition of a macro:

#define SOSEMANUK_ECRYPT

By removing this line, the implementation is made independant of the
ECRYPT API. The API implemented is then the one described in the
comments in the sosemanuk.h file. Only sosemanuk.c and sosemanuk.h files
are used when SOSEMANUK_ECRYPT is not defined.

Moreover, when SOSEMANUK_ECRYPT is not defined, the code can be compiled
with either the SOSEMANUK_VECTOR or the SOSEMANUK_SPEED macros defined.
If the SOSEMANUK_VECTOR macro is defined, the code compiles to a
stand-alone program which, when run, outputs two test vectors (with
details on the internal variable contents). If the SOSEMANUK_SPEED macro
is defined, the code compiles to a stand-alone program which, when run,
performs a computation speed benchmark. If neither of SOSEMANUK_VECTOR
nor SOSEMANUK_SPEED is defined, then this code compiles to library code
implementing the interface described in sosemanuk.h.


Portability has been actively sought. By default, the ECRYPT header
files are used to achieve that portability.
