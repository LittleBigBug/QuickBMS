gcc -O2 -D PRINTON -c GLZAcomp.c GLZAformat.c GLZAcompress.c GLZAencode.c GLZAdecode.c GLZAmodel.c -static -lpthread
g++ -O2 -D PRINTON -c GLZA.c
g++ -O2 -D PRINTON -o GLZA32 GLZA.o GLZAcomp.o GLZAformat.o GLZAcompress.o GLZAencode.o GLZAdecode.o GLZAmodel.o -static -lpthread