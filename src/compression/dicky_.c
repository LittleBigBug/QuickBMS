// modified by Luigi Auriemma
// Note that this is not a real compression

/* Dicky - public domain */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "dicky_p.h"
#include "dicky.h"

static void lolfsm_reset(LOLFSM * const fsm)
{
    memset(fsm, 0, sizeof *fsm);
}

static unsigned char lookup(const int c)
{
    assert((size_t) c < sizeof ctable / sizeof ctable[0]);
    
    return (unsigned char) ctable[(size_t) c];   
}

static int lolfsm_round(LOLFSM * const fsm, int c)
{
    switch (c) {
    case 'l':
        if (fsm->seen_o) {
            lolfsm_reset(fsm);
            return C_LOL;
        }
        fsm->seen_l = 1;
        return C_HOLD;
    case 'o':
        if (fsm->seen_l) {
            fsm->seen_o = 1;
            return C_HOLD;
        }
        break;
    case 'm':
        fsm->seen_m = 1;
        return C_HOLD;
    case 'd':
        if (fsm->seen_m) {
            fsm->seen_d = 1;
            return C_HOLD;
        }
        break;
    case 'r':
        if (fsm->seen_d) {
            lolfsm_reset(fsm);
            return C_MDR;            
        }
        break;
    case 'b':
        fsm->seen_b = 1;
        return C_HOLD;
    case 'i':
        if (fsm->seen_b) {
            fsm->seen_i = 1;
            return C_HOLD;
        }
        break;
    case 't':
        if (fsm->seen_i) {
            fsm->seen_t = 1;
            return C_HOLD;
        }
        break;
    case 'e':
        if (fsm->seen_t) {
            lolfsm_reset(fsm);
            return C_BITE;            
        }
    }
    lolfsm_reset(fsm);
    
    return (int) lookup(c);
}

static int init_input_buffer(InputBuffer * const input_buffer,
                             const unsigned char *buffer,
                             const size_t sizeof_buffer)
{
    assert(input_buffer != NULL);
    assert(sizeof_buffer > (size_t) 0U);
    input_buffer->buffer = buffer;
    input_buffer->pos = (size_t) 0U;    
    input_buffer->sizeof_buffer = sizeof_buffer;
    input_buffer->quartet = 0;
    
    return 0;
}

static int init_output_buffer(OutputBuffer * const output_buffer,
                              const size_t chunk_size)
{
    assert(chunk_size > (size_t) 0U);
    output_buffer->buffer = NULL;
    output_buffer->pos = (size_t) 0U;
    output_buffer->sizeof_buffer = (size_t) 0U;
    output_buffer->chunk_size = chunk_size;
    output_buffer->quartet = 0;
    
    return 0;
}

static void free_output_buffer(OutputBuffer * const output_buffer)
{
    if (output_buffer == NULL) {
        return;
    }
    free(output_buffer->buffer);
    output_buffer->buffer = NULL;
}

static int append_byte_to_output_buffer(OutputBuffer * const output_buffer,
                                        const unsigned char c)
{
    if (output_buffer->pos >= output_buffer->sizeof_buffer) {
        size_t wanted_size;
        unsigned char *new_buffer;
        
        #define SIZE_T_MAX 0x7fffffff
        if (SIZE_T_MAX - output_buffer->chunk_size <
            output_buffer->sizeof_buffer) {
            return -1;
        }
        wanted_size = output_buffer->sizeof_buffer + output_buffer->chunk_size;
        new_buffer = realloc(output_buffer->buffer, wanted_size);
        if (new_buffer == NULL) {
            return -1;
        }
        memset(new_buffer + output_buffer->sizeof_buffer, 0, wanted_size - output_buffer->sizeof_buffer);
        output_buffer->buffer = new_buffer;
        output_buffer->sizeof_buffer = wanted_size;
    }
    assert(output_buffer->pos < output_buffer->sizeof_buffer);
    output_buffer->buffer[output_buffer->pos++] = c;
    
    return 0;
}

static int append_quartet_to_output_buffer(OutputBuffer * const output_buffer,
                                           const unsigned char c)
{
    assert(c <= 0xF);
    if (output_buffer->quartet != 0) {
        assert((output_buffer->buffer[output_buffer->pos] & 0xF) == 0U);
        assert(output_buffer->pos > (size_t) 0U);
        output_buffer->buffer[output_buffer->pos - (size_t) 1U] |= c;
        output_buffer->quartet = 0;
    } else {
        if (append_byte_to_output_buffer(output_buffer, c << 4) != 0) {
            return -1;
        }
        output_buffer->quartet = 1;
    }
    return 0;
}

static int emit(OutputBuffer * const output_buffer, const unsigned char c)
{
    return append_quartet_to_output_buffer(output_buffer, c);
}

static int pad_output_buffer(OutputBuffer * const output_buffer)
{
    if (output_buffer->quartet == 0) {
        return 0;
    }
    return emit(output_buffer, C_SPACE);
}

static int get_quartet_from_input_buffer(InputBuffer * const input_buffer)
{
    unsigned char c;
    assert(input_buffer->pos <= input_buffer->sizeof_buffer);

    if (input_buffer->pos >= input_buffer->sizeof_buffer) {
        return EOF;
    }
    c = input_buffer->buffer[input_buffer->pos];
    if (input_buffer->quartet == 0) {
        input_buffer->quartet = 1;
        return (int) ((c >> 4) & 0xF);
    }
    input_buffer->quartet = 0;
    input_buffer->pos++;
    
    return (int) (c & 0xF);
}

static int unhold(OutputBuffer * const output_buffer,
                  const char * const start_held,
                  const char * const end_held)
{
    const char *pnt_held = start_held;
    
    assert(start_held != NULL);
    assert(start_held != end_held);
    do {
        if (emit(output_buffer, lookup(*pnt_held)) != 0) {
            return -1;
        }
    } while (++pnt_held != end_held);
       
    return 0;
}

static unsigned int get_rand(unsigned int max)
{
    //return (unsigned int) (random() % (long) max);

    // you have to call srand(...) first
    return (unsigned int) (rand() % (long) max);
}

int dicky_compress(unsigned char ** const target, size_t * const target_size,
                   const char * const source, const size_t source_size)
{
    const char *pnt_source = source;
    const char * const end_source = pnt_source + source_size;
    const char *pnt_held = NULL;
    OutputBuffer output_buffer;
    LOLFSM fsm;
    int compressed_char;
    int c;

    *target = NULL;
    *target_size = (size_t) 0U;
    if (init_output_buffer(&output_buffer, source_size) != 0) {
        return -1;
    }
    lolfsm_reset(&fsm);
    while (pnt_source != end_source) {
        c = (int) (unsigned char) *pnt_source;
        compressed_char = lolfsm_round(&fsm, c);
        if (compressed_char == C_HOLD) {
            if (pnt_held == NULL) {
                pnt_held = pnt_source;
            }
        } else {
            if (pnt_held != NULL && compressed_char != C_LOL &&
                compressed_char != C_MDR && compressed_char != C_BITE) {
                unhold(&output_buffer, pnt_held, pnt_source);
            }
            pnt_held = NULL;            
            if (emit(&output_buffer, (unsigned char) compressed_char) != 0) {
                free_output_buffer(&output_buffer);
                return -1;
            }
        }
        pnt_source++;
    }
    if (pad_output_buffer(&output_buffer) != 0) {
        free_output_buffer(&output_buffer);
        return -1;
    }
    *target = output_buffer.buffer;
    *target_size = output_buffer.pos;
    
    return 0;
}

static int uncompress_special(OutputBuffer * const output_buffer, const int c)
{
    if (c == C_LOL) {
        if (append_byte_to_output_buffer(output_buffer, 'L') != 0 ||
            append_byte_to_output_buffer(output_buffer, 'O') != 0 ||
            append_byte_to_output_buffer(output_buffer, 'L') != 0) {
            return -1;
        }
        return 1;
    }
    if (c == C_MDR) {
        if (append_byte_to_output_buffer(output_buffer, 'M') != 0 ||
            append_byte_to_output_buffer(output_buffer, 'D') != 0 ||
            append_byte_to_output_buffer(output_buffer, 'R') != 0) {
            return -1;
        }
        return 1;
    }
    if (c == C_BITE) {
        if (append_byte_to_output_buffer(output_buffer, 'b') != 0 ||
            append_byte_to_output_buffer(output_buffer, 'i') != 0 ||
            append_byte_to_output_buffer(output_buffer, 't') != 0 ||
            append_byte_to_output_buffer(output_buffer, 'e') != 0) {
            return -1;
        }
        return 1;
    }
    return 0;
}

static int uncompress_space(OutputBuffer * const output_buffer,
                            signed char * const seen_space,
                            int * const uncompressed_char)
{
    if (*seen_space != 0) {
        switch ((int) get_rand(5U)) {
        case 0:
            if (append_byte_to_output_buffer(output_buffer, '\n') != 0) {
                return -1;
            }
            break;
        case 1:
            if (append_byte_to_output_buffer(output_buffer, '.') != 0 ||
                append_byte_to_output_buffer(output_buffer, '.') != 0 ||
                append_byte_to_output_buffer(output_buffer, '.') != 0 ||
                append_byte_to_output_buffer(output_buffer, ' ') != 0) {
                return -1;
            }
            break;
        case 2:
            if (append_byte_to_output_buffer(output_buffer, ' ') != 0) {
                return -1;
            }
            break;
        case 3:
            if (append_byte_to_output_buffer(output_buffer, '!') != 0 ||
                append_byte_to_output_buffer(output_buffer, ' ') != 0) {
                return -1;
            }
            break;
        case 4:
            if (append_byte_to_output_buffer(output_buffer, '.') != 0 ||
                append_byte_to_output_buffer(output_buffer, ' ') != 0) {
                return -1;
            }
        }
        *seen_space = 0;
        
        return 1;
    } else {
        *uncompressed_char = ' ';
        *seen_space = 1;                
    }
    return 0;
}

int dicky_uncompress(char ** const target, size_t * const target_size,
                     const unsigned char * const source,
                     const size_t source_size)
{
    InputBuffer input_buffer;    
    OutputBuffer output_buffer;
    int c;
    int ret;
    int uncompressed_char;
    const unsigned char *row;
    signed char seen_space = 0;

    *target = NULL;
    *target_size = (size_t) 0U;
    if (init_input_buffer(&input_buffer, source, source_size) != 0) {
        return -1;
    }    
    if (init_output_buffer(&output_buffer, source_size *
                           (size_t) AVG_COMPRESSION_RATIO) != 0) {
        return -1;
    }
    while ((c = get_quartet_from_input_buffer(&input_buffer)) != EOF) {
        ret = uncompress_special(&output_buffer, c);
        if (ret < 0) {
            return -1;
        }
        if (ret > 0) {
            continue;
        }
        if (c == C_SPACE) {
            ret = uncompress_space(&output_buffer, &seen_space,
                                   &uncompressed_char);
            if (ret < 0) {
                return -1;
            }
            if (ret > 0) {
                continue;
            }
        } else {
            seen_space = 0;
            row = dtable[c];
            uncompressed_char =
                row[get_rand((unsigned int) strlen((const char *) row))];
        }
        if (append_byte_to_output_buffer(&output_buffer,
                                         uncompressed_char) != 0) {
            return -1;
        }
    }
    *target = (char *) output_buffer.buffer;
    *target_size = output_buffer.pos;
    
    return 0;
}

void dicky_free(void * const buffer)
{
    free(buffer);
}
