
### liblzhl - LZH-Light compression/decompression library

This is an implementation of [An Algorithm for Online Data Compression](http://collaboration.cmc.ec.gc.ca/science/rpn/biblio/ddj/Website/articles/CUJ/1998/9810/ignatche/ignatche.htm).

## Origin

I could find two existing implementations of this code on the Internet:

* The [J2K Library](http://j2k.sourceforge.net/) project, which appears to be the original implementation
* [This implementation](https://github.com/daeken/PSReverse/tree/master/LHZL), which looks like part of a Windows tool

I forked the second one, and made the following changes:

* Got everything compiling with g++ and clang, on both Mac OSX and Linux
* Removed a bunch of non-portable Windows-isms
* Fixed a buffer overrun (read past end of array)
* Fixed some type mismatch warnings
* Fixed case-sensitivity trouble in the #includes
* Cleaned up the C API
* Added a rudimentary test program
* Re-indented a few messy spots

## Future

One day, I may get around to fixing up all the nasty int-long casts and re-doing this in more modern C++ style, but for now I'm just hoping that providing the existing code as a standalone project is useful to someone.

## Building

```sh
make
```

## Testing

Only a basic test program is supplied:

```sh
make test
./test
```

## Resources

A collection of helpful links

* [An Algorithm for Online Data Compression](http://collaboration.cmc.ec.gc.ca/science/rpn/biblio/ddj/Website/articles/CUJ/1998/9810/ignatche/ignatche.htm)
* [Dr.Dobb's link](http://www.drdobbs.com/an-algorithm-for-online-data-compression/184403560)
* [J2K Library (original code)](http://j2k.sourceforge.net/)
* [Another derived implementation](https://github.com/daeken/PSReverse/tree/master/LHZL)

## License

liblzhl is released under the GNU LGPL as required by http://j2k.sourceforge.net/faq.shtml
