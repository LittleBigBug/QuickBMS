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
//     TurboRLE - "TurboRLE: Turbo Run Length Encoding" Java Critical Natives Interface

/* Usage: (actually no makefile available)
0 - install jdk. 
On ubuntu type: sudo apt-get install default-jdk
1 - generate header jtrle.h 
$ cd ~/TurboRLE/java
$ javah -jni jtrle
$ cp jtrle.h ..

2 - Compile jtrle and jtrlebench
$ javac jtrle.java
$ javac jtrlebench.java

3 - compile & link a shared library
$ cd ..
$ gcc -O3 -march=native -fstrict-aliasing -m64 -shared -fPIC -I/usr/lib/jvm/default-java/include -I/usr/lib/jvm/default-java/include/linux trlec.c trled.c jtrle.c -o libtrle.so

4 - copy "libtrle.so" to java library directory

5 - start trlebench
$ cd ~/TurboRLE/java
$ java trlebench

*/

import java.util.Random;

class jtrlebench {
  // Note: this is a simple interface test not a real benchmark 

  public static void main(String args[]) {
    jtrle trle = new jtrle();
    int n = 10000000; 
    final byte[]  in = new byte[n];
    final byte[] out = new byte[n];
    final byte[] cpy = new byte[n];

    Random random = new Random();
    for (int i = 0; i < n; i++) { 
      int r = random.nextInt(8);
      in[i] = (byte)r;
      cpy[i] = 0;
    }
    long t0 = System.currentTimeMillis();		 
    int tnum = 100; 
    int len = 0;
    for (int i = 0; i < tnum; ++i) { 
      len = trle.trlec( in, n, out);
    }

    long t = System.currentTimeMillis() - t0;    
    System.out.println("encode time'" + t + "'");
    t0 = System.currentTimeMillis();	
    for (int i = 0; i < tnum; ++i) { 
      len = trle.trled(out, len, cpy, n);
    }

    for (int i = 0; i < n; ++i) { 
      if(in[i] != cpy[i]) {
        System.err.println("Error at'" + i + "'");
        System.exit(1);
      }
    }
    t = System.currentTimeMillis() - t0;
    System.out.println("decode time'" + t + "'");
  }
}

