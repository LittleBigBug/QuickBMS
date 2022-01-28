// modified by Luigi Auriemma

// http://freearc.org/download/research/dict.zip
/*

DICT - алгоритм словарной замены. Построение словаря осуществляется
       во время работы программы, использованный словарь выводится
       перед закодированными данными
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef unsigned char byte;
#define USE_DICT2                    1

#define dict_put_byte(c)      (*outptr++ = (c))

#define dict2(i,j)       (dict2_var [i*(UCHAR_MAX+1) + j])
#define dict_len(i)      (dict[i]? dict[i]->len : 0)
#define dict_ptr(i)      (dict[i]->ptr)
#define dict2_len(i,j)   (dict2(i,j)? dict2(i,j)->len : 0)
#define dict2_ptr(i,j)   (dict2(i,j)->ptr)

// РАСПАКОВКА *****************************************************************************

// Словарь-матрица, используемый для декодирования
typedef struct
{
    unsigned len;     // длина слова
    byte *ptr;        // начало слова
} dict_entry;
static dict_entry dict[UCHAR_MAX+1];

#define dict_get_byte()       (*ptr++)
#define dict_put_byte(c)      (*outptr++ = (c))
#define dict_put_word(p,len)  (memcpy (outptr, (p), (len)), outptr += (len))

// Распаковать входные данные buf[bufsize] в outbuf и возвратить размер распакованных данных
int DictDecode (byte *buf, unsigned bufsize, byte *outbuf, unsigned *outsize)
{
    int retcode = 0;
    byte *ptr = buf,
         *end = buf+bufsize,
         *outptr = outbuf;     // текущий указатель в выходном буфере

    // Словарь-матрица, используемый для декодирования 2-байтовых слов
    dict_entry *dict2_var = (dict_entry*) calloc ((UCHAR_MAX+1)*(UCHAR_MAX+1), sizeof (dict_entry));

    //stat1 ("ЧТЕНИЕ СЛОВАРЯ");
    // Состоит из 5 циклов:
    //   1. Прочесть 256 байт - это длины всех слов, кодируемых одним байтом
    //        (0 означает, что этот байт слов не кодирует, 1 означает, что с этого байта начинаются коды 256 слов)
    //   2. Прочесть длины всех слов, кодируемых двумя байтами
    //        (256*n байт, где n - количество единиц, прочитанных на предыдущем этапе)
    //   3. Прочесть текст всех однобайтовых слов
    //   4. Прочесть текст всех двухбайтовых слов
    //   5. Создать псевдо-слова для декодирования тех символов, которые отдали свои коды словам
    int dictsize = 0, words2 = 0;
    int i, j, k;
    dict_entry *d;

    for( i=0; i<=UCHAR_MAX; i++ ) {
        dictsize += dict[i].len = dict_get_byte();
    }
    for( i=0; i<=UCHAR_MAX; i++ ) {
        if( dict[i].len==USE_DICT2 ) {
            for( j=0; j<=UCHAR_MAX; j++ ) {
                dictsize += dict2(i,j).len = dict_get_byte();
                words2++;
            }
        }
    }
    // Буфер для хранения текста слов (память для него сейчас выделяется на авось, но с большим запасом :)
    byte *words = (byte*) malloc (dictsize+UCHAR_MAX+1+words2*20+100000), *wordptr = words;
    for( i=0; i<=UCHAR_MAX; i++ ) {
        if (dict[i].len == USE_DICT2)  continue;
        dict[i].ptr = wordptr;
        for( k=0; k<dict[i].len; k++ ) {
            *wordptr++ = dict_get_byte();
        }
    }
    {
    byte word_sep = dict_get_byte();
    byte *prevptr = NULL;
    for( i=0; i<=UCHAR_MAX; i++ ) {
        if( dict[i].len==USE_DICT2 ) {
            for( j=0; j<=UCHAR_MAX; j++ ) {
                dict2(i,j).ptr = wordptr;
                // Скопируем начало слова из предыдущего
                for( k=0; k<dict2(i,j).len; k++ ) {
                    if (prevptr==NULL)  {retcode = -1; goto done;}  // Ошибка во входных данных - копирование данных из предыдущего слова, которого нет :)
                    *wordptr++ = *prevptr++;
                }
                // И прочитаем остаток слова из входного потока
                for(;;) {
                    byte c = dict_get_byte();
                    if (c==word_sep) break;
                    *wordptr++ = c;
                }
                dict2(i,j).len = wordptr - dict2(i,j).ptr;
                prevptr = dict2(i,j).ptr;
            }
        }
    }
    // Префикс, используемый для кодирования украденных символов
    byte prefix = dict_get_byte();
    dict[prefix].len = USE_DICT2;
    // Создадим псевдо-слова, кодирующие украденные символы
    for (j=0; j<=UCHAR_MAX; j++) {
        dict2(prefix,j).len = 1;
        dict2(prefix,j).ptr = wordptr;
        *wordptr++ = (byte)j;
    }


    //stat1 ("ДЕКОДИРОВАТЬ ТЕКСТ, ИСПОЛЬЗУЯ ПРОЧИТАННЫЙ ВЫШЕ СЛОВАРЬ");
    while( ptr<end ) {
        byte c = dict_get_byte();
        d = &dict[c];

        // Если этот байт не кодирует никаких слов, то вывести его сам по себе
        if (d->len == 0) {
            dict_put_byte(c);

        // Если этот байт - начало кода двухбайтового слова, то вывести это слово
        } else if( d->len == USE_DICT2 ) {
            byte c2 = dict_get_byte();
            d = &dict2(c,c2);
            dict_put_word (d->ptr, d->len);

        // Иначе этот байт - начало кода однобайтового слова
        } else {
            dict_put_word (d->ptr, d->len);
        }
    }
    }
done:
    free(words);
    free(dict2_var);

    // Записать длину декодированного текста и вернуть код (без)успешного завершения
    *outsize = outptr-outbuf;
    return retcode;
}

