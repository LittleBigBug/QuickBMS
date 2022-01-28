// common functions and defines for algorithms created by Mark Nelson
// made by Luigi Auriemma

#include <stdio.h>
#include <stdlib.h>
#include "mn_incs.h"

int mn_getc(memory_file *x) {
    if(x->data >= x->datal) return(EOF);
    return(*x->data++);
}
int mn_putc(int chr, memory_file *x) {
    if(x->data >= x->datal) return(EOF);
    *x->data++ = chr;
    return(chr);
}
void fatal_error(char *msg, ...) {
    //printf("\nError: %s\n", msg);
    puts(msg);
    exit(1);
}
void do_nothing(void) {}
int return_eof(void) {
    return(EOF);
}



BIT_FILE *OpenOutputBitFile( unsigned char *out, unsigned char *outl ) {
    BIT_FILE *bit_file;

    bit_file = (BIT_FILE *) calloc( 1, sizeof( BIT_FILE ) );
    if ( bit_file == NULL )
        return( bit_file );
    //bit_file->file = fopen( name, "wb" );
    bit_file->file.datas = out;
    bit_file->file.data  = out;
    bit_file->file.datal = outl;
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    //bit_file->pacifier_counter = 0;
    return( bit_file );
}

BIT_FILE *OpenInputBitFile( unsigned char *in, unsigned char *inl ) {
    BIT_FILE *bit_file;

    bit_file = (BIT_FILE *) calloc( 1, sizeof( BIT_FILE ) );
    if ( bit_file == NULL )
	return( bit_file );
    //bit_file->file = fopen( name, "rb" );
    bit_file->file.datas = in;
    bit_file->file.data  = in;
    bit_file->file.datal = inl;
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    //bit_file->pacifier_counter = 0;
    return( bit_file );
}

void CloseOutputBitFile( bit_file )
BIT_FILE *bit_file;
{
    if ( bit_file->mask != 0x80 )
        if ( mn_putc( bit_file->rack, &bit_file->file ) != bit_file->rack )
            fatal_error( "Fatal error in CloseBitFile!\n" );
    //fclose( bit_file->file );
    free( (char *) bit_file );
}

void CloseInputBitFile( bit_file )
BIT_FILE *bit_file;
{
    //fclose( bit_file->file );
    free( (char *) bit_file );
}

void OutputBit( bit_file, bit )
BIT_FILE *bit_file;
int bit;
{
    if ( bit )
        bit_file->rack |= bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 ) {
	if ( mn_putc( bit_file->rack, &bit_file->file ) != bit_file->rack )
	    fatal_error( "Fatal error in OutputBit!\n" );
	//else
        //if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
		//putc( '.', stdout );
	bit_file->rack = 0;
	bit_file->mask = 0x80;
    }
}

void OutputBits( bit_file, code, count )
BIT_FILE *bit_file;
unsigned long code;
int count;
{
    unsigned long mask;

    mask = 1L << ( count - 1 );
    while ( mask != 0) {
        if ( mask & code )
            bit_file->rack |= bit_file->mask;
        bit_file->mask >>= 1;
        if ( bit_file->mask == 0 ) {
	    if ( mn_putc( bit_file->rack, &bit_file->file ) != bit_file->rack )
		fatal_error( "Fatal error in OutputBit!\n" );
        //else if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
		//putc( '.', stdout );
	    bit_file->rack = 0;
            bit_file->mask = 0x80;
        }
        mask >>= 1;
    }
}

int InputBit( bit_file )
BIT_FILE *bit_file;
{
    int value;

    if ( bit_file->mask == 0x80 ) {
        bit_file->rack = mn_getc( &bit_file->file );
        if ( bit_file->rack == EOF )
            fatal_error( "Fatal error in InputBit!\n" );
    //if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
	    //putc( '.', stdout );
    }
    value = bit_file->rack & bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 )
	bit_file->mask = 0x80;
    return( value ? 1 : 0 );
}

unsigned long InputBits( bit_file, bit_count )
BIT_FILE *bit_file;
int bit_count;
{
    unsigned long mask;
    unsigned long return_value;

    mask = 1L << ( bit_count - 1 );
    return_value = 0;
    while ( mask != 0) {
	if ( bit_file->mask == 0x80 ) {
	    bit_file->rack = mn_getc( &bit_file->file );
	    if ( bit_file->rack == EOF )
		fatal_error( "Fatal error in InputBit!\n" );
        //if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
		//putc( '.', stdout );
	}
	if ( bit_file->rack & bit_file->mask )
            return_value |= mask;
        mask >>= 1;
        bit_file->mask >>= 1;
        if ( bit_file->mask == 0 )
            bit_file->mask = 0x80;
    }
    return( return_value );
}



void FilePrintBinary( file, code, bits )
void *file;
unsigned int code;
int bits;
{
/*
    unsigned int mask;

    mask = 1 << ( bits - 1 );
    while ( mask != 0 ) {
        if ( code & mask )
            fputc( '1', file );
        else
            fputc( '0', file );
        mask >>= 1;
    }*/
}

