// modified by Luigi Auriemma

/*
    REP is an LZ77-family algorithm, i.e. it founds matches and outputs them as
    (len,offset) pairs. It is oriented toward very fast compression and small
    memory overhead (1/4 of buffer size), but limited to rather large values of
    mimimal match length (say, 32), and don't search for optimum match. It's
    intended to preprocess data before using full-fledged compressors. and in
    this area it beats RZIP and, to some degree, LZP preprocessors. Small
    memory overhead means that RZIP/LZP/REP are capable to find matches at very
    long distances and this algorithm does it much better than RZIP and LZP.
    The algorithm implemented in functions REPEncode() and REPDecode().

    Main differences comparing to RZIP:
    1) Sliding window which slides at 1/16 of buffer size each time
    2) Almost ideal hash function (see update_hash)
    3) Direct hashing without hash chains which 1.5x cuts memory requirements
    4) Tags are not saved in hashtable, which again halves memory requirements.
         Instead, a few lower bits of hash table entry are used to save a few
         bits of tag (see chksum)
    5) Hash size is proportional to buffer size (which is equal to the maximum
         search distance) and by default limited to 1/4 of buffer size
    6) In order to find strings of length >=MinLen, blocks of length L=MinLen/2
         are indexed via hash. Of all those possible blocks, only 1/sqrt(L) are
         indexed and only 1/sqrt(L) are searched. It is alternative to solution
         described in RZIP paper where 1/L of blocks are indexed and each block
         searched. This means that logb(sqrt(L)) lower bits of hash entry are
         zeroes which allows to use trick 4.


References for RZIP algorithm explanation and implementations:
http://samba.org/~tridge/phd_thesis.pdf
http://rzip.samba.org/ftp/rzip/rzip-2.1.tar.gz
http://ck.kolivas.org/apps/lrzip/lrzip-0.18.tar.bz2
http://www.edcassa-ict.nl/lrzip.zip
http://www.edcassa-ict.nl/rzip21.zip

TAYLOR, R., JANA, R., AND GRIGG, M. 1997. Checksum testing of remote
synchronisation tool. Technical Report 0627 (November), Defence Science and
Technology Organisation, Canberra, Australia. (p.72)


References for LZP algorithm implementations:
http://magicssoft.ru/content/download/GRZipII/GRZipIISRC.zip
http://www.compression.ru/ds/lzp.rar


** Detailed algorithm description in Russian **********************************************

    Этот алгоритм является разновидностью LZ77, т.е. он находит повторяющиеся
    строки во входных данных, и кодирует их как (len,offset). Его особенностью
    является ориентация на поиск совпадений достаточно большой длины на большом
    расстоянии. Поэтому он весьма эффективно использует память - как правило,
    для структур поиска требуется не более 25% от размера окна поиска. При этом
    он находит практически все совпадения если минимальная длина (MinLen)
    искомых строк - 512 байт, и порядка 98% - в одном моём эксперименте на
    поиск совпадений с длиной от 32 байт. На практике этот алгоритм нацелен на
    использование в качестве препроцессора, уменьшающего избыточность файла
    и/или находящего сопадения на таких дистанциях, которые недоступны
    основному алгоритму упаковки, и в этом качестве он конкурирует с такими
    алгоритмами, как LZP by Ilya Grebnev и RZIP. При этом, как показывают
    эксперименты, для препроцессора оптимальная величина минимальной искомой
    строки находится именно в этих пределах - 32-512 байт. Этот алгоритм
    находит куда больше совпадений, чем LZP/RZIP, и кроме того, его
    скорость работы увеличивается при увеличении MinLen.

    Алгоритм реализуется функциями REPEncode() и REPDecode(), и использует
    сочетание идей из LZP, RZIP и моих собственных. Поиск совпадений ведётся в
    скользящем окне - входные данные считываются блоками по 1/16 от размера
    буфера, и это означает что в любой момент времени как минимум 15/16 буфера
    содержат предыдущие данные, которые сканируются в поисках совпадений. Для
    упрощения алгоритма ни входные блоки, ни совпадения не могут пересекать
    границу буфера.

    Как обычно, для поиска строк с длиной от MinLen у каждого блока файла
    длиной MinLen вычисляется некая контрольная сумма (КС), которая заносится в
    хеш-таблицу. Поскольку алгоритм ориентирован на большие значения MinLen,
    быстрое вычисление КС от блоков такой длины является проблемой. Эта
    проблема решается использованием "скользящей КС", то есть такой, которую
    можно быстро пересчитать при добавлении нового байта в конец блока и
    удалении одного байта в начале (см. update_hash).

    Подбор наилучшей формулы для скользящего хеширования был отдельным
    приключением. В конце концов простая формула hash = p[-1] + PRIME*p[-2] +
    PRIME*PRIME*p[-3] + ..., где PRIME - простое число, оказалась самой быстрой
    и дающей весьма равномерное распределение. Разумеется, все вычисления идут
    по модулю 1<<32, любезно предоставленному нам процессором :)

    Далее, были использованы дополнительные меры для уменьшения требований к
    памяти и увеличения скорости. Рассмотрим к примеру работу алгоритма для
    MinLen=512. Поскольку любой 512-байтный блок включает в себя 256-байтный
    блок, начинающийся с позиции, кратной 256, то нам достаточно вставлять в
    хеш-таблицу ссылки только на эти блоки и искать совпадение только с ними.
    Разумеется, при проверке совпадения мы не ограничиваемся в точности 256
    байтами, а пытаемся продолжить его как можно дальше в обе стороны. Именно
    это и позволяет значительно уменьшить расход памяти при гарантированном
    нахождения почти всех совпадений - во всяком случае, когда MinLen
    достаточно велико.

    Однако можно пойти ещё дальше - вместо того, чтобы вставлять в хеш-таблицу
    каждый 256-й блок, но искать каждый-каждый, мы можем например вставлять
    каждый 32-й, а искать каждый 8-й, или вставлять каждый 2-й, а искать каждый
    128-й. Разумеется, оптимумом будет вставлять и искать каждый 16-й блок.
    Точнее говоря, нужно вставлять один блок через каждые 16 байт, а искать
    первые 16 блоков из каждых 256, то есть вставляем блоки, начинающиеся с
    позиций 0, 16, 32..., а ищем блоки, начинающиеся с позиций 0, 1, 2..., 15,
    256. 257... Таким образом, для MinLen=512 достигается 8-кратное ускорение
    работы (за счёт 8-кратного уменьшения количества обращений в память) по
    сравнению с прямолинейной реализацией - правда, за счёт увеличения
    требований к памяти (с 1/64 размера буфера до 1/4, что на мой взгляд вполне
    приемлемо).

    Наконец, последним трюком является использование младших битов записи в
    хеш-таблице для хранения нескольких бит из значения хеш-функции (chksum) -
    разумеется, тех, которые не являются частью индекса в хеш-таблице. Это
    позволяет отсеять большую часть ложных совпадений, не сравнивая содержимое
    блоков, и тем самым уменьшить количество обращений в память и ещё больше
    ускорить работу программы.

    В алгоритме используется хеширование с прямой адресацией, без вторичного
    хеширования, что делает реализацию очень простой. Значение хеш-функции
    от 256-байтного блока (в общем случае размер этого блока - L=MinLen/2)
    используется как индекс в хеш-таблице (hasharr[hash&HashMask]), при
    коллизиях новый блок просто заменяет более ранний. На практике это
    (практически) не ведёт к деградации компрессии. Ещё раз подчеркну, что
    этот алгоритм, в отличие от полноценных LZ77 реализаций, ищет не
    оптимальное (самое длинное) совпадение, а проверяет лишь одну ссылку - на
    последний блок, который занял этот хеш-слот, и чья КС, следовательно,
    предположительно совпадает с КС текущего блока.

    Размер хеша (HashSize): при разработке алгоритма я предполагал, что он
    должен быть в 2-4 раза больше количества элементов, которые в него придётся
    вставлять. Однако на практике оказалось, что вполне достаточно иметь то же
    самое кол-во слотов, а для MinLen=32 - даже вчетверо (!) меньшее. то есть,
    например, для 32 мб блока при MinLen=512 в хеш вставляется каждый 16-й
    256-байтный блок и общее количество вставляемых элементов - 32млн/16=2млн,
    т.е. 8 мб, и хеш создаётся именно такого размера. Для MinLen=32 общее
    количество элементов 32млн/4=8млн, но мы создаём хеш-таблицу вчетверо
    меньше, то есть получаются те же самые 8 мб. Таким образом, подбираемый
    алгоритмом автоматически размер хеш-таблицы никогда не превосходит 1/4
    размера входного буфера. Если вы хотите установить другое значение - то
    используйте параметр HashBits (опцию -h). Увеличение HashSize при небольших
    MinLen способно немного увеличить степень сжатия.

    Amplifier: как было описано выше, при поиске проверяется только часть
    блоков, которой бы с гарантией хватило для нахождения всех строк с длиной
    >=MinLen - будь у нас идеальное хеширование. Однако наше хеширование
    неидеально, и часть потенциальных хитов из-за этого теряется. Параметр
    Amplifier (опция -a) позволяет затребовать тестирование большего числа
    блоков (в эти самые Amplifier раз). Таким образом, для максимально
    тщательного поиска можно просто установить Amplifier в достаточно большое
    значение, скажем 99. Разумеется, это уменьшает скорость и лишь ненамного
    увеличивает сжатие.

    Barrier и SmallestLen: некоторые алгоритмы, в частности ppmd, выигрывают,
    если препроцессор использует меньшее значение MinLen для больших
    дистанций. Эти два параметра позволяют установить двухступенчатую границу
    отбора совпадений, например "в первом мегабайте - MinLen=128, далее
    MinLen=32" задаётся через MinLen=128, Barrier=1<<20, SmallestLen=32
    (опции -l128 -d1048576 -s32). При этом поиск строк настраивается, ес-но,
    на нахождение строк с длиной от SmallestLen вместо MinLen.


** Benchmarks using 1GHz processor ****************************************************************

Test results for 26mb:
        Compression time   Compressed size
-l8192  0.5 seconds
 -l512  1.1
 -l128  1.4
  -l32  2.5                12.7 mb
lrzip   2.6                14.1
lzp:h20 6.5                13.1
lzp:h13 3.0                20.6

Compression speed on uncompressible data:
-l8192  52 mb/sec
 -l512  25 mb/sec
 -l128  17 mb/sec
  -l32   8 mb/sec
lrzip    8 mb/sec

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>



typedef int unrep_io_func (/*void* param,*/ void* buf, int size);



unsigned char   *unrep_in,
                *unrep_inl,
                *unrep_out,
                *unrep_outl;

int unrep_fread(void *buf, int size) {
    if(size > (unrep_inl - unrep_in)) size = unrep_inl - unrep_in;
    memcpy(buf, unrep_in, size);
    unrep_in += size;
    return(size);
}
int unrep_fwrite(void *buf, int size) {
    if(size > (unrep_outl - unrep_out)) size = unrep_outl - unrep_out;
    memcpy(unrep_out, buf, size);
    unrep_out += size;
    return(size);
}



static inline void unrep_copymem (byte* p, byte* q, unsigned len)
{
    if (len)
    do *p++ = *q++;
    while (--len);
}


// Classical LZ77 decoder with sliding window
int rep_decompress (int BlockSize, int MinCompression, int MinMatchLen, int Barrier, int SmallestLen, int HashBits, int Amplifier, unrep_io_func* In, void* InParam, unrep_io_func* Out, void* OutParam)
{
    byte *last_data;
    int i;
    // Фактический размер буфера сохранён во входных данных
    if (In (/*InParam,*/ &BlockSize, sizeof(int32)) != sizeof(int32))   return -1;  // Error: can't read input data
    byte *data = (byte*) malloc (BlockSize), *data0=data;
    if (data==NULL)  return -1;                             // Error: not enough memory
    //stat ("Decompression",0);

    // Цикл, каждая итерация которого обрабатывает один блок сжатых данных
    for (last_data=data; ; last_data=data) {

        // Прочитаем один блок сжатых данных
        int ComprSize;
        if (In (/*InParam,*/ &ComprSize, sizeof(int32)) != sizeof(int32))   return -1;  // Error: can't read input data
        if (ComprSize == 0)  break;    // EOF flag (see above)

        byte *buf = (byte*) malloc(ComprSize), *buf0=buf;   // Буфер, куда будут помещены входные данные
        if (buf==NULL)           return(-1);       // Error: not enough memory
        int Size = In (/*InParam,*/ buf, ComprSize);
        if (Size != ComprSize)   return(-1);       // Error: can't read input data

        // Заголовок блока содержит размер таблиц lens/offsets/datalens; затем идут сами эти таблицы и наконец несжавшиеся данные
        int         num = *(int32*)buf;  buf += sizeof(int32);           // Количество совпадений (= количеству записей в таблицах lens/offsets/datalens)
        int32*     lens =  (int32*)buf;  buf += num*sizeof(int32);
        int32*  offsets =  (int32*)buf;  buf += num*sizeof(int32);
        int32* datalens =  (int32*)buf;  buf += (num+1)*sizeof(int32);   // Точнее, datalens содержит num+1 записей

        // Каждая итерация этого цикла копирует один блок несжатых данных и один match, которые interleaved в нашей реализации процеса упаковки
        for (i=0; i<num; i++) {
            memcpy (data, buf, datalens[i]);  buf += datalens[i];  data += datalens[i];
            //debug (verbose>1 && printf ("Match %d %d %d\n", -offsets[i], data-data0, lens[i]));
            // Если смещение попадает за начало буфера, то вычесть из него BlockSize, чтобы "обернуться" вокруг границы буфера
            int offset = offsets[i] <= data-data0 ?  offsets[i] : offsets[i]-BlockSize;
            unrep_copymem (data, data-offset, lens[i]);  data += lens[i];
        }
        // Плюс ещё один блок несжавшихся данных в самом конце (возможно, нулевой длины)
        memcpy (data, buf, datalens[num]);  buf += datalens[num];  data += datalens[num];

        // Вывод распакованных данных, печать отладочной статистики и подготовка к следующей итерации цикла
        Out (/*OutParam,*/ last_data, data-last_data);
        //debug (verbose>0 && printf( " Decompressed: %u => %u bytes\n", ComprSize+sizeof(int32), data-last_data) );
        //stat ("Decompression", data-last_data);
        if (data==data0+BlockSize)  data=data0;
        free(buf0);
        // NB! check that buf==buf0+Size, data==data0+UncomprSize, and add buffer overflowing checks inside cycle
    }
    free(data0);
    return 0;
}



int unrep(unsigned char *in, int insz, unsigned char *out, int outsz) {
    int bufsize=1<<27, mincompr=100, minlen=512, small_len=0, barrier=8<<20, hashbits=0, amplifier=1;

    unrep_in    = in;
    unrep_inl   = in + insz;
    unrep_out   = out;
    unrep_outl  = out + outsz;
    rep_decompress (bufsize, mincompr, minlen, barrier, small_len, hashbits, amplifier, unrep_fread, in, unrep_fwrite, out);
    return(unrep_out - out);
}

