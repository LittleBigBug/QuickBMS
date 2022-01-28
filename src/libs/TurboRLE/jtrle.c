/**
    Copyright (C) powturbo 2013-2020
    GPL v2 License
  
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    - homepage : https://sites.google.com/site/powturbo/
    - github   : https://github.com/powturbo
    - twitter  : https://twitter.com/powturbo
    - email    : powturbo [_AT_] gmail [_DOT_] com
**/
// TurboRLE - Java Critical Natives

#include <stdio.h>
#include <stdlib.h>
#include "trle.h"

#include "jtrle.h"

#define JNIB jboolean _inlock, _outlock;\
  jint i  = (*env)->GetArrayLength(env, _in);  jbyte *in  = (jbyte *)(*env)->GetPrimitiveArrayCritical(env, _in, &_inlock);\
  jint o = (*env)->GetArrayLength(env, _out); jbyte *out = (jbyte *)(*env)->GetPrimitiveArrayCritical(env, _out,&_outlock)

#define JNIE\
 (*env)->ReleasePrimitiveArrayCritical(env, _in,   in, JNI_ABORT);\
 (*env)->ReleasePrimitiveArrayCritical(env, _out, out, JNI_ABORT)

#define JNICCI(  __func, _in,    _out, x   ) JNIB; jint l = __func ((unsigned char *)in,    (unsigned char *)out, x   ); JNIE; return l
#define JNICCII( __func, _in,    _out, x, b) JNIB; jint l = __func ((unsigned char *)in,    (unsigned char *)out, x, b); JNIE; return l
#define JNICIC(  __func, _in, n, _out      ) JNIB; jint l = __func ((unsigned char *)in, n, (unsigned char *)out      ); JNIE; return l
#define JNICICI( __func, _in, n, _out, x   ) JNIB; jint l = __func ((unsigned char *)in, n, (unsigned char *)out, x   ); JNIE; return l
#define JNICICII(__func, _in, n, _out, x, b) JNIB; jint l = __func ((unsigned char *)in, n, (unsigned char *)out, x, b); JNIE; return l

//---------------------------------- TurboRLE ------------------------------------------------------------------------------------------
JNIEXPORT jint JNICALL Java_jtrle__srlec8(  JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out,               jint e ) { JNICICI(_srlec8,  _in, n, _out,          e ); } JNIEXPORT jint JNICALL JavaCritical_jtrle__srlec8(  jint i, jbyte *in, jint n, jbyte *out,              jint e ) { return _srlec8(  (unsigned char *)in, n, (unsigned char *)out,         e ); }
JNIEXPORT jint JNICALL Java_jtrle__srled8(  JNIEnv *env, jclass cls, jbyteArray _in,         jbyteArray _out, jint _outlen, jint e ) { JNICCII(_srled8,  _in,    _out, _outlen, e ); } JNIEXPORT jint JNICALL JavaCritical_jtrle__srled8(  jint i, jbyte *in,         jbyte *out, jint outlen, jint e ) { return _srled8(  (unsigned char *)in,    (unsigned char *)out, outlen, e ); }

JNIEXPORT jint JNICALL Java_jtrle__srlec32( JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out,               jint e ) { JNICICI(_srlec32, _in, n, _out,          e ); } JNIEXPORT jint JNICALL JavaCritical_jtrle__srlec32( jint i, jbyte *in, jint n, jbyte *out,              jint e ) { return _srlec32( (unsigned char *)in, n, (unsigned char *)out,         e ); }
JNIEXPORT jint JNICALL Java_jtrle__srled32( JNIEnv *env, jclass cls, jbyteArray _in,         jbyteArray _out, jint _outlen, jint e ) { JNICCII(_srled32, _in,    _out, _outlen, e ); } JNIEXPORT jint JNICALL JavaCritical_jtrle__srled32( jint i, jbyte *in,         jbyte *out, jint outlen, jint e ) { return _srled32( (unsigned char *)in,    (unsigned char *)out, outlen, e ); }

// functions w/ overflow handling
JNIEXPORT jint JNICALL Java_jtrle_srlec8(   JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out,               jint e ) { JNICICI( srlec8,  _in, n, _out,          e ); } JNIEXPORT jint JNICALL JavaCritical_jtrle_srlec8(   jint i, jbyte *in, jint n, jbyte *out,              jint e ) { return srlec8(   (unsigned char *)in, n, (unsigned char *)out,         e ); }
JNIEXPORT jint JNICALL Java_jtrle_srled8(   JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out, jint _outlen, jint e ) { JNICICII(srled8,  _in, n, _out, _outlen, e ); } JNIEXPORT jint JNICALL JavaCritical_jtrle_srled8(   jint i, jbyte *in, jint n, jbyte *out, jint outlen, jint e ) { return srled8(   (unsigned char *)in, n, (unsigned char *)out, outlen, e ); }

JNIEXPORT jint JNICALL Java_jtrle_srlec32(  JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out,               jint e ) { JNICICI( srlec32, _in, n, _out,          e ); } JNIEXPORT jint JNICALL JavaCritical_jtrle_srlec32(  jint i, jbyte *in, jint n, jbyte *out,              jint e ) { return srlec32(  (unsigned char *)in, n, (unsigned char *)out,         e ); }
JNIEXPORT jint JNICALL Java_jtrle_srled32(  JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out, jint _outlen, jint e ) { JNICICII(srled32, _in, n, _out, _outlen, e ); } JNIEXPORT jint JNICALL JavaCritical_jtrle_srled32(  jint i, jbyte *in, jint n, jbyte *out, jint outlen, jint e ) { return srled32(  (unsigned char *)in, n, (unsigned char *)out, outlen, e ); }

// RLE w. automatic escape char determination
JNIEXPORT jint JNICALL Java_jtrle_srlec (   JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out                       ) { JNICIC(  srlec ,  _in, n, _out             ); } JNIEXPORT jint JNICALL JavaCritical_jtrle_srlec (   jint i, jbyte *in, jint n, jbyte *out                       ) { return srlec(    (unsigned char *)in, n, (unsigned char *)out            ); }
JNIEXPORT jint JNICALL Java_jtrle__srled(   JNIEnv *env, jclass cls, jbyteArray _in,         jbyteArray _out, jint _outlen         ) { JNICCI( _srled,   _in,    _out, _outlen    ); } JNIEXPORT jint JNICALL JavaCritical_jtrle__srled(   jint i, jbyte *in,         jbyte *out, jint outlen          ) { return _srled(   (unsigned char *)in,    (unsigned char *)out, outlen    ); }
JNIEXPORT jint JNICALL Java_jtrle_srled (   JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out, jint _outlen         ) { JNICICI( srled ,  _in, n, _out, _outlen    ); } JNIEXPORT jint JNICALL JavaCritical_jtrle_srled (   jint i, jbyte *in, jint n, jbyte *out, jint outlen          ) { return srled(    (unsigned char *)in, n, (unsigned char *)out, outlen    ); }

// Turbo RLE
JNIEXPORT jint JNICALL Java_jtrle_trlec (   JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out                       ) { JNICIC(  trlec ,  _in, n, _out             ); } JNIEXPORT jint JNICALL JavaCritical_jtrle_trlec (   jint i, jbyte *in, jint n, jbyte *out                       ) { return trlec(    (unsigned char *)in, n, (unsigned char *)out            ); }
JNIEXPORT jint JNICALL Java_jtrle__trled(   JNIEnv *env, jclass cls, jbyteArray _in,         jbyteArray _out, jint _outlen         ) { JNICCI( _trled,   _in,    _out, _outlen    ); } JNIEXPORT jint JNICALL JavaCritical_jtrle__trled(   jint i, jbyte *in,         jbyte *out, jint outlen          ) { return _trled(   (unsigned char *)in,    (unsigned char *)out, outlen    ); }
JNIEXPORT jint JNICALL Java_jtrle_trled (   JNIEnv *env, jclass cls, jbyteArray _in, jint n, jbyteArray _out, jint _outlen         ) { JNICICI( trled ,  _in, n, _out, _outlen    ); } JNIEXPORT jint JNICALL JavaCritical_jtrle_trled (   jint i, jbyte *in, jint n, jbyte *out, jint outlen          ) { return trled(    (unsigned char *)in, n, (unsigned char *)out, outlen    ); }



















