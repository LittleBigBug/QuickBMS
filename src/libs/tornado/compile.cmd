@echo off
:: -DSTAT -DDEBUG -DFULL_COMPILE -DFREEARC_NO_TIMING

set libraries=user32.lib ole32.lib oleaut32.lib shell32.lib advapi32.lib
set gcc_libraries=-lshell32 -lole32 -loleaut32 -ladvapi32 -luuid
::t C:\Base\Compiler\MinGW\bin\g++.exe -DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE -O3 --param inline-unit-growth=99999 -funroll-loops -fno-exceptions -fno-rtti -fomit-frame-pointer -fstrict-aliasing -ffast-math -fforce-addr %* main.cpp -otor.exe %gcc_libraries% -s -Xlinker --large-address-aware

set defines=-DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE
set optimizations=-O3 -finline-limit=99999 --param inline-unit-growth=99999 -funroll-loops -fno-exceptions
::Comment out the next line in order to build 64-bit executable
set _32bit=-m32 -Xlinker --large-address-aware -otor.exe
::t g++.exe %defines% %optimizations% %* main.cpp -otor64.exe %gcc_libraries% -s -static %_32bit%

::call "C:\Program Files (x86)\Intel\Parallel Studio 2011\ips-vars.cmd" ia32
::call "C:\Program Files (x86)\Intel\Composer XE 2013 SP1\bin\ipsxe-comp-vars.bat" ia32
::t icl -W0 -DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE -O3 -Gy -Gr -GL -Qipo -Qinline-factor9999 %* main.cpp %libraries% /Fetor.exe /link /LARGEADDRESSAWARE

call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
::call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86
::call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
t cl -DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE -Ox -GL -Gy %* main.cpp %libraries% /Fetor.exe /link /SUBSYSTEM:CONSOLE,5.01 /LARGEADDRESSAWARE


:: 64-bit compilers :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

::call "C:\Program Files (x86)\Intel\Parallel Studio 2011\ips-vars.cmd" ia32_intel64
::call "C:\Program Files (x86)\Intel\Composer XE 2013 SP1\bin\ipsxe-comp-vars.bat" intel64
::t icl -W0 -DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE -O3 -Gy -GL -Qipo -Qinline-factor9999 %* main.cpp %libraries% /Fetor64.exe

::call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86_amd64
::call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86_amd64
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64
t cl -DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE -Ox -GL -Gy %* main.cpp %libraries% /Fetor64.exe /link /SUBSYSTEM:CONSOLE,5.02
