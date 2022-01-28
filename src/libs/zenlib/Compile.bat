@echo off
color b
\masm32\bin\ml /c /coff /Cp ZenLib.asm
\masm32\bin\link /DLL /DEF:ZenLib.def /SUBSYSTEM:WINDOWS /LIBPATH:\masm32\lib ZenLib.obj
pause