cl /O2 /EHsc /I.\ lzw-enc.c encoder.c
cl /O2 /EHsc /I.\ lzw-dec.c decoder.c
lib /out:lzw.lib lzw-enc.obj lzw-dec.obj
