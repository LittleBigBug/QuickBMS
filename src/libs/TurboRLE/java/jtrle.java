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
//     TurboRLE - "TurboRLE: Turbo Run Length Encoding" Java Critical Natives Interface Class

public class jtrle {
  // RLE with specified escape char
  public native static int _srlec8(  byte[] in, int inlen, byte[] out,             int e);
  public native static int _srled8(  byte[] in,            byte[] out, int outlen, int e);

  public native static int _srlec32( byte[] in, int inlen, byte[] out,             int e);
  public native static int _srled32( byte[] in,            byte[] out, int outlen, int e);

  // functions w/ overflow handling
  public native static int  srlec8(  byte[] in, int inlen, byte[] out,             int e);
  public native static int  srled8(  byte[] in, int inlen, byte[] out, int outlen, int e);

  public native static int  srlec32( byte[] in, int inlen, byte[] out,             int e);
  public native static int  srled32( byte[] in, int inlen, byte[] out, int outlen, int e);

  // RLE w. automatic escape char determination
  public native static int  srlec(   byte[] in, int inlen, byte[] out);
  public native static int _srled(   byte[] in,            byte[] out, int outlen);
  public native static int  srled(   byte[] in, int inlen, byte[] out, int outlen);

  // Turbo RLE
  public native static int  trlec(   byte[] in, int inlen, byte[] out);
  public native static int _trled(   byte[] in,            byte[] out, int outlen);
  public native static int  trled(   byte[] in, int inlen, byte[] out, int outlen);

  static {
    try {
      System.loadLibrary("libtrle.so");
      //System.load("/home/x/TurboRLE/libtrle.so");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("can't load Native TurboRLE from '" + System.getProperty("java.library.path") + "'" + e);
      System.exit(1);
    }
  }
}

