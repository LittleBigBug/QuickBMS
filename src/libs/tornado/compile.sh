g++ -DFULL_COMPILE -DFREEARC_UNIX -DFREEARC_INTEL_BYTE_ORDER -D_FILE_OFFSET_BITS=64 -O3 --param inline-unit-growth=999999 -funroll-loops -fno-exceptions -fno-rtti -Wno-unknown-pragmas -Wno-sign-compare -Wno-conversion -fomit-frame-pointer -fstrict-aliasing -ffast-math -fforce-addr $* main.cpp -otor -lrt -s
# -DSTAT -DDEBUG -DFULL_COMPILE
