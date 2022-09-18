###################################################################


# QuickBMS

by Luigi Auriemma

e-mail: me@aluigi.org

web:    aluigi.org

home:   http://quickbms.com

help:   http://zenhax.com


###################################################################


1) Introduction
2) Usage
3) Reimporting the extracted files
4) How to create scripts (for developers only!)
5) Experimental input, output and other features
6) Notes
7) Support
8) Additional credits


###################################################################


## 1) Introduction


QuickBMS is a multiplatform extractor engine programmed through
some simple instructions contained in textual scripts, it's
intended for extracting files and information from the archives and
files of any software and, moreover, games.

The script language used in QuickBMS is an improvement of MexScript
documented here: http://wiki.xentax.com/index.php/BMS
QuickBMS is FULLY compatible with that original syntax and all the
scripts that were created here:
http://forum.xentax.com/viewtopic.php?t=1086

QuickBMS also supports most of the WCX plugins of Total Commander:
  http://www.totalcmd.net/directory/packer.html
  http://www.ghisler.com/plugins.htm

The original BMS language has been improved for:
- removing implied fields, like the file number in some commands
- adding new commands, like Encryption
- adding new behaviors and features, like negative GoTo
These improvements allow QuickBMS to work with tons of simple and
complex formats and even doing tasks like modifying files, creating
new files with headers, converting files and reimporting the
extracted files back in their original archives.

The tool is open source under the GPL 2.0 license and works on
Windows, Linux and MacOSX, on both little and big endian platforms
like Intel (littlen endian) and PPC (big endian).
You can distribute the original quickbms.exe file as you desire
but reusing its source code and/or modifying it may require the
same or compatible open source license.

The official homepage of QuickBMS with all the scripts I have
written from 2009 till now is (they are just links to the same
website):

  http://quickbms.com
  \ http://quickbms.aluigi.org
   \ http://aluigi.altervista.org/quickbms.htm
    \ http://aluigi.zenhax.com/quickbms.htm (rarely updated)

There is also an official forum where it's provided support for
QuickBMS and help with file formats, it's also a very good and
friendly free community for reverse engineering game files:

  https://zenhax.com

QuickBMS is perfect for those tasks in which you need a quick way
to extract information from files and at the same time you would
like to reinject them back without writing a standalone tool to do
both the extraction and rebuilding jobs.
This is particularly useful if you have 100 different types of
archives to analyze (reverse engineering), parsing and then sharing
your tools with your community. It's more easy to do that with some
lines of text pasted on a forum or pastebin rather than writing 100
different standalone extraction tools plus other 100 standalone
rebuilders.


-------------------------------------------------------------------


For Linux and MacOSX users there is a Makefile in the src folder,
the only requirements are openssl, zlib and bzip2 while the
optional components are mcrypt and tomcrypt (uncomment the line
near the end of the Makefile to enable them).
If your distro supports apt-get and you have problems during the
usage of "make", try the following:

  apt-get install gcc g++ zlib1g-dev libssl-dev unicode

In case of problems on 64bit versions of Linux, try also to append
a ":i386" to the previous dependencies, like:

  apt-get install libssl-dev:i386

MacOSX users need to read the simple instructions written in the
Makefile, just few steps for being able to compile QuickBMS easily
without problems, anyway maybe try a "make" first because from
version 0.8.1 it was rewritten to work easily.
Updated static builds for Linux x86 and MacOSX are available on
http://aluigi.altervista.org/quickbms.htm#builds

Feel free to contact me in case of problems or just post on
https://zenhax.com


###################################################################

## 2) Usage


Simple and quick:

- double-click on quickbms.exe

- select the script for the type of archive you want to extract,
  for example zip.bms if it's a zip file.

- select the input archive or multiple files.
  you can also select a whole folder by entering in it and then
  typing * (or "" on systems before Windows 7) in the "File name:"
  field, and then select Open.
  You can even use * to set wildcards, for example *.txt or
  *required_name* or prefix*suffix

- select the output folder where extracting the files.
  you can specify any filename, it will be ignored because only the
  current selected directory is taken

- watch the progress status of the extraction and the final message

That's the simple "GUI" usage but QuickBMS can do various other
things when launched from the console, in fact it supports many
command-line options for advanced users and for who writes the
scripts.
You can view all the available options simply launching QuickBMS
from command-line ("cmd.exe" on Windows) without arguments.
The following is the current list of options:


    Usage: quickbms.exe
             [options]
               <script.BMS>
                 <input_archive/folder>
                   [output_folder]

    Options:
    -l     list the files without extracting them
    -f W   filter the files to extract using the W wildcards separated by comma or
           semicolon, example -f "{}.mp3,{}.txt;{}myname{}"
           if the filter starts with ! it's considered an ignore/exclusion filter,
           if .txt it's read as text file with multiple filters, * and {} are same
           example: quickbms -f "{}.mp3;!{}.ogg" script.bms archive.dat output
           example: quickbms -f myfilters_list.txt script.bms archive.dat
           use {} instead of * to avoid issues on Windows, multiple -f are ok too
    -F W   as above but works only with the files in the input folder (if used)
           example: quickbms -F "{}.dat" script.bms input_folder output_folder
    -o     overwrite the output files without confirmation if they already exist
    -k     keep the current files if already exist without asking (skip all)
    -K     automatically rename the output files if duplicates already exist
    -r     experimental reimport option that should work with many archives:
             quickbms script.bms archive.pak output_folder
             modify the needed files in output_folder and maybe remove the others
             quickbms -w -r script.bms archive.pak output_folder
           you MUST read section 3 of quickbms.txt before using this feature,
           use -r -r for the alternative and better REIMPORT2 mode
           use -r -r -r for REIMPORT3 that shrinks/enlarges archive if no offset
    -u     check if there is a new version of QuickBMS available
    -i     generate an ISO9660 file instead of extracting every file, the name of
           the ISO image will be the name of the input file or folder
    -z     exactly as above but it creates a ZIP file instead of an ISO image

    Advanced options:
    -d     automatically create an additional output folder with the name of the
           input folder and file processed, eg. models/mychar/mychar.arc/*,
           -d works also if input and output folders are the same (rename folder)
    -D     like -d but without the folder with the filename, eg. models/mychar/*
    -E     automatically reverse the endianess of any input file by simply reading
           each field and writing the reversed value, each Get produces a Put
    -c     quick list of basic BMS commands and some notes about this tool
    -S CMD execute the command CMD on each file extracted, you must specify the
           #INPUT# placeholder which will be replaced by the name of the file
           example: -S "lame.exe -b 192 -t --quiet #INPUT#"
    -Y     automatically answer yes to any question
    -O F   redirect the concatenated extracted files to output file F, data is
           appended if file F exists, optional F extensions supported: TAR
    -s SF  add a script file or command before the execution of the input script,
           useful if an archive uses a different endianess or encryption and so on
           SF can be a script or directly the bms instruction you want to execute
    -.     don't terminate QuickBMS if there is an error while parsing multiple
           files (like wrong compression or small file), just continue with the
           other files in the folder; useful also in rare cases in reimport mode

    Debug and experimental options:
    -v     verbose debug script information, useful for verifying possible errors
    -V     alternative verbose info, useful for programmers and formats debugging
    -q     quiet, no *log information
    -Q     very quiet, no information displayed except the Print command
    -L F   dump the offset, size and name of the extracted files into the file F
    -x     use the hexadecimal notation in myitoa (debug)
    -0     no extraction of files, useful for testing a script without using space
    -R     needed for programs that act as interface for QuickBMS and in batch
    -a S   pass arguments to the input script that will take the names
           quickbms_arg1, quickbms_arg2, quickbms_arg3 and so on, note they are
           handled as arguments so pay attention to spaces and commas, eg:
             -a "arg1 \"arg 2\", arg3"
             -a arg1 -a "\"arg 2\"" -a arg3
           a full backup of the whole -a options is on the var quickbms_arg
    -H     experimental HTML hex viewer output, use it only with very small files!
    -X     experimental hex viewer output on the console (support Less-like keys)
    -9     toggle XDBG_ALLOC_ACTIVE  (enabled)
    -8     toggle XDBG_ALLOC_INDEX   (enabled)
    -7     toggle XDBG_ALLOC_VERBOSE (disabled)
    -6     toggle XDBG_HEAPVALIDATE  (disabled)
    -3     execute an INT3 before each CallDll, compression and encryption
    -I     toggle variable names case sensitivity (default insensitive)
    -M F   experimental compare and merge feature that allows to compare the
           extracted files with those located in the folder F, currently this
           experimental option will create files of 0 bytes if they are not
           different, so it's not simple to identify what files were written
    -Z     input file cleaner, in reimport mode replaces all archived files with
           zeroes, no matter if they exist or not in the folder, will be all zeroed
    -P CP  set the codepage to use (default utf8), it can be a number or string
    -T     do not delete the TEMPORARY_FILE at the end of the process
    -N     decimal names for files without a name: 0.dat instead of 00000000.dat
    -e     ignore the compression errors and dump the (wrong) output data anyway,
           in reimport2 it disables the compression of the files (experimental)
    -J     all the constant strings are considered Java/C escaped strings (cstring)
    -B     debug option dumping all the non-parsed content of the open files, the
           data will be saved in the output folder as QUICKBMS_DEBUG_FILE*
    -W P   experimental web API (P is the port) and pipe/mailslot IPC interface
    -t N   experimental tree-view of the extracted/listed files where N is:
           0:text1, 1:text2, 2:text3, 3:json1, 4:json2, 5:web, 6:dos, 7:ls
    -U [S] list of available compression algorithms, use S for searching names
    -#     in reimport mode checks if the archived files and those to reimport are
           the same (hash), it's useful if you didn't remove the unmodified files
    -j     force UTF16 output in some functions, for example with SLog
    -b C   use C (char or hex) as filler in reimporting if the new file is smaller,
           by default it's used space in SLog and 0 for Log and CLog
    -y F   experimental debug output to file F, supported formats on file extension
           json, csv, yaml, c/java and so on

    Features and security activation options:
    -w     enable the write mode required to write physical input files with Put*
    -C     enable the usage of CallDll without asking permission
    -n     enable the usage of network sockets
    -p     enable the usage of processes
    -A     enable the usage of audio device
    -g     enable the usage of video graphic device
    -m     enable the usage of Windows messages
    -G     force the GUI mode on Windows, it's automatically enabled if you
           double-click on the QuickBMS executable


Remember that the script and the input archive/folder are ever
REQUIRED and they must be specified at the end of the command-line.

The following is an example for listing all the mp3 files from the
input archive:

  quickbms -l -f "{}.mp3" zip.bms myfile.zip
  quickbms -l -f "{}.mp3;{}.ogg" zip.bms myfile.zip
  quickbms -l -f "{}.mp3;{}.ogg,{}filename{}" zip.bms myfile.zip
  quickbms -l -f file_containing_the_filters.txt zip.bms myfile.zip
  (file_containing_the_filters.txt has one filter per each line)

So -l for listing the files without extracting them, and -f for
filtering the archived files. Regarding the -f and -F options it's
worth to note that both * and {} are accepted as wildcards because
the first pattern may be interpreted by the Windows console (my
suggestion is to use ever {} to avoid problems).

QuickBMS supports also a folder as input which means that with a
single command it's possible to unpack all the archives of a whole
game directly using QuickBMS.

Imagine to use the zip.bms script with all the zip files located in
the Program Files folder:

  quickbms -F "{}.zip" zip.bms "c:\Program Files (x86)" c:\outfolder

Note: as said before, sometimes Windows doesn't like the * char
      even if used between quotes, so in case of problems with
      "*.zip" you can use {} instead of *, for example "{}.zip"

Except for -l, -f, -F and maybe -o and -s options, the others are
intended for debugging, or they are special features or switches to
enable/disabe some internals, so they should be ignored by the
common users.

If output_folder is omitted, the current directory is used.
From version 0.9.1, if output_folder is "", the same direcotyr of
input file (or each file in case of input folder) is used.

If the extraction with a particular script is too slow or scanning
a folder takes too much memory and time try using the -9 option
that disables the memory protection.

You can apply these options directly in a shortcut to quickbms.exe
in the Target field of its properties, so you can use the
double-click "GUI" method and all the command-line options you
desire without using the command-line.

In the quickbms.zip package you can also see quickbms_4gb_files.exe
(previously known as quickms64_test.exe) which is an "experimental"
version that uses 64bit numbers instead of the original 32 bits:
- it supports archives and files bigger than 4 gigabytes
- it may have problems to work with "some" scripts
- it's a native 32bit software so it works on both Windows 32 & 64
- it's experimental and partially supported, problems like crashes
  and incorrect math operations may happen often in some scripts


-------------------------------------------------------------------


Advanced users could find useful also these specific options:

-d Automatically creates a folder with the name of the input file
   where placing all the files, it's useful if you have many small
   archives containing the same filenames and need to separate the
   extracted files without overwriting or renaming them.

-E If you have a bms script that simply reads a file format, you
   can change the endianess of all its numeric fields on the fly by
   simply using this option.
   For example if you have a "get SIZE long" a 32bit number will be
   read as usual and additionally it will be reversed (0x11223344
   to 0x44332211 or viceversa) and placed at the same location.
   Remember that you need to specify also the -w option with
   physical files, alternatively you can save the whole file in a
   memory file and then dumping it so that -w is not necessary.
   With this option is really trivial to convert the endianess of
   files between different platforms, like Xbox 360 and PC.



###################################################################

## 3) Reimporting the extracted files


QuickBMS is mainly an extraction tool, but it supports also the -r
option that converts the tool in a simple reimporter/reinjector and
so it may be useful for modding or translating a game.

The idea consists of being able to reimport ("injecting back") the
modified files in the original archives without editing the script,
just reusing the same bms scripts that already exist!


-------------------------------------------------------------------


Using this feature is really trivial and the following is a
step-by-step example:

- Make a backup copy of the original archive!

- Extract the files or only those you want to modify (-f option) as
  you do normally via the GUI (double-click on quickbms.exe) OR via
  command-line like the following example:

    quickbms script.bms archive.pak output_folder

- Modify the extracted files leaving their size unchanged or
  smaller than before.
  I suggest to delete the files that have not been modified so that
  the reimporting process will be faster and safer. In the folder
  leave only the files you modified.
  Remember that their size must be smaller/equal than the original!

- Reimport the files in the archive via the GUI by clicking on the
  file called "reimport.bat" OR via command-line:

    quickbms -w -r script.bms archive.pak output_folder

- Test the game with the modified archive

Remember that you can use the GUI for the reimporting procedure,
just click on "reimport.bat" found in the quickbms package, it
contains the command: quickbms.exe -G -w -r.



IMPORTANT NOTE ABOUT "REIMPORT2" MODE
From version 0.8.2 QuickBMS started to implement an additional
alternative reimport mode enabled by using -r twice like:

    quickbms -w -r -r script.bms archive.pak output_folder
or
    reimport2.bat

This mode can be used with many formats and offers the following
advantages:
- no size limits with the imported files, the bigger files will be
  inserted (appended) at the end of the archive
- the fields "offset", "size" and "compressed size" are rewritten
  by matching the new imported file, that's useful with various
  size-dependent compression algorithms like lz4
The reimport2 method doesn't work if:
- the TOC is compressed or located on a MEMORY_FILE
- the TOC/magic is (relatively) located at the end of the archive
- the content is sequential, so there is no offset
- the 3 fields mentioned above are very different than those
  originally read from the TOC, in this mode only one maximum
  "math" operation is allowed on the variable which means that the
  following example works:
    get OFFSET long ; math OFFSET * 0x800 ; log NAME OFFSET SIZE
  while this example produces an incorrect OFFSET field:
    get OFFSET long ; math OFFSET * 0x800 ; math OFFSET + BASE_OFF ; log NAME OFFSET SIZE
  the same is valid for the size fields too, anyway note that
  "offset" is rewritten only if the new file is bigger than before
- the game strictly trusts the original size of the archive and
  ignores data appended to it, for example some archives may have a
  field in the TOC that specifies the size of the archive
- SLog is implemented but may not work with some archives
- the archive is subject to other limits described below, excluded
  the advantages listed before

From version 0.10.0 QuickBMS has an additional mode called
REIMPORT3, it's identical to REIMPORT2 with the only difference
that the archive is shrinked or enlarged if there is no offset
field used in the archive and the size of the input file differs
than the original.
This method "may" be useful with some language files and some
archives with sequential data.


-------------------------------------------------------------------


Another example:
- First step, use QuickBMS as usual:

  archive.pak -> file1.txt
              -> file2.dat
              -> file3.jpg

- Second step:
  - delete file1.txt and file2.dat
  - modify file3.jpg, for example adding a "smile" in it
  - save file3.jpg and be sure that it's size is SMALLER or EQUAL
    than the original

- Third step, clink on the reimport.bat file provided in quickbms
  and select the SAME file and output folder you selected in the
  first step:

  archive.pak <- file1.txt  (doesn't exist so it's not reimported)
              <- file2.dat  (doesn't exist so it's not reimported)
              <- file3.jpg  (successfully reimported)


-------------------------------------------------------------------


Some important notes about this particular reimporting process:

- you CANNOT increase the size of the files you want to reimport,
  so the new files must be smaller or equal than the original ones.

- the reimport process of compressed files may be very slow in some
  cases, for example with zlib, deflate, lzma and few others that
  are optimized to use less space as possible at cost of time.
  zlib/deflate is particular slow because QuickBMS uses different
  solutions to reduce the size as much as possible.

- for the maximum compatibility within the thousands of available
  file formats I decided to not use tricks for modifying the
  original size and compressed_size values.
  for example imagine those formats that use encrypted information
  tables or MEMORY_FILEs for such tables or that use things like
  "math SIZE *= 0x800".
  the reimport process must be generic, universal and without
  work-arounds.

- the script is just the same for both extraction and reimporting,
  it means that many of the scripts written by me and the other
  users already work, cool!

- the reimporting of compressed files is perfectly possible because
  the tool automatically switches to the relative compression
  algorithm if available (for example deflate -> deflate_compress),
  if an algorithm is not available in recompress mode then the
  reimporting will fail

- SLog is a new command that has been recently added to QuickBMS
  for dumping strings and texts, it works also in reimport mode but
  it's very limited and prone to errors. I suggest to check the
  manual for the SLog command (search slog in this text), but a
  generic universal rule is:
  - keep the length of the edited line of text as the original

? if the original archive uses complex encryptions that require
  the usage of MEMORY_FILEs to perform temporary decryption, then
  it's NOT supported and the same is valid for chunked content
  (like those scripts that use the command Append)
  From version 0.6.6, QuickBMS has an experimental mode for
  reimporting chunked files, it works very well with files saved
  directly to disk and less well with those that use MEMORY_FILEs
  (most of my scripts).
  In my opinion this feature is great but don't expect much, with
  some scripts you can have success but many others may not work.

- FileXor, FileRot, Encryption and Filecrypt should work correctly

- things like CRCs and hashes can't be supported

- it's also possible to reimport the nameless files dumped with
  'log "" OFFSET SIZE', the tool will automatically check for files
  in the folder with the same number so if the file was saved as
  00000014.xml it will be reimported perfectly.

- the reimport mode doesn't work if you renamed the files with the
  same name during the extraction (for example using the 'r'
  choice), in this case there is no way for the tool to know the
  correct file to reimport and will reimport only the one with the
  same original name.

- the -Z option is a simple way to zero ALL the spaces of the
  archive occupied by the original files, the result will be a sort
  of "empty" archive. It "may" be useful for releasing the empty
  archive and the files separately and then reinjecting them in
  reimport mode with the option leaving out some unused files.
  Example:
    - quickbms script.bms archive.ar output_folder
    - quickbms -r -w -Z script.bms archive.ar output_folder
      (the content of output_folder is completely ignored in -Z)
    - remove videos from output_folders
    - compress archive.ar and output_folder, give them to a friend
    - quickbms -r -w    script.bms archive.ar output_folder
    - now archive.ar all the files but the videos
  The behaviour of this feature may change in future depending by
  the feedback of the users, currently there is no real usage.


Please note that often the games are able to load the extracted
files directly from their installation folder, sometimes directly
maybe by just removing the original archive and other times by
launching the game with specific command-line arguments.
The reimport feature of QuickBMS has already allowed to slightly
mod and translate various games, but it's meant as a quick or
temporary solution till a proper stand-alone rebuilder tool is
written by the community of the target game, due to the better
benefits coming from a complete and specific solution.
But if nobody is going to write a stand-alone rebuilder for a
specific game, then the reimport feature of QuickBMS is a great and
immediately available solution.


###################################################################

===============================================
4) How to create scripts (for developers only!)
===============================================


Originally the tool was created just for myself to be able to write
quick extractors for simple archives immediately without writing a
new tool, but QuickBMS revealed to be a powerful tool that I use
for many tasks, including the parsing of some protocols and much
more.

So, how to write these scripts?
Giving a look at http://wiki.xentax.com/index.php/BMS is a good
first step to understand at least the basis of this language
originally written by Mike Zuurman (alias Mr.Mouse of XeNTaX) in
the far 1997.
Then it's good to take a look at the various examples provided on
http://quickbms.com and http://zenhax.com

A programming knowledge and background is not required but it's
very useful for understanding the "logic" of the scripts and some
terms.
What is really necessary is the full knowledge of the format to
implement: reverse engineering is ever useful for figuring the
needed fields.

Luckily in the extraction process it's not needed to know all the
fields of an archive, so a field like a CRC doesn't matter while
the important fields to extract a file are ever the following:

- filename
- offset
- size
- optional compressed size if the file is compressed

If you don't have filename and size, it's not a problem. What's
really necessary is knowing at least of the offsets of the files.
If you check my scripts you can notice the name DUMMY assigned to
the fields that are not useful for the extraction.

Note that I will try to keep the following documentation updated as
much as I can, and also in sync with what happens inside QuickBMS
for each command.
The source code of the tool is not easy to understand so I hope
that this documentation may be useful and complete.

The fields between [] are optional fields.

---

A quick and limited list of available commands is available when
QuickBMS is launched with the -c option.
Some important notes about the QuickBMS environment:

- Everything is handled as a variable except if it starts with a
  number in which case it's considered a numeric constant, so when
  in this document I talk about VAR, STRING and other types of data
  I refer EVER to both variables and constants because they are
  EXACTLY the SAME thing inside the tool.

- All the commands and the names of the variables are case
  INsensitive, "get OFFSET long" is the same as "GeT oFfSeT lOnG".

- Everything works with signed 32 bit numbers (-2147483648 to
  2147483647) so QuickBMS may not work well with files over 2 Gb
  but it can seek on files of 4 Gb without problems.
  Consider the following limits:
  - max 4gb size for archives
  - max 2gb size for the archived files
  Try quickbms_4gb_files.exe when working with bigger archives.

- The constant strings depends by the context of the command, in
  fact in some commands they are handled as strings in C notation
  like "\x12\x34\\hello\"bye\0", in this case you must know how
  this representation works.
  This is a solution for using binary data in the textual script.
  The keyword is "C language escape characters" or escape
  sequences (or cstring), they are very simple, take a look here:
  https://docs.microsoft.com/en-us/cpp/c-language/escape-sequences
  From http://www.acm.uiuc.edu/webmonkeys/book/c_guide/1.1.html

    Escape Name / Meaning
    \a 	   Alert
    \b     Backspace
    \f     Form Feed
    \n     New Line
    \r     Carriage Return
    \t     Horizontal Tab
    \v     Vertical Tab
    \'     Produces a single quote
    \"     Produces a double quote
    \?     Produces a question mark
    \\     Produces a single backslash
    \0     Produces a null character
    \ddd   Defines one character by the octal digits (base-8)
    \xdd   Defines one character by the hexadecimal digit (base-16)

  ONLY some commands support this C string notation for the escape
  characters, a quick way to find them is searching the keyword
  "(cstring)" without quotes in this document.
  From version 0.8.2 exists the -J option that considers all the
  constant strings as escaped Java and C-like strings, so every
  string is a cstring when you use such option

- Both decimal and hexadecimal numbers are supported, the former is
  used if the number starts with 0x so 1234 and 0x4d2 are the same.

- Any operation made on fields bigger than 8 bits is controlled by
  the global endianess, it means that any number and unicode field
  is read in little endian by default otherwise it's valid the
  endianess specified with the Endian command.

- Comments can be used in C (// and /* */) and BMS syntax (#), for
  example:
    get DUMMY long  # this is a comment
    /*
    this is a comment
    */

- The FILENUM (file number) field in the commands is set as a
  constant, it means that it cannot be modified at runtime using a
  variable, examples:
    get TMP string 0    # ok
    get TMP string VAR  # wrong

- All the commands use variables for their arguments except those
  in which it's specified that a constant number or a string
  (STRING) is needed.
  For example the commands that use a C string (cstring) use
  constant strings and not variables, except some cases like the
  dictionary of ComType.
  Note that this behaviour may change in future or may have been
  already changed in some commands.


File numbers:
  Every file opened in QuickBMS has a number assigned to it, if
  this number is not specified it will be considered 0, the main
  input file.
  The first opened file is the input archive to which is assigned
  the number 0 (zero), the others must use the Open command.
  Negative numbers are considered MEMORY_FILEs, so -1 is
  MEMORY_FILE, -2 MEMORY_FILE2 and so on.

MEMORY_FILEs:
  This is a particular type of temporary file which resides in
  memory and works exactly like a normal temporary file.
  It's extremely useful for doing many operations and you can use
  multiple memory files: MEMORY_FILE, MEMORY_FILE2, MEMORY_FILE3
  and so on.
  MEMORY_FILE and MEMORY_FILE1 are the same file.
  .
  If you need to work with chunked parts of a file to concatenate
  to the memory file, you need to use the following trick:
  .
    putvarchr MEMORY_FILE FINAL_SIZE 0  # allocate memory
    log MEMORY_FILE 0 0                 # create the file
  .
  The first instruction allocates the memory for containing the
  final size of your chunks, and the second one is necessary for
  resetting the memory file (current offset and size, not the
  allocated size).
  If you need to create a MEMORY_FILE of 0x100 bytes set to zero to
  use in CallDLL use the following
  .
    log MEMORY_FILE 0 0                 # create the file
    putvarchr MEMORY_FILE 0x100 0       # write 0x100+1 zeroes

TEMPORARY_FILE:
  This additional file called TEMPORARY_FILE resides physically on
  the target folder and has that exact name.
  Despite its "temporary" name, it's not deleted by the output
  folder and QuickBMS will ask to remove it at end of extraction.
  The file is created in any condition, even when it's used the -l
  (list) option for listing the files, so it's perfect in certain
  situations like when it's used a chunks based file system.
  The difference with the MEMORY_FILE is only related to the amount
  of memory available on the system because the previous file types
  uses the RAM while this one uses the disk, so use it if you need
  to create a temporary file bigger than 2 gigabytes.
  .
  For using the temporary file check this example:
  .
    log TEMPORARY_FILE 0 0     # reset it if it already exists
    append                     # enables the append mode
    ...
        log TEMPORARY_FILE OFFSET SIZE
    ...
    append                     # disable the append mode
    open "." TEMPORARY_FILE 1  # open temporary file as file 1
  .
  Note that from version 0.6.8, QuickBMS automatically overwrites
  this file if it already exists.


The following is the list of types of variables supported, also
know as datatypes or types.
The list is ordered just like in defs.h:

    BYTE            8 bit, 0 to 0xff

    SIGNED_BYTE     0x99 is read as 0xffffff99

    SHORT           16 bit (aka INT), 0 to 0xffff

    SIGNED_SHORT    0x9999 is read as 0xffff9999

    THREEBYTE       24 bit, 0 to 0xffffff

    SIGNED_THREEBYTE

    LONG            32 bit, 0 to 0xffffffff

    SIGNED_LONG     mainly useful in quickbms_4gb_files:
                    0x99999999 is read as 0xffffffff0x99999999

    LONGLONG        fake 64 bit, so only 0 to 0xffffffff but Get takes 8 bytes

    FLOAT           32 bit, 123.345 is read as 123
                    From QuickBMS 0.10.1 floats (and doubles) are partially
                    handled in Get, Put, Math and Print commands.

    DOUBLE          64 bit, 123.345 is read as 123

    LONGDOUBLE      96 bit, 123.345 is read as 123
                    Note that size of long double is compiler dependent

    STRING          NUL delimited string (one byte for each char)

    UNICODE         special type used for unicode utf16 strings, the
                    endianess of the utf16 is the same used globally in the
                    script (watch the Endian command), it's used also for
                    converting an unicode string to an ascii one:
                      Set ASCII_STRING UNICODE UNICODE_STRING

                    unicode conversion is performed via Win32 API (CP_UTF8
                    and CP_ACP in case of 0xfffd chars) while on Linux it
                    uses iconv, fallback on mbtowc and byte=short

    UTF32           experimental support for 32bit unicode (unicode32)

    BINARY          special type used for binary strings in C notation like
                    "\xff\x00\x12\x34", used mainly as a constant (cstring)

    LINE            special type used for carriage return/line feed delimited
                    string (so any string ending with a 0x00, 0x0a or 0x0d),
                    from version 0.6 the tool supports also strings that
                    have no delimiter at the end of file

    ASIZE           special type used to return the size of the opened file,
                    used only with the GET command 

    FILENAME        special type used to return the name of the opened file
                    like "myfile.zip", used only with the GET command

    BASENAME        special type used to return the base name of the opened
                    file like "myfile", used only with the GET command

    FILEPATH        the folder of the file, like "c:\path\folder" for
                    "c:\path\folder\file.txt"

    FULLBASENAME    just like FULLNAME without extension

    EXTENSION       special type used to return the extension of the opened
                    file like "zip", used only with the GET command

    FULLNAME        full path of the file, in reality at the moment it returns
                    the same path used in the input filename

    CURRENT_FOLDER  the path from which has been launched QuickBMS

    FILE_FOLDER     the path of the loaded input file

    OUTPUT_FOLDER   the extraction folder (the last argument of QuickBMS)

    INPUT_FOLDER    same as above

    BMS_FOLDER      the folder where the bms script is located

    EXE_FOLDER      the folder where quickbms.exe is located

    ALLOC           a type used only in the Set command for creating a variable
                    with a specific allocated size

    COMPRESSED      a special type used for setting big strings and memory
                    files using a small amount of text, for using this type
                    you must take the original text/file, compress it with
                    zlib (you can use my packzip tool) and then encoding the
                    output file with base64 (you can use my bde64 tool) and
                    placing the result like the following:
                      set MEMORY_FILE compressed eNrtwbEJACAMBMBecIfvnMUxPuEJAe0UHN81LLzrbYKwDOjI96IN1cLveRfAGqYu
                    this type is very useful if you want to embed a dll inside
                    a script without wasting much space

                    You can create this variable using the following script:
                      http://aluigi.org/bms/file_compressed_var.bms

    VARIABLE        read byte per byte till the byte is negative

    VARIABLE2       Unreal engine index numbers

    VARIABLE3       used in various software

    VARIABLE4       used in Battlefield 3 (Frostbite engine) and Rar

    VARIABLE5       used in 7z archives

    VARIABLE6       requires a ValueMax variable

    VARIABLE7       similar to VARIABLE2

    UNKNOWN         use it to ask the user to insert the content of the variable

    VARIANT         VB/C++ variant type (http://en.wikipedia.org/wiki/Variant_type)

    BITS            read a specific amount of bits, QuickBMS and the
                    language are byte based but the "bits" method works
                    very well

    TIME            time_t Unix 32bit time

    TIME64          64bit time used as FILETIME on Windows

    CLSID           ClassID like 00000000-0000-0001-0000-000000000000

    IPV4            7f 00 00 01 = "127.0.0.1"

    IPV6            like 2001:0db8:85a3:0000:0000:8a2e:0370:7334

    ASM             x86 assembly
    ASM64           x86_x64 assembly
    ASM16           x86 16bit assembly
    ASM_???         arm, arm_thumb, arm64, mips, mips64, ppc,
                    ppc64, sparc, sysz, xcore

    TCC             a special type that compiles C text

    ???             the user will be asked to input a string that
                    will be the new value of the variable, prompt
                    from user / pause

Just for the record, the original MexScript probably contained some
types of variables that have never been used and for which it's
unknown what they should represent: PURETEXT, PURENUMBER,
TEXTORNUMBER and FILENUMBER.

QuickBMS supports also the "experimental" multidimensional arrays
inside the variables, for example:

    for i = 0 < 10
        get VAR[i] long
        for j = 0 < 5
            get VAR2[i][j] long
        next j
    next i

But it's possible to access that variable ONLY by specifying the
original name and index, so:

    print "%VAR[0]%"    # fail!
    print "%VAR[j]%"    # fail!

    math i = 0
    print "%VAR[i]%"    # ok

QuickBMS supports also embedded text like the following:

    Set VAR string "
        this is
        a text with \"blah\" and 'blah'
        and so on.
    "


The following is the list of bms commands:

    QuickBMSver VERSION
    FindLoc VAR TYPE STRING [FILENUM] [ERR_VALUE] [END_OFF]
    For [VAR] [OP] [VALUE] [COND] [VAR]
    Next [VAR] [OP] [VALUE]
    Get VAR TYPE [FILENUM] [OFFSET]
    GetDString VAR LENGTH [FILENUM]
    GoTo OFFSET [FILENUM] [TYPE]
    IDString [FILENUM] STRING
    Log NAME OFFSET SIZE [FILENUM] [XSIZE]
    Clog NAME OFFSET ZSIZE SIZE [FILENUM] [XSIZE]
    Math VAR OP VAR
    XMath VAR INSTR
    Open FOLDER NAME [FILENUM] [EXISTS]
    SavePos VAR [FILENUM]
    Set VAR [TYPE] VAR
    Do
    While VAR COND VAR
    String VAR OP VAR
    CleanExit
    If VAR COND VAR [...]
    [Elif VAR COND VAR]
    [Else]
    EndIf
    GetCT VAR TYPE CHAR [FILENUM]
    ComType ALGO [DICT] [DICT_SIZE]
    ReverseShort VAR [ENDIAN]
    ReverseLong VAR [ENDIAN]
    ReverseLongLong VAR [ENDIAN]
    Endian TYPE [VAR]
    FileXOR SEQ [OFFSET] [FILENUM]
    FileRot SEQ [OFFSET] [FILENUM]
    FileCrypt SEQ [OFFSET] [FILENUM]
    Strlen VAR VAR [SIZE]
    GetVarChr VAR VAR OFFSET [TYPE]
    PutVarChr VAR OFFSET VAR [TYPE]
    Debug [MODE]
    Padding VAR [FILENUM] [BASE_OFF]
    Append [DIRECTION]
    Encryption ALGO KEY [IVEC] [MODE] [KEYLEN]
    Print MESSAGE
    GetArray VAR ARRAY VAR_IDX
    PutArray ARRAY VAR_IDX VAR
    SortArray ARRAY [ALL]
    SearchArray VAR ARRAY VAR
    CallFunction NAME [KEEP_VAR] [ARG1] [ARG2] ... [ARGn]
    StartFunction NAME
    EndFunction
    ScanDir PATH NAME SIZE [FILTER]
    CallDLL DLLNAME FUNC/OFF CONV RET [ARG1] [ARG2] ... [ARGn]
    Put VAR TYPE [FILENUM]
    PutDString VAR LENGTH [FILENUM]
    PutCT VAR TYPE CHAR [FILENUM]
    GetBits VAR BITS [FILENUM]
    PutBits VAR BITS [FILENUM]
    Include FILENAME
    NameCRC VAR CRC [LISTFILE] [TYPE] [POLYNOMIAL] [PARAMETERS]
    Codepage VAR
    SLog NAME OFFSET SIZE [TYPE] [FILENUM] [TAG]
    Reimport [MODE]
    ImpType MODE VAR [...]
    CRCHash ALGO ARG1 ARG2
    Label NAME
    Break [NAME]
    Continue [NAME]


The following is the description of bms commands:


...................................................................

```
QuickBMSver VERSION

    Checks if the current version of QuickBMS is recent enough to
    support the script. Mainly for scripts created after the
    introduction of a new feature or an important fix.
    The instruction also enables some command-line options.

    Arguments:
      VERSION   Oldest version of QuickBMS for which the script was
                created the script, it's just the version displayed
                at runtime by the tool.
                It's possible to add some command-line options too:
                  -64   force quickbms_4gb_files.exe
                  -9    disable the safe memory allocator
                  -I    makes the variables case sensitive
                  -.    useful in reimport mode with data builders
                  -N    decimal names: 00000000.dat -> 0.dat
                  -q    quiet
                  -T    keep the temporary file if generated
                  -d    useful with some formats and scripts
                  -D    useful with some formats and scripts
                  -e    doesn't quit if compression fails
                  -J    all the strings are considered cstring
                  -32   checks if the user is using quickbms.exe
                  -F    filter the input files
                  -x    hexadecimal notation in myitoa (debug)
                  -j    force UTF16 output in some functions
                  -b C  use C (char or hex) as filler in reimport
                        if the new file is smaller
                  -c    this is NOT related to the -c option at
                        command-line, it's a way to avoid being
                        prompted when using C structures in the bms

    Examples:
      QuickBMSver 0.2.4
      QuickBMSver "0.5.14 -9"
      QuickBMSver "-I -9"


...................................................................

FindLoc VAR TYPE STRING [FILENUM] [ERR_VALUE] [END_OFF]

    It searches the first occurrence of a given string or number
    from the current offset of the file, just by scanning it byte
    per byte.
    It's used in those cases when the format of the archive is not
    known or it's a particular text file.

    Arguments:
      VAR       The variable receiving the offset of the occurrence
      TYPE      Type of the data we want to search, supported:
                - string
                - binary, can include any bytes (NUL too), since
                  version 0.11 it can also contain wildcards like
                  "\x??" or "\x**" for a wildcard byte (they covers
                  only the first 32 bytes of the string)
                - unicode, the search will be performed as utf16
                  with the data stored using the current endianess
                - numeric type (byte, short, long ...), it searches
                  a number stored using the current endianess
                - regex, experimental regular expression using a
                  limited set of features (no grouping and others):
                  https://github.com/kokke/tiny-regex-c
                  it also works on binary files (0x00 -> line feed)
                    findloc OFFSET regex "expression"
                    findloc OFFSET regex "START.*END"
      STRING    Must be a number if TYPE is a numeric type, or a
                string in C notation (cstring) in the other cases
      FILENUM   Number of the file associated to the archive (0)
      ERR_VALUE By default FindLoc terminates the script if no
                string is found, if ERR_VALUE is set this value is
                assigned to VAR without terminating when there are
                no other occurrences, the suggested ERR_VALUE is ""
      END_OFF   Limit the scanning from current offset till this
                offset, if END_OFF is lower than the current offset
                then the scanning will be performed backward

    Examples:
      For
          FindLoc OFFSET string "filename="
          ...
          FindLoc OFFSET string "filename=" 0 ""
          if OFFSET == ""
              cleanexit
          endif

          # scan backward
          goto 0 0 SEEK_END
          findloc OFFSET string "filename=" 0 "" 0
          FindLoc OFFSET string "file\x??am\x??"
      Next


...................................................................

For [VAR] [OP] [VALUE] [COND] [VAR]
...
Next [VAR] [OP] [VALUE]

    A classical "for" cycle with initializers, conditions and
    incrementers.
    There is also the Break instruction available to break the
    cycle at any moment and the Continue instruction for skipping
    the remaining part of the cycle.
    "For" allows to perform an initial operation on a variable and
    a check in each cycle to ensure a particular condition.
    "Next" is the command which delimits the cycle and at the same
    time increments the given variable if specified.
    It's also possible to use a math operation in Next so that you
    can increment, decrement or perform any other operation at the
    end of each cycle.
    All the parameters are optionals and must be inserted in the
    specific order, so if there is no initialization you must use:
      For OFFSET = OFFSET < 1000
    For the record, there is also a "Prev" variant of the Next
    command, it just decrements the variable at each cycle.

    Arguments:
      VAR       Variable on which is performed the first math
                operation and is checked for the condition
      OP        Any of the available Math operators (check Math)
      VALUE     Value to assign to the variable or part of the math
                operation
      COND      Condition (check the If command)
      VAR       Second part of the condition

    Examples:
      For i = 0 < FILES
          ...
      next i
      For
         # do what you want here, this is an endless loop
      Next
      For VAR1 = VAR1 != VAR2
         # same of using while(VAR1 != VAR2) {...} in C
      Next VAR2 /= 3
      For OFFSET = OFFSET != ARCHIVE_SIZE
        ...
        Savepos OFFSET
        if OFFSET > 100
          break
        endif
      Next


...................................................................

Get VAR TYPE [FILENUM]

    It reads strings and numbers from the file.
    It supports many types of input, they are listed at the
    beginning of this documentat like byte, short, long, string,
    unicode and so on.
    The tool automatically terminates when there is no data or
    partial data to read at the end of the file.

    Arguments:
      VAR       Variable which will receive the read data
      TYPE      Check the description of the types explained before
      FILENUM   Number of the file associated to the archive (0)

    Examples:
      Get OFFSET long
      Get NAME string


...................................................................

GetDString VAR LENGTH [FILENUM]

    It reads a defined amount of data from the file and stores it
    in the given variable.
    It's useful with filenames and other strings that have a length
    specified in a previous 8, 16 or 32 bit field.

    Arguments:
      VAR       Variable which will receive the read data
      LENGTH    Amount of bytes to read.
                There is also an experimental method in which you
                can specify the elements and their size like
                LENGTH*NUM, for example:
                  getdstring ARRAY NUMBERS*4
      FILENUM   Number of the file associated to the archive (0)

    Examples:
      GetDString NAME NAME_LENGTH
      GetDString NAME 0x100
      getdstring ARRAY ELEMENTS*4


...................................................................

GoTo OFFSET [FILENUM] [TYPE]

    It changes the current position in the file, like fseek in C.

    Arguments:
      OFFSET    Position to reach.
                The offset "SEEK_SET" is offset 0.
                The offset "SEEK_END" is the end of file.
                If it's a constant negative it will be considered
                the amount of bytes from the end of the file, so a
                negative variable is considered as unsigned 32bit.
                The offset depends also by the TYPE field.
      FILENUM   number of the file associated to the archive (0)
      TYPE      - SEEK_SET, absolute offset (default)
                - SEEK_CUR, relative offset from current position
                - SEEK_END, amount of bytes from the end, must be
                  negative or OFFSET will be converted to negative

    Examples:
      GoTo OFFSET
      GoTo 0x100
      GoTo -4           # 4 bytes before the end of the file
      GoTo SEEK_SET     # like goto 0
      Goto SEEK_END     # like goto 0 0 SEEK_END


...................................................................

IDString [FILENUM] STRING

    It terminates the program if the magic/signature at the current
    position of the file differs than the provided string.
    If the string doesn't match and it's 4 bytes long QuickBMS will
    automatically swap it and perform the comparison again and
    change the endianess if it matches. This solution makes most of
    the scripts written for an architecture (for example PC)
    virtually compatible with others (for example Xbox360).
    Pay attention to the FILENUM/VAR order different than other
    commands, that's a rule of the original BMS syntax.

    Arguments
      FILENUM   number of the file associated to the archive (0)
      STRING    string in C notation (cstring), it can also use
                wildcard bytes like "\x??" (they covers only the
                first 32 bytes of the string)

    Examples:
      IDString "PK\x03\x04"
      IDString " KAP"
      IDString MEMORY_FILE "hello"
      IDString 1 "magic_on_file_one"
      IDString "PK\x??\x??"


...................................................................

Log NAME OFFSET SIZE [FILENUM] [XSIZE]

    It extracts the file, this operation doesn't change the current
    position of the input file.
    The content of the extracted file can be automatically
    decrypted using the Encryption command before it.
    If NAME is an empty string like "", QuickBMS will assign a
    sequential hexadecimal number and will try to guess the
    extension based on the content at the beginning of the file.
    The extension will be automatically guessed and appended also
    to all the files that terminate with a dot or an asterisk like
    ".", "*" or ".*" or if they point to folders like "folder/".
    NAME can also be a special file like those that we will see
    later like a socket, a process, an audio device and so on (they
    require previous authorization by the user via command-line).
    The filename will be automatically cleaned for dumping the file
    without problems.
    NAME can also be a MEMORY_FILE or a TEMPORARY_FILE.
    If a file with the same name already exists, QuickBMS will ask
    what action to take, the suggested one is the 'r' choice that
    will allow to automatically rename all the files with the same
    name without overwriting them.
    If you have used the Append command, the data will be appended
    to the existent file with the same name.
    Log and Clog share the same code, so the compression is the only
    difference.

    Arguments:
      NAME      Name of the output file
      OFFSET    Position in the archive where is located the file
      SIZE      Amount of the data to extract
      FILENUM   Number of the file associated to the archive (0)
      XSIZE     Used with block encryptions, this value is the aligned
                amount of data read from the disk, example for AES:
                   log NAME OFFSET 0x123      0 0x130
                  clog NAME OFFSET 0x123 SIZE 0 0x130

    Examples:
      Log NAME OFFSET SIZE
      Log "dump.dat" 0 SIZE
      Log "" 0 SIZE
      Log "folder/name.*" 0 SIZE


...................................................................

Clog NAME OFFSET ZSIZE SIZE [FILENUM] [XSIZE]

    It extracts the file decompressing it in real-time, this
    operation doesn't change the current position of the file.
    The decompression algorithm used in the operation is decided by
    the ComType command which is zlib by default.
    The content of the extracted file can be decrypted
    automatically after decompression using the Encryption command.
    For additional information please refer to the Log command.

    Arguments:
      NAME      Name of the output file
      OFFSET    Position of the archive where is located the file
      ZSIZE     Size of the compressed data in the archive
      SIZE      Size of the uncompressed file, if you have used a
                "_compress" algorithm then use SIZE equal to ZSIZE
                because the tool will automatically calculate the
                maximum amount of bytes taken for the compression
      FILENUM   Number of the file associated to the archive (0)
      XSIZE     Used with block encryptions like AES, just like Log

    Examples:
      Clog NAME OFFSET ZSIZE SIZE
      Clog "dump.dat" 0 ZSIZE 10000000
        # with some compression algorithms the file will have the
        # real size while others will set it to 10000000


...................................................................

Math VAR OP VAR

    Mathematical operation between two variables with the result
    placed in the first one.
    Note that due to compatibility all the operations are performed
    using signed 32 bit numbers by default. It makes the difference
    with some operations like shift and divisions, pay attention!
    For unsigned operations add an 'u' before OP.
    The additional '=' you see in many scripts and in the examples
    is not needed, programmers are used to add it when the first
    variable is both input and output, like in C: var += 123;.

    Arguments
      VAR       Variable which acts as input and output
      OP        +   sum
                *   multiplication
                /   division
                -   substraction
                ^   xor
                &   and
                |   or
                %   modulus
                !   negation of var2 (0 becomes 1 and any other
                    value becomes 0)
                ~   complement of var2 (like "xor 0xffffffff")
                <   shift left (also <<)
                >   shift right (also >>)
                l   rotate left (also <<<)
                r   rotate right (also >>>)
                s   byte swapping, 2 for reverseshort and 4 for
                    reverselong
                w   bit swapping, reverse the amount of bits
                    specified in var2
                =   assign var2 to var1
                n   negative value of var2 (like var1 = -var2)
                a   absolute value of var2 (-10 = 10 and 10 = 10)
                v   radix (also //)
                p   power (also **)
                x   alignment/padding, examples:
                    var1=0   var2=16  result=0
                    var1=1   var2=16  result=16
                    var1=16  var2=16  result=16
                    var1=17  var2=16  result=32
                y   round, like var1=(var1/var2)*var2, examples:
                    var1=0   var2=16  result=0
                    var1=1   var2=16  result=0
                    var1=16  var2=16  result=16
                    var1=17  var2=16  result=16
                z   common bitswapping (also <>):
                    var1=0xab    var2=4  result=0xba
                    var1=0xabcd  var2=4  result=0xdc
                    var1=0xabcd  var2=8  result=0xcdab
                reverselong     swap of 32bit variable
                reverseshort    swap of 16bit variable
                reverselonglong swap of 64bit variable
                binary  convert from binary to decimal
                octal   convert from octal to decimal
                hex     convert from hexadecimal to decimal (this
                        is automatic, use it only if VAR2 doesn't
                        have a 0x prefix)
                base*   convert from base* to decimal, so base8 is
                        octal, base2 is binary, base16 is hex
                Add a 'u' before or after OP for forcing the usage
                of unsigned operations useful with shift, divisions
                and possibly other operations.
                Any operation starting with a '?' is considered a
                verbose operator, for example ?add is the same of +.
                QuickBMS supports also all the functions available in
                math.h like ?sin, ?cos, ?atan and so on.
                Unfortunately it's not possible to list them here,
                please check math_operations() and
                old_set_math_operator() in the cmd.c source code.
      VAR       Second input variable

    Examples:
      Math SIZE * 0x100
      Math OFFSET << 2
      Math OFFSET u<< 2
      Math TMP = SIZE
      Math TMP ~ TMP
      Math TMP n TMP
      Math TMP2 a TMP
      Math SIZE u/ 5
      Math RADIX v 2


...................................................................

XMath VAR INSTR

    Multiple mathematical operations in one line, just a way to
    avoid the limitations of the original Math command.
    Currently this command is just an experiment and supports only
    the most simple operators named with a non-alphanumeric
    character and applied to unsigned numbers:
        ~ ! < > & ^ | * / % - +
        <<< shift left
        >>> shift right
        **  power
        //  root
        &&  alignment
        <>  common bit swapping
        %%  percentage ("VAR %% 15" will return the 15% of VAR)
    This command is directly derived from my calcc tool:
      http://aluigi.org/mytoolz.htm#calcc
    Please note that XMath is a lot slower than Math.
    Do NOT use the unsigned labels or the additional '=' you use
    with the Math command because they are NOT supported since all
    operations are unsigned in XMath, so:
      xmath TMP "TMP u<<= 5"    is WRONG
      xmath TMP "TMP << 5"      is CORRECT

    Arguments
      VAR       Variable that acts as output
      INSTR     The full instruction, the operator must never start
                with a alphanumeric character because it would be
                interpreted as a variable

    Examples:
        XMath VAR "1 + 2 - ((3 + 4) + VAR * VAR2)"
        xmath VAR "VAR ?x 16"
        xmath VAR "VAR ?align 16"


...................................................................

Open FOLDER NAME [FILENUM] [EXISTS]

    It opens a file, basically it assigns a file number/id to an
    existent file that you want to use.
    If NAME is '?':
    - and FOLDER is FDDE the user must type the extension of the
      file to load, the name is the same of the one currently open
    - and FOLDER is FDSE the user must type the name of the file
      loaded from the same folder
    - the user must type the full name of the file to load
    From version 0.9 QuickBMS has introduced the emulated file
    number 0, if you use "open MEMORY_FILE" or "open 1" then any
    operation on the current file will be performed on the chosen
    file, use "open 0" to restore it.

    Arguments:
      FOLDER    FDDE, means that you want to open the file in the
                  same location of the input one which has the
                  extension provided with NAME, so FDDE is for the
                  extension only
                FDSE, it will consider NAME as a file located in
                  the same folder of the input file (very useful)
                FDDE2, like FDDE forcing the original input folder
                FDSE2, like FDSE forcing the original input folder
                any other value is considered the folder where is
                  located the file to load so use "." for the
                  current output folder
      NAME      Read above, NAME can also be a ? in which case
                QuickBMS will ask the user to insert the name of
                the file to open manually if NAME is "" then will
                be performed a flush operation that could be useful
                (or not?) only in write mode (debug)
      FILENUM   Number of the file associated to the archive (0)
      EXISTS    If the file doesn't exist this variable will be set
                to 0 or 1 if it exists. By default QuickBMS
                terminates with an error if the file doesn't exist.

    Examples:
      Open FDDE DAT 0
      Open FDDE IDX 1
      Open FDSE "myfile.zip"
      Open "." TEMPORARY_FILE 1


...................................................................

SavePos VAR [FILENUM]

    Current position of the file, like ftell in C.

    Arguments:
      VAR       Variable which will contain the offset
      FILENUM   Number of the file associated to the archive (0)

    Examples:
      SavePos OFFSET


...................................................................

Set VAR [TYPE] VAR

    Command for assigning a constant or a variable to another
    variable with the possibility of changing its type, like utf8
    to unicode and vice versa, and so on.

    Arguments:
      VAR       Output variable or memory file
      TYPE      In general the type is not much important because
                in QuickBMS there is almost no difference between
                numbers and strings, these are the special types:
                - unicode, unicode to utf8, endian dependent
                  set NAME unicode NAME
                - to_unicode, utf8 to unicode, endian dependent
                  set NAME to_unicode NAME
                - binary, C notation (cstring)
                  set MEMORY_FILE binary "\x11\x22\x00hello"
                - alloc: allocates memory, something like
                  putvarchr VAR SIZE 0 ; set VAR alloc 0x1234
                - filename: example that returns "myfile.txt":
                  set NAME filename "c:\folder\myfile.txt"
                - basename: for example it returns "myfile"
                - filepath
                - fullbasename
                - extension: the extension part from a string (txt)
                - unknown: the user is prompted to insert the
                  content of the variable: set VAR ? ?
                - signed_byte/short/threebyte/long
                - unicode32: utf32
                - strlen: just a wrapper for the Strlen command
                  set NAMESZ strlen NAME
      VAR       Variable or constant to assign

    Examples:
      Set i long 0
      Set TMP long SIZE
      Set TMPNAME NAME
      Set MEMORY_FILE binary "\x12\x34\x56\x78"
      Set ASCII_VAR unicode UNICODE_VAR # from unicode to string
      Set VAR ? ?   # the user is prompted to insert a filename


...................................................................

Do
...
While VAR COND VAR

    A less useful type of cycle where the check of the condition is
    performed at the end of the cycle... really rarely used.
    If you need a C-like "while(...) {...}" use the For command.

    Arguments:
      VAR       first part of the condition
      COND      condition, check the If command below for more info
      VAR       second part of the condition

    Examples:
      Do
          ...
      While OFFSET < MAX_OFFSET


...................................................................

String VAR OP VAR

    The equivalent of the Math command for the strings.
    The first variable can be an input and output or only an output
    depending by the operator.
    You can use also a textual OP, this value is the one in the
    first line of the operator seen below ("equal" is '=').
    The string searching operators are quite confusing because the
    tool didn't have this feature and they were implemented in the
    String command later as experimental features.
    The variables are used as NUL-delimited strings by default, but
    from QuickBMS 0.11 you can use them in binary mode by prefixing
    the operator with a zero ("0").

    Arguments:
      VAR       Input and output variable
      OP        The following examples are based on these values:
                  VAR1      "MyStringExampleString!" (22 bytes)
                  1 VAR2    "STRING"
                  2 NUM2    3
                  3 NUM2    -3
                =   just a copy, if var2 is a number it's used as a
                    raw string, good for Long to String conversions:
                      var2="0x44434241", result="ABCD"
                +   append
                      MyStringExampleString!STRING
                      MyStringExampleString!3
                      MyStringExampleString!-3
                -   remove, truncate
                      MyExample!
                      MyStringExampleStri
                      MyS
                ^   xor
                      .-.=<.=3.1/*#87.:5::5h
                      ~J`GAZ]TvKR^C_V`GAZ]T.
                      `J~G_ZCThKL^]_H`YAD]J.
                <   strrstr + var2 (before)
                      MyStringExampleString
                      tringExampleString!
                      ng!
                *   replicate
                      MyString
                      MyStringExampleString!MyStringExampleString!MyStringExampleString!
                      MyStringExampleString!MyS
                %   strstr (before), truncate, mod
                      My
                      M
                      MyStringExampleString!
                &   strstr
                      StringExampleString!
                      MyStringExampleString!
                      MyStringExampleString!
                |   strstr + var2
                      ExampleString!
                      MyStringExampleString!
                      MyStringExampleString!
                $   strrstr
                      String!
                      MyStringExampleString!
                      MyStringExampleString!
                !   strrstr + var2
                      !
                      MyStringExampleString!
                      MyStringExampleString!
                >   strrstr (before)
                      MyStringExample
                      MyStringExampleStri
                      MyStringExampleString!
                r   reverse
                    reversed string, "abcd" -> "dcba"
                b   byte2hex
                    byte2hex of var2: var2="abc", result="616263"
                B   byte2hex_string
                    as above but var2 is a NUL delimited string
                h   hex2byte
                    hex2byte of var2: var2="616263", result="abc"
                e   encrypt, encryption
                    experimental, based on the Encryption command
                E   encrypt_string
                    as above but var2 is a NUL delimited string
                c   compress, compression, comtype
                    experimental, based on the ComType command
                C   compress_string
                    as above but var2 is a NUL delimited string
                u   toupper
                    var2="hello", result="HELLO"
                l   tolower
                    var2="HELLO", result="hello"
                R   replace
                    replace chars: var1="helloworld", var2="world", var3="me", result="hellome"
                p   printf, sprintf
                    a printf-like experimental work-around
                    the format for float (f) and double (g) works
                    only for one element, so:
                      get VAR_LONG long
                      String TMP p "%10.10f" VAR_LONG # no VAR2 or VAR3
                      print "%TMP%"
                P   QuickBMS Print
                    same output of the Print command, for example:
                      string VAR P "hello %VAR1% test %VAR2|x%"
                s   sscanf
                    a sscanf-like experimental work-around, only
                    for numeric 32bit values
                      string "123:456" s "%d:%d" VAR1 VAR2
                S   split
                    it's like a sscanf for strings, both ' and "
                    are handled as quotes:
                    string ELEMENTS S "string1 \"string 2\" 'string3'" VAR1 VAR2 VAR3
                x   cstring
                    convert a C string (cstring) to the relative
                    string/binary:
                      string VAR x "\x78\x7a"
                H   string to cstring, all bytes escaped if with
                    "0" prefix
                      string VAR 0H "hello" # VAR = "\x68\x65\x6c\x6c\x6f"
                f   filter
                    filter the non alphanumeric chars by replacing
                    them with '_'
                m   math, xmath
                    math and xmath operation just like those in the
                    Encryption command
                    so #INPUT#+1 means that 0x01 will be added to each char of VAR
                    quick example:  string VAR m "#INPUT#+1"    # xmath if there is INPUT
                                    string VAR m "+ 1"          # math
                w   hex2uri
                    var2="%2fhello&amp;", result="/hello&"
                W   uri2hex
                    var2="hello<>", result="hello%3c%3e"
                t   very basic html/xml tags remover, de_html
                T   html/xml, one tag or text per line, html_easy
                _   trim, removes spaces from the beginning and end
                J   JSON formatter, json_viewer
                X   experimental parser for XML, JSON and other
                    formats (use option -9), xml_json_parser:
                      https://zenhax.com/viewtopic.php?t=4887&p=26349#p26349
                    currently it automatically escapes backslashes
                    and HTML tags (backslashes added in 0.11, in
                    theory the HTML tags are not in the standard)
                    nested elements are stored as variables: VAR[i]
                    so remember to use the "i" index to read them:
                      string RET X INPUT
                      for i = 0 < NAME[]
                        print "%NAME[i]%"
                      next i
                v   CSV with custom separators like "," or ",|;"
                      string ELEMENTS v "arg1,arg2,  arg 3 , arg4" "," ARG1 ARG2 ARG3 ARG4
                n   byte2num
                    var2="abc", result="97 98 99"
                N   num2byte
                    var2="97 98 99" result="abc"
                U   base64/uudecode
                Use an additional zero ("0") to return "" in case
                of errors like when the operators that search
                strings can't find the pattern (in which case will
                be returned the original string by default), this
                is very useful while playing with strings, so
                "string VAR1 0strchr VAR2" will return "" if VAR2
                is not found in VAR1 (instead of leaving VAR1
                unchanged), another example: String VAR1 0$ VAR2
                From version 0.11 the "0" prefix is also used for
                working with binary strings.
      VAR       The second variable or string

    Examples:
      string FULLPATH + NAME
      string FULLPATH + \
      string NAME - ".zip"
      string NAME - 4
      string PATH R "." "/"
      string FULLPATH p "c:\folder\%04x%04x.dat" VAR1 VAR2 # input
      string FULLPATH s "c:\folder\%04x%04x.dat" VAR1 VAR2 # output


...................................................................

CleanExit

    Terminates the script, it's possible also to use just Exit.


...................................................................

If VAR COND VAR [...]
...
[Elif VAR COND VAR]
...
[Else]
...
EndIf

    It checks various conditions and performs the needed operation
    when the condition is verified, in short:
    - If is ever the first condition
    - Elif is another condition and can be used endless times
    - Else is the operation to do when no conditions are met, last
    - EndIf delimits the If command statement
    It's also possible to use multiple conditions (max 4) like:
      if VAR1 < VAR2 && VAR3 > VAR4
      elif VAR1 != 0 || VAR2 != 0
    The 'u' added before the condition forces an unsigned
    comparison with numbers and a case sensitive one with strings.
    The condition is considered for both strings and numbers, for
    more technical details check the check_condition() function
    in the cmd.c source code.

    Arguments:
      VAR       First part of the condition
      COND      Valid for both strings and numbers:
                <   minor, lower, below
                >   major, greater, above
                !=  different, <> !==
                ==  equal, = === strcmp stricmp strcasecmp
                >=  major/equal
                <=  minor/equal
                &   string: var2 is included in var1 (strstr)
                    number: logical AND
                ^   string: equal
                    number: logical XOR
                |   number: logical OR
                %   number: modulus
                /   number: division
                <<  number: shift left
                >>  number: shift right
                !   number: negation, not
                !!  number: true, use it to know if VAR is non-zero
                ~   number: complement
                strncmp     if "mystring" strncmp "myst"
                ext compares the string after the last dot
                basename    compares the string before the last dot
                filepath    compares the part before the filenames,
                            you can force a folder without filename
                            by appending a slash: "c:\folder/"
                            instead of "c:\folder" (will be "c:")
                any other operation supported by the Math command
                  (valid only for the numeric variables)
                use 'u' before COND for forcing the usage of
                unsigned operations useful with shift, divisions
                and possibly other operations, if the variables
                are strings then it will perform an case sensitive
                comparison instead of the default insensitive one,
                while the '0' prefix before COND works just like in
                String performing a binary comparison
      VAR       Second part of the condition

    Examples:
      If NAME != ""
          ...
      Endif
      If MASK & 1
      Elif MASK & 2
      Elif MASK & 4
      Elif MASK & 8
      Else
      Endif


...................................................................

GetCT VAR TYPE CHAR [FILENUM]

    It reads a string till the reaching of the CHAR delimiter.

    arguments
      VAR       Output variable
      TYPE      Only unicode is the alternative type, any other
                value is just ignored because doesn't matter for
                this operation
      CHAR      The delimiter character as 8bit number, if this
                number is negative QuickBMS will convert it to
                positive and read till the current byte is the
                same (like skipping the same byte in a file)
      FILENUM   Number of the file associated to the archive (0)

    Examples:
      GetCT NAME string 0x0a
      GetCT NAME string 0x3b
      set DELIMITER_BYTE long 0x0a
      GetCT NAME string DELIMITER_BYTE
      GetCT NAME unicode 0x0a


...................................................................

ComType ALGO [DICT] [DICT_SIZE]

    It selects a specific compression algorithm to use with the
    Clog command.
    It's also possible to choose a number as ALGO, this is a
    feature coming from my project for a compression scanner and
    brute-forcer able to guess the possible algorithm of an
    unknown raw compressed data block by simply trying every
    decompressor on the input (do NOT use it if you don't know what
    you are doing! this is NOT offzip!):
      http://aluigi.org/bms/comtype_scan2.bat
      http://aluigi.org/bms/comtype_scan2.bms
      comtype_scan2.bat comtype_scan2.bms input_file output_folder
      comtype_scan2.bat comtype_scan2.bms input_file output_folder uncompressed_size
    Note that some algorithms may work on Windows only.
    if ALGO is "?", the user is prompted to type the desired
    algorithm name.

    Arguments:
      ALGO      copy, simple copy that useful in some rare cases
                  with data encrypted with block ciphers like AES
                  and blowfish so use comtype copy and encryption
                zlib, RFC1950 (windowbit 15, data starts with 'x')
                  DICT supported
                  if DICT starts with "Z_FULL_FLUSH"
                  inflate/deflate will use Z_FULL_FLUSH
                deflate, RFC1951 (windowbit -15) used in ZIP files
                  DICT supported
                  if DICT starts with "Z_FULL_FLUSH"
                  inflate/deflate will use Z_FULL_FLUSH
                lzo1x
                  DICT supported
                lzo1a till lzo2a, LZO (the most used is lzo1x!)
                  DICT supported
                lzss, with default configuration (window of 4096)
                  this particular algorithm can be fully configured
                  setting the EI, EJ and P fields plus another
                  number rarely used:
                    EI, EJ, P, rless, init_chr
                  for setting them it's enough to use a DICT equal
                  to something like "12 4 2" which means
                  EI:12 (N:4096), EJ:4 (F:18), P:2
                  use a negative init_chr for different window
                  slide customization like -1 for Tales of Vesperia
                  the default character of lzss is a space (0x20),
                  but you can use the ALGO lzss0 or the dictionary
                  "12 4 2 2 0" to use 0x00
                lzx, used by the old unlzx tool and on Amiga
                gzip, automatic handling of gzip data, in this case
                  the uncompressed size is ignored and calculated
                  automatically so in CLog use ZSIZE ZSIZE
                pkware, algorithm also known as blast, explode,
                  implode or DCL
                lzma, 5 bytes + lzma (in some cases you may need to
                  use ZSIZE + 5)
                lzma86head, 5 bytes + 8 bytes (size) + lzma
                lzma86dec, 1 byte + 5 bytes + lzma (in some cases
                  you may need to use ZSIZE + 5)
                lzma86dechead, 1 + 5 + 8 bytes (size) + lzma
                lzmaefs, the format implemented in ZIP
                bzip2
                XMemDecompress, (xmemlzx) Xbox 360 LZX algorithm of
                  xcompress.lib use DICT to specify a custom
                  WindowSize and CompressionPartitionSize like
                  "131072 524288", the algorithm automatically
                  idenfities and extracts the LZXTDECODE and
                  LZXNATIVE files generated by xbcompress.exe
                  (0x0FF512ED / 0x0FF512EE)
                hex, from "01234567" to bytes: 0x01 0x23 0x45 0x67
                base64, from "aGVsbG8=" to "hello", also supports
                  the Gamespy and URL chars
                uudecode
                ascii85
                yenc
                COM_LZW_Decompress, used in Vietcong
                milestone_lzw, lzw used in the Milestone games
                lzxcab, lzx used in cab files (libmspack 21 0)
                lzxchm, lzx used in chm files (libmspack 16 2)
                rlew, 16 bit RLE algorithm used in AIM Racing
                lzjb, a compression used in a file system for *nix
                sfl_block, expand_block from iMatix SFL
                sfl_rle, expand_rle from iMatix SFL
                sfl_nulls, expand_nulls from iMatix SFL
                sfl_bits, expand_bits from iMatix SFL
                lzma2, 1 bytes + lzma2
                lzma2_86head, 1 bytes + 8 bytes (size) + lzma2
                lzma2_86dec, 1 byte + 1 bytes + lzma2
                lzma2_86dechead, 1 + 1 + 8 bytes (size) + lzma2
                nrv2b, UCL
                nrv2d, UCL
                nrv2e, UCL
                huffboh, used in the Asura engine
                uncompress, lzw used in the compress utility, use
                  dict for bits (the lzw data starts from offset 3
                  of the .Z files)
                dmc, Dynamic Markov Compression (DMC)
                lzhuf, aka LZH/LHA
                lzari
                rle7
                rle0
                rle
                rlea, another generic rle decompressor
                  use DICT to choose the escape char
                bpe, byte pair encoding
                quicklz
                q3huff, Adaptive Huffman used in the Quake 3 engine
                unmeng, algorithm used in DreamKiller
                lz2k, used in various games of Traveller's Tales
                darksector, lzfx used in the game Dark Sector
                mszh, used in the LossLess Codec Library
                un49g, used in the games of 49Games
                unthandor, used in the old game Thandor
                doomhuff, huffman used in doom, hexen, skulltag and
                  other doom ports, the DICT field can be used to
                  specify a custom HuffFreq table (256 floats)
                aplib
                tzar_lzss, used in Tzar of HaemimontGames
                  DICT must contain the name of the variable with
                  the algorithm number to use, example:
                  ComType tzar_lzss MYVAR
                lzf, aka fastlz
                clz77, http://compressions.sf.net/about.html
                lzrw1
                dhuff, Huffman Decompression in LDS ("lossless
                  datacompression sources" kit 1.1)
                fin, from LDS
                lzah (not tested)
                lzh12, aka -lh4-
                lzh13, aka -lh5-
                grzip, aka GRZipII
                ckrle, Chilkat RLE
                quad, raw data without the first 32bit field
                balz, raw data without the first 9 bytes from files
                deflate64
                shrink (not tested)
                z-base-32
                base32hex
                base32crockford
                base32nintendo
                base???, if ALGO starts with "base" then will be
                  taken its subsequent number (for example 32 if it
                  is "base32") and used for the conversion. the
                  function supports ANY base from 2 to 256.
                  for bases larger than 64 will be used a table
                  starting from byte 0x00 so base128 will have a
                  charset from 0 to 0x7f
                brieflz
                paq6, raw data block
                shcodec
                hstest_hs_unpack (never tested, may be removed)
                hstest_unpackc (never tested, may be removed)
                sixpack (never tested)
                ashford (never tested, may be removed)
                jcalg
                jam
                lzhlib
                srank
                zzip
                scpack
                  DICT supported (for the SCPACK_TABLE field)
                rle3
                bpe2
                bcl_huf, Basic Compression Library
                bcl_lz, Basic Compression Library
                bcl_rice, Basic Compression Library
                  you must use DICT to specify the format (1 to 8)
                bcl_rle, Basic Compression Library
                bcl_sf, Basic Compression Library
                scz
                szip
                ppmd, ppmd var.i rev.1 with ZIP specifics so 2
                  bytes of info followed by the compressed data
                ppmdi_raw
                ppmdg
                ppmdg_raw, requires DICT "SASize MaxOrder"
                ppmdh
                ppmdh_raw, requires DICT "SASize MaxOrder"
                ppmdj
                ppmdj_raw, requires DICT "SASize MaxOrder CutOff"
                sr3c
                huffmanlib
                sfastpacker, smart+simple mode
                sfastpacker2, smart-mode only
                dk2, RefPack used in Dungeon Keeper 2 and other
                  Bullfrog/EA games
                lz77wii, (use input size as output size in clog)
                lz77wii_raw10, tag 0x10 lz77
                darkstone, lz77 used in the game DarkStone
                sfl_block_chunked, as sfl_block with automatic
                  handling of the chunks if used
                yuke_bpe, used in the PS2 games developed by Yuke
                stalker_lza, used in STALKER, use the output size
                  equal to the compressed one (handled by quickbms)
                prs_8ing
                puyo_cnx, raw compressed data from offset 0x10
                puyo_cxlz, raw compressed data from offset 0x8
                puyo_lz00, raw compressed data from offset 0x32
                puyo_lz01, raw compressed data from offset 0x10
                puyo_lzss, raw compressed data from offset 0x4
                puyo_onz, raw compressed data from offset 0x4
                puyo_prs
                falcom
                cpk, used by the CRI developers (LAYLA)
                bzip2_file, exactly like bzip2 but it calculates
                  the output size, use ZSIZE ZSIZE in clog
                lz77wii_raw11, tag 0x11 lzss
                lz77wii_raw20, tag 0x20 huffman
                lz77wii_raw30, tag 0x30 rle
                lz77wii_raw40
                pglz, postgresql compression (headerless)
                UnPackSLZ
                slz_01, used in tri-ace slz type 1
                slz_02, used in tri-ace slz type 2
                slz_03, used in tri-ace slz type 2
                lzhl
                d3101
                squeeze
                lzrw3
                tdcb_ahuff
                tdcb_arith
                tdcb_arith1
                tdcb_arith1e
                tdcb_arithn
                tdcb_compand
                tdcb_huff
                tdcb_lzss, dict for INDEX_BIT_COUNT,
                  LENGTH_BIT_COUNT, DUMMY9, END_OF_STREAM
                tdcb_lzw12
                tdcb_lzw15v
                tdcb_silence
                rdc
                ilzr
                dmc2
                diffcomp
                lzr
                lzs (aka mppc but NOT exactly the same!)
                lzs_big (aka mppc_big but NOT exactly the same!)
                mohlzss
                mohrle
                yaz0 (aka szs)
                byte2hex
                un434a
                xxdecode
                pack, the one supported in gzip
                unzip_dynamic, automatic zlib/deflate and output
                  size, please note that while zlib is almost error
                  free due to the checksum at its end, deflate
                  doesn't guarantee a valid output so, even if it
                  uncompresses the data, it may be invalid. use
                  zlib_noerror if you are 100% sure that the input
                  is zlib or plain, and deflate_noerror for deflate
                zlib_noerror, as zlib but doesn't quit in case of
                  errors and automatically reserves the output size
                deflate_noerror, as above but for deflate
                ppmdh
                ppmdh_raw
                rnc
                rnc_raw
                pak_explode, alone in the dark
                KENS_Nemesis
                KENS_Kosinski
                KENS_Kosinski_moduled
                KENS_Enigma
                KENS_Saxman
                dragonballz (STPZ/0DCS/0LCS/STPK archives, Spyke?)
                NitroSDK (nitroCompLib)
                zdaemon, like doomhuff but different freq table
                skulltag, like doomhuff but different freq table
                msf, headerless lzma same as lzma_0
                stargunner
                ntcompress
                crle
                ctw
                DACT_DELTA
                DACT_MZLIB2
                DACT_MZLIB
                DACT_RLE
                DACT_SNIBBLE
                DACT_TEXT
                DACT_TEXTRLE
                EXECUTE:
                  use DICT to specify the command to execute using
                  #INPUT# instead of input filename and #OUTPUT#
                  for the output one and the various variables like
                  you do for the Print command, example:
                  comtype EXECUTE "ctw.exe d #INPUT# #OUTPUT#"
                  comtype EXECUTE "ctw.exe d #INPUT# %NAME%"
                  clog "output.dat" 0 ZSIZE ZSIZE   # no need SIZE
                CALLDLL:
                  as above but allows to specify a calldll command
                  executed on input: "#INPUT#", "#INPUT_SIZE#",
                  "#OUTPUT#", "#OUTPUT_SIZE#" and %VAR%
                  full support also for pointers using the '&' or
                  '*' prefix like &MEMORY_FILE, &VAR, &#INPUT#,
                  &INPUT_SIZE
                lz77_0
                lzbss
                bpaq0
                lzpx, lzpxj
                mar_rle
                gdcm_rle
                lzmat
                dict
                rep
                lzp (it's a preprocessor, not a real compression)
                elias_delta
                elias_gamma
                elias_omega
                packbits
                darksector_nochunks, aka lzf or lzfx
                enet
                eduke32, lzwuncompress
                xu4_rle
                rvl, lemur int compression
                lzfu, MS RTF
                lzfu_raw
                xu4_lzw, Ultima 4
                he3, without the HE3\x0d signature and output size
                iris, Ultima Online algorithms
                iris_huffman
                iris_uo_huffman
                ntfs
                pdb
                COMPRLIB_SPREAD
                COMPRLIB_RLE1
                COMPRLIB_RLE2
                COMPRLIB_RLE3
                COMPRLIB_RLE4
                COMPRLIB_ARITH
                COMPRLIB_SPLAY
                cabextract, it may be the same lzx of mspack
                mrci
                hd2_01
                hd2_08
                hd2_01raw
                rtl_lznt1
                rtl_xpress, looks not supported by XP/7
                rtl_xpress_huff, looks not supported by XP/7
                prs
                sega_lz77
                saint_seya, used for GMI compression
                ntcompress30
                ntcompress40
                yakuza, used by SEGA CS1 team
                lz4 (the algorithm of lz4hc is the same)
                snappy
                lunar_lz1  to lunar_lz19
                lunar_rle1 to lunar_rle4
                goldensun
                luminousarc
                lzv1
                fastlzah, it should be identical to lzf and lzfx
                zax
                shrinker
                mmini_huffman
                mmini_lz1
                mmini
                clzw
                lzham, use the dictionary to specify the following
                  fields: m_dict_size_log2, m_table_update_rate,
                  m_decompress_flags, m_table_max_update_interval,
                  m_table_update_interval_slow_rate
                  it will try to brute force the first 3 fields
                lpaq8
                sega_lzs2, automatic CM/lzs2 and decompressed size
                wolf
                coreonline
                mszip, "CK" included (from libmspack)
                qtm, (from libmspack)
                mslzss, (from libmspack)
                mslzss1, (from libmspack)
                mslzss2, (from libmspack)
                kwaj, mslzh (from libmspack)
                lzlib (lzip)
                dflt
                lzma_dynamic, automatic output size and automatic
                  scanning of any supported flag, so it should
                  blindly work against any compressed lzma input.
                  if it's not compressed, the input will be copied
                  on the output.
                  while "lzma" returns an error if the input buffer
                  doesn't have other compressed bytes
                  (LZMA_STATUS_NEEDS_MORE_INPUT), lzma_dynamic
                  gives the ok, this is useful with some rare cases
                  like Far Cry 3 (fat2_fat3.bms)
                lzma2_dynamic, automatic output size
                lzma2_efs
                lzxcab_delta
                lzxchm_delta
                ffce
                SCUMMVM1 -> SCUMMVM53, many algos used in Scummvm
                lzs_unzip, PSP_Nanoha
                legend_of_mana
                dizzy
                edl1
                edl2
                dungeon_kid
                frontmission2
                rleinc1
                rleinc2
                evolution, aka lzf or lzfx
                puyo_lz10
                puyo_lz11
                nislzs
                unknown1 -> unknown19
                blackdesert
                blackdesert_raw
                pucrunch
                zpaq
                zyxel_lzs
                blosc
                gipfeli
                crush
                yappy
                lzg
                doboz
                tornado
                xpksqsh
                amiga_unsquash
                amiga_bytekiller
                amiga_flashspeed
                amiga_iamice
                amiga_iamatm
                amiga_isc1p
                amiga_isc2p
                amiga_isc3p
                amiga_upcomp
                amiga_uphd
                amiga_bytekiller3
                amiga_bytekiller2
                amiga_crunchmania17b
                amiga_powerpacker
                amiga_stonecracker2
                amiga_stonecracker3
                amiga_stonecracker4
                amiga_crunchmaster
                amiga_crunchmania
                amiga_crunchmaniah
                amiga_crunchomatic
                amiga_discovery
                amiga_lightpack
                amiga_mastercruncher
                amiga_maxpacker
                amiga_megacruncher
                amiga_packit
                amiga_spikecruncher
                amiga_tetrapack
                amiga_timedecrunch
                amiga_tryit
                amiga_tuc
                amiga_turbosqueezer61
                amiga_turbosqueezer80
                amiga_turtlesmasher
                amiga_dms
                amiga_packfire
                alba_bpe
                alba_bpe2
                flzp
                sr2
                sr3
                bpe2v3
                bpe_alt1
                bpe_alt2
                cbpe
                scpack0
                LZMA_0, headerless lzma, use the dictionary to
                  specify the dictionary size IF necessary
                LZMA_86HEAD0
                LZMA_86DEC0
                LZMA_86DECHEAD0
                LZMA_EFS0
                LZMA2_0
                LZMA2_86HEAD0
                LZMA2_86DEC0
                LZMA2_86DECHEAD0
                LZMA2_EFS0
                lzovl
                NITROSDK_DIFF8
                NITROSDK_DIFF16
                NITROSDK_HUFF8
                NITROSDK_HUFF16
                NITROSDK_LZ
                NITROSDK_RL
                qcmp
                sparse
                stormhuff
                gzip_strict, gzip without autoguessing the presence
                  of crc32 and isize at the end, use this if the
                  file is a real 100% gzip file
                CT_HughesTransform
                CT_LZ77
                CT_ELSCoder
                CT_RefPack
                qfs, same as dk2
                PXP
                BOH
                GRC
                ZEN
                LZHUFXR
                FSE
                FSE_RLE
                ZSTD, automatically supports all the legacy algos
                CSC
                RNCb
                RNCb_RAW
                RNCc_RAW
                AZO
                PP20
                DS_BLZ
                DS_HUF
                DS_LZE
                DS_LZS
                DS_LZX
                DS_RLE
                FAB
                LZ4F
                PCLZFG
                LZOO
                DELZC
                DEHUFF
                HEATSHRINK
                NEPTUNIA
                SMAZ
                LZFX
                PITHY
                ZLING
                DENSITY
                BROTLI
                RLE32
                RLE35
                BSC
                SHOCO
                WFLZ
                FASTARI
                RLE_ORCOM
                DICKY
                SQUISH
                LZNT1
                XPRESS
                XPRESS_HUFF
                LZJODY
                TRLE
                SRLE
                MRLE
                JCH
                LZRW1KH
                LZSS0
                LHA_lz5
                LHA_lzs
                LHA_lh1
                LHA_lh4
                LHA_lh5
                LHA_lh6
                LHA_lh7
                LHA_lhx
                LHA_pm1
                LHA_pm2
                SQX1, currently it doesn't work at 100%
                MDIP_ARAD
                MDIP_ARST
                MDIP_DELTA
                MDIP_FREQ
                MDIP_HUFFMAN
                MDIP_CANONICAL
                MDIP_LZSS
                MDIP_LZW
                MDIP_RICE
                MDIP_RLE
                MDIP_VPACKBITS
                bizarre
                bizarre_skip
                lzssx
                ash
                YAY0
                DSTACKER
                DSTACKER_SD3
                DSTACKER_SD4
                DBLSPACE
                DBLSPACE_JM
                XREFPACK, just another dk2/refpack
                XREFPACK0, as above but without handling of header
                qcmp2, UFG::qDecompressLZ probably the same of qcmp
                deflatex, deflate used in Aeriagames pkg.idx, DICT
                  for btype order ("012")
                zlibx, as above but skips the first 2 bytes
                lzrw1a
                lzrw2
                lzrw3a
                lzrw5
                LEGO_IXS
                mcomp,   libmcomp / MCMQ / MCMP
                mcomp0,  rolz
                mcomp1,  rolz3 (or deflate fast on mcomp.exe)
                mcomp2,  lz
                mcomp3,  deflate
                mcomp4,  deflate64
                mcomp5,  bzip2
                mcomp6,  ppmdj
                mcomp7,  sl
                mcomp8,  sm
                mcomp9,  dmc
                mcomp10, ??? (10,11,12,18,19,20 are all the same)
                mcomp13, fpw1
                mcomp14, fpw2
                mcomp15, fpw3
                mcomp16, fpw4
                mcomp17, pwcm
                irolz
                irolz2
                uclpack
                ace
                ea_comp
                ea_huff
                ea_jdlz
                tornado_byte
                tornado_bit
                tornado_huf
                tornado_ari
                lbalzss1
                lbalzss2
                dbpf, Maxis DBPF
                TITUS_LZW
                TITUS_HUFFMAN
                KB_LZW
                KB_DOSLZW
                CARMACK
                MBASH
                DDAVE
                GOT
                SKYROADS
                ZONE66
                EXEPACK
                DE_LZW
                JJRLE
                K13RLE
                SFRLC
                WESTWOOD1
                WESTWOOD3
                WESTWOOD3b
                WESTWOOD40
                WESTWOOD80
                PKWARE_DCL
                TERSE
                TERSE_SPACK_RAW
                TERSE_PACK_RAW
                REDUCE1
                REDUCE2
                REDUCE3
                REDUCE4
                LZW_ENGINE, requires the following parameters:
                  - initial codeword length (in bits)
                  - maximum codeword length (in bits)
                  - first valid codeword
                  - EOF codeword is first codeword
                  - reset codeword is shared with EOF
                  - flags (check src/compression/filter-lzw.h)
                LZW_BASH
                LZW_EPFS
                LZW_STELLAR7
                ULTIMA6
                LZ5
                LZ5F
                YALZ77
                LZKN1
                LZKN2
                LZKN3
                TFLZSS
                SYNLZ1
                SYNLZ1b
                SYNLZ1partial
                SYNLZ2
                PPMZ2
                OPENDARK
                DSLZSS
                KOF, not working at 100%
                KOF1, not working at 100%
                RFPK
                WP16
                LZ4_STREAM
                OODLE
                OODLE_LZH
                OODLE_LZHLW
                OODLE_LZNIB
                OODLE_LZB16
                OODLE_LZBLW
                OODLE_LZNA
                OODLE_BitKnit
                OODLE_LZA
                OODLE_LZQ1, OODLE_kraken
                OODLE_LZNIB2, OODLE_Mermaid
                SEGS
                OODLE_Selkie
                OODLE_Akkorokamui
                ALZ
                REVELATION_ONLINE
                ps_lz77
                lzfse
                zle
                KOF2, not working at 100%
                KOF3, not working at 100%
                HSQ
                FACT5LZ
                LZCAPTSU
                TF3_RLE
                WINIMPLODE
                DZIP
                DZIP_COMBUF, just a placeholder, doesn't work
                LBALZSS1X, first version of LBALZSS1 (then fixed)
                LBALZSS2X, first version of LBALZSS2 (then fixed)
                GHIREN
                FALCOM_DIN, automatically parses the chunks
                FALCOM_DIN1, just input->output
                FALCOM_DIN0, mode 0
                FALCOM_DINX, mode X
                GLZA
                M99CODER
                LZ4X
                TAIKO
                LZ77EA_970
                DRV3_SRD
                RECET
                LIZARD, it's just the new name/version of LZ5
                MICROVISION
                DR12AE
                MSPACK, requires parameters
                KONAMIAC
                WOLF0, just WOLF without header
                ARTSTATION
                LEVEL5
                ZENPXP, dict must contain 0, 1, 2, 3, 4, 0xd, 0xe
                  or others supported
                ZENPXP1, raw algo 1 of zenpxp
                ZENPXP2, raw algo 2 of zenpxp
                ZENPXP34, raw algo 3,4 of zenpxp
                ZENPXPde, raw algo d,e of zenpxp
                liblzs
                SHREK
                EA_MADDEN
                nvcache, used by NVIDIA NV_Cache bin/toc
                DE_HTML, same as the String command
                HTML_EASY, same as the String command
                JSON_VIEWER, same as the String command
                XML_JSON_PARSER, same as the String command
                OodleNetwork1UDP_State_Uncompact
                OodleNetwork1_Shared_SetWindow, dict number of bits
                OodleNetwork1UDP_Decode, requires the previous
                  usage of OodleNetwork1_Shared_SetWindow and
                  OodleNetwork1UDP_State_Uncompact
                OodleNetwork1UDP_Encode
                qcmp1, raw algorithm used in Sleeping Dogs
                ykcmp
                lzwab
                ncompress, like uncompress plus the 3 bytes header
                swzap
                mzx
                LZRRV
                BCM
                ULZ
                SLZ_ROF, untested probably not working
                LZ4X_NEW
                COPY2
                SLZ_03b
                MPPC
                MPPC_BIG
                alzss
                clz
                GTC
                ANCO
                ANCO0
                ANCO1
                ANCO2
                ANCO3
                ANCO4
                ANCO5
                konami_lz77
                vct_lzs
                umesoft
                systemaqua_catf
                sogma
                pac_ads
                ail_lzs
                agsi
                foster_fa2
                an21
                arc_link
                maika_bk
                maika_mk2
                propeller_mgr
                qlie
                avg32_seen
                sas5_iar
                seraphim_scn
                ugos_det
                aaru_fl4
                inspire_ida
                kurumi_mpk
                dice_rlz
                pulltop
                vnsystem
                QlzUnpack
                umesoft_pk
                tomcat_tcd
                tail_pren
                tail_crp0
                tail_hp
                tactics_arc
                sviu_pkz
                nekox_gpc
                rec_arc
                warc
                warc10
                warc_ylz
                warc_huff
                sh_him
                pandora_pbx
                origin_lz
                origin_huffman
                origin_rle
                origin_alphav2
                garbro_huffman
                ankh_grp
                ankh_hdj
                caramelbox_arc3
                caramelbox_arc4
                circus_V1
                circus_V2
                circus_V3
                cmvs_cpz
                daisystem_pac
                ethornell_bgi
                fc01_mrg
                fc01_mrg_quant
                fc01_pak_lz
                favorite_lzw
                frontwing_rle
                frontwing_huffman
                g2_gcex
                gss_arc
                hypatia_mariel
                interheart_fpk
                kaguya_ari
                kaguya_lin2
                kaguya_link
                kaguya_uf
                kid_dat
                lambda_lax
                microvision_arc
                moonhir_fpk
                spack
                azsys
                dxlib
                glibg
                gamesystem_cmp
                puremail
                groover_pcg
                mnp_mma
                strikes_pck
                sega_lz77x
                neptunia0, headerless chunk
                puff8
                lzh8
                romchu
                lzsd_of
                lzsd_gfd
                lzsd_gba2
                pzz
                SL01
                rage_xfs
                wangan1
                wangan2
                wangan3
                wangan5
                LZ48
                exo_decrunch
                exo_decrunch_new
                bitbuster
                lazy
                nibrans
                LZRS_ASOBO
                lzrhys
                lze
                zx0
                zx1
                zx2
                zx5
                rzip
                melt1
                melt2

                  --------------------------------
                  --- recompression algorithms ---
                  --------------------------------
                zlib_compress
                deflate_compress
                lzo1_compress
                lzo1x_compress
                lzo2a_compress
                xmemlzx_compress
                bzip2_compress
                gzip_compress
                lzss_compress
                sfl_block_compress
                sfl_rle_compress
                sfl_nulls_compress
                sfl_bits_compress
                lzf_compress
                brieflz_compress
                jcalg_compress
                bcl_huf_compress
                bcl_lz_compress
                bcl_rice_compress
                bcl_rle_compress
                bcl_sf_compress
                szip_compress
                huffmanlib_compress
                lzma_compress
                lzma_86head_compress
                lzma_86dec_compress
                lzma_86dechead_compress
                lzma_efs_compress
                falcom_compress
                kzip_zlib_compress
                kzip_deflate_compress
                prs_compress
                rnc_compress
                lz4_compress
                sfl_block_chunked_compress
                snappy_compress
                zpaq_compress
                blosc_compress
                gipfeli_compress
                yappy_compress
                lzg_compress
                doboz_compress
                nitrosdk_compress
                hex_compress
                base64_compress
                lzma2_compress
                lzma2_86head_compress
                lzma2_86dec_compress
                lzma2_86dechead_compress
                lzma2_efs_compress
                lzma_0_compress
                lzma2_0_compress
                stormhuff_compress
                CT_HughesTransform_compress
                CT_LZ77_compress
                CT_ELSCoder_compress
                CT_RefPack_compress
                dk2_compress, also as ea_compress, refpack_compress
                qfs_compress, same as dk2_compress
                LZHUFXR_COMPRESS, aka STALKER_LZA_COMPRESS
                FSE_COMPRESS
                ZSTD_COMPRESS, only current algorithm, no legacy
                DS_BLZ_COMPRESS
                DS_HUF_COMPRESS
                DS_LZE_COMPRESS
                DS_LZS_COMPRESS
                DS_LZX_COMPRESS
                DS_RLE_COMPRESS
                HEATSHRINK_COMPRESS
                SMAZ_COMPRESS
                LZFX_COMPRESS, DARKSECTOR_NOCHUNKS_COMPRESS,
                    FASTLZAH_COMPRESS, EVOLUTION_COMPRESS,
                    UNKNOWN6_COMPRESS
                PITHY_COMPRESS
                ZLING_COMPRESS
                DENSITY_COMPRESS
                BSC_COMPRESS
                SHOCO_COMPRESS
                WFLZ_COMPRESS
                FASTARI_COMPRESS
                DICKY_COMPRESS
                SQUISH_COMPRESS
                LZHL_COMPRESS
                LZHAM_COMPRESS
                TRLE_COMPRESS
                SRLE_COMPRESS
                MRLE_COMPRESS
                CPK_COMPRESS, note that in reimport mode it may not
                    work because the decompression is size
                    dependent so the compressed size will probably
                    change but the value will not be modified in
                    the original archive (the reimport mode changes
                    only the data of the files).
                    if you want it to work you have to manually
                    edit the value from the archive, launch
                    quickbms -V and search something like:
                        . 00000eef get     value      0x0000e548 4
                        . 00000023 get     column_name "FileSize" -1
                    in that example you have to set the new value
                    at offset 0xeef, but note that the new
                    compressed size is not displayed by quickbms
                LZRW1KH_COMPRESS
                BPE_COMPRESS
                NRV2b_COMPRESS
                NRV2d_COMPRESS
                NRV2e_COMPRESS
                LZSS0_COMPRESS, aka PUYO_LZ01_COMPRESS
                CLZW_COMPRESS
                QUICKLZ_COMPRESS
                ZOPFLI_ZLIB_COMPRESS
                ZOPFLI_DEFLATE_COMPRESS
                PKWARE_DCL_COMPRESS
                LZ5_COMPRESS
                YALZ77_COMPRESS
                SYNLZ1_COMPRESS
                SYNLZ2_COMPRESS
                PPMZ2_COMPRESS
                EA_JDLZ_COMPRESS
                OODLE_COMPRESS
                LZFSE_COMPRESS
                M99CODER_COMPRESS
                LZ4X_COMPRESS
                YUKE_BPE_COMPRESS
                LZO1A_COMPRESS
                LZO1B_COMPRESS
                LZO1C_COMPRESS
                LZO1F_COMPRESS
                LZO1Y_COMPRESS
                LZO1Z_COMPRESS
                LIZARD_COMPRESS
                LIBLZS_COMPRESS
                dizzy_compress
                level5_compress
                brotli_compress
                DRV3_SRD_COMPRESS
                YAZ0_COMPRESS
                BIZARRE_COMPRESS
                BIZARRE_SKIP_COMPRESS
                BLACKDESERT_COMPRESS
                DR12AE_COMPRESS
                EA_COMP_COMPRESS
                LBALZSS_COMPRESS
                MOHLZSS_COMPRESS
                MOHRLE_COMPRESS
                NISLZS_COMPRESS
                QCMP1_COMPRESS
                RFPK_COMPRESS
                RLEW_COMPRESS
                SAINT_SEYA_COMPRESS
                SHREK_COMPRESS
                SLZ_01_COMPRESS
                SLZ_02_COMPRESS
                SLZ_03_COMPRESS
                UCLPACK_COMPRESS
                SEGA_LZS2_COMPRESS
                WOLF_COMPRESS
                YAKUZA_COMPRESS
                YKCMP_COMPRESS
                LZWAB_COMPRESS
                NCOMPRESS_COMPRESS
                UNCOMPRESS_COMPRESS
                LZ4X_NEW_COMPRESS
                DRAGONBALLZ_COMPRESS
                MPPC_COMPRESS
                MPPC_BIG_COMPRESS
                ZLIBX_COMPRESS, just same as zlib_compress
                DEFLATEX_COMPRESS, just same as deflate_compress
                LZ77WII_COMPRESS, use dictionary for cmd, default
                    is 0x40 of DS_LZX_COMPRESS
                APLIB_COMPRESS
                LZ4F_COMPRESS
                LZ5F_COMPRESS
                ppmdh_compress
                ppmdi_compress
                lunar_lz1_compress  to lunar_lz19_compress
                lunar_rle1_compress to lunar_rle4_compress
                coreonline_compress

                *note: the updated list is available in comtype.h

    DICT        an optional C string containing the bytes of the
                dictionary (cstring) or particular parameters
                depending by the chosen algorithm.
                Note that DICT can be:
                - a cstring
                  comtype algo "\x11\x22\x33" // static binary dict
                - a variable for which, often, you must specify
                  also a length
                  comtype algo DICT DICT_SIZE // variable dict


...................................................................

ReverseShort VAR [ENDIAN]

    Classical swap that inverts a 16bit variable from 0x2211 to
    0x1122
     and vice versa.

    Arguments:
      VAR       variable to flip
      ENDIAN    desired endianess like little or big


...................................................................

ReverseLong VAR [ENDIAN]

    Classical swap that inverts a 32bit variable from 0x44332211 to
    0x11223344 and vice versa.

    Arguments:
      VAR       variable to flip
      ENDIAN    desired endianess like little or big


...................................................................

ReverseLongLong VAR [ENDIAN]

    Classical swap that inverts a 32bit variable from
    0x8877665544332211 to 0x1122334455667788 and vice versa.
    This command works only with quickbms_4gb_files.exe

    Arguments:
      VAR       variable to flip
      ENDIAN    desired endianess like little or big


...................................................................

Endian TYPE [VAR]

    It changes the current global endianess of the read/written
    data, the default one is little endian.

    Arguments:
      TYPE      - little, intel
                  0x11223344 is stored as 44 33 22 11
                - big, network
                  0x11223344 is stored as 11 22 33 44
                - swap, change, invert
                  swap the current endianess, big <-> little
                - guess
                  followed by a 32bit number. The function swaps
                  the number and compares it with the original one
                  so if the number is 0x04000000 then the swapped
                  one will be 0x4 and the tool will change the
                  global endianess and the one of the variable
                - guess16
                  followed by a 16bit number
                - guess64
                  followed by a 64bit number
                - guess24
                  followed by a 24bit number
                - save, store
                  stores the current endian in VAR: 1=big, 0=little

    Examples:
      print "little endian"
      endian big
      print "big endian"
      endian little
      print "little endian"
      endian change
      print "little->big endian"
      endian guess 0x04000000
      print "guess endian"
      endian save CURRENT_ENDIAN
      if CURRENT_ENDIAN == 0
        print "little endian"
      else
        print "big endian"
      endif
      endian set CURRENT_ENDIAN


...................................................................

FileXOR SEQ [OFFSET] [FILENUM]

    Any read operation (Get, *Log and so on) on any file will
    also perform the XORing of the read data with the numbers
    contained in the given string or in the given variable.
    The OFFSET field by default is zero which means that if the
    data must be xored with more than one byte (a "xor key") the
    first byte of the key is the first byte at OFFSET which is 0
    by default (beginning of the file).
    Recap: the FileXOR command works with ANY file access.

    Arguments:
      SEQ       Sequence of space-separated 8bit numbers, like:
                - asequence of bytes separated by space like 0x12
                  or "0x12 0x34 0x56" or directly a C hex string
                  like "\x12\x34\x56" (NOT a C notation!)
                - a numeric variable like MYXORKEY
                - a string not starting with numbers, '\' or '-'
                Currently it's not possible to use a key in string
                mode and use the Encryption command for doing it,
                so if you have a string convert it to a numeric
                sequence first or be sure that it doesn't start
                with the chars shown above.
                Set it to 0 or "" for disabling the xor.
                Note that SEQ can be also a 32bit signed number
                like filexor 0x11223344 but the size is decided by
                value so 0x00000022 is 8 bit and not 32, while
                -0x20 is considered 8bit and 0x80112233 a 32bit.
      OFFSET    Needed only for the xor key offset.
                If the archive is xored with a xor key from its
                beginning (so first byte of the archive xored with
                the first one of the key) this argument is usually
                not necessary
                Instead if only the file to extract is xored, this
                argument must have the same offset of the file (so
                just reuse the same OFFSET used in Log)
      FILENUM   By default FileXOR is applied to ALL the files and
                OFFSET (if specified) is referred to file 0.
                When FILENUM is specified, it will only be applied
                to that specific file.

    Examples:
      filexor 0xff
      filexor -0x20
      filexor 0x1122    # 32bit
      filexor -0x1122   # 32bit
      filexor "0x12 0x34 123 255"
      filexor MYXORBYTE
      saepos OFFSET
      filexor "0x12 0x34 123 255" OFFSET
      filexor "\x12\x34\x7b\xff"
      Log NAME OFFSET SIZE


...................................................................

FileRot SEQ [OFFSET] [FILENUM]

    Exactly as for FileXOR but it performs a sum operation.
    For example if SEQ is 0x01 and the file contains "hello" it
    will become "ifmmp" while if SEQ is -1 or 0xff it will become
    "gdkkn".
    -1 and 0xff are the same because it's a 8 bit number while
    0x100 or -0x100 are considered 32bit.
    Recap: the FileRot command works with ANY file access

    Watch the previous arguments and examples.


...................................................................

FileCrypt SEQ [OFFSET] [FILENUM]

    Experimental, it works only if has been already specified and
    enabled the Encryption command and basically applies those
    algorithms to the normal file reading operations.
    Note that at the moment OFFSET is unused and SEQ can be only 1
    for activating it and "" to disable it ("" and NOT 0!).
    Remember that the encryption algorithms usually work on blocks
    of data so this command is probably useless.

    Full example:
      get NAMESZ long
      encryption xor "\x11\x22\x33\x44"
      filecrypt 1
      getdstring NAME NAMESZ
      filecrypt ""
      encryption "" ""


...................................................................

Strlen VAR VAR [SIZE]

    It calculates the length of the second variable (as string) and
    stores it in the first one.
    The length is the amount of consecutive non-zero bytes, so it
    doesn't work with unicode strings, maybe only if SIZE is set.
    Note that for practical reasons this command can be emulated
    also using "set VAR strlen VAR".

    arguments
      VAR       Destination variable which will contain the length
      VAR       Variable of which calculating the length
      SIZE      set it to 1 to receive the size of the variable
                instead of its NUL delimited lenght, may be useful
                in some situations where you need to ignore the
                0x00 bytes

    examples
      strlen NAME_LENGTH NAME
      strlen NAMESZ NAME
      strlen RAW_NAMESZ NAME 1


...................................................................

GetVarChr VAR VAR OFFSET [TYPE]

    A particular and sometimes very useful command which works
    exactly like accessing an array of elements contained in the
    second variable, for example a string or a memory file.
    It can be compared to C as: var1 = var2[offset];
    This simple and effective method allows the manipulation of
    strings and variables for creating custom headers (like a DDS)
    and moreover for performing operations on a piece of the
    memory, like a custom encryption algorithm.
    Some real examples are my Deer Hunter 2004/2005 scripts.

    Arguments:
      VAR       Destination variable that will contain the element
      VAR       Variable or memory file from which you want to get
                the element
      OFFSET    Position of the element in the second variable
      TYPE      Type of the element to read and assign to the first
                variable, if not specified it's a BYTE (8bit).
                You can specify most of the available datatypes
                like short, long, longlong and so on

    Examples:
      For i = 0 < SIZE
          GetVarChr TMP MEMORY_FILE i
          GetVarChr TMP MEMORY_FILE i long
          # GetVarChr TMP MEMORY_FILE i string
      Next i


...................................................................

PutVarChr VAR OFFSET VAR [TYPE]

    The "write-mode" alternative of the previous command which
    allows to perform various complex operations with custom
    algorithms (like in my Deer Hunter 2004/2005 scripts).
    It can be compared to C as: var1[offset] = var2;
    Note that PutVarChr can be also used as an allocator of memory
    that is often useful in the implementation of custom
    decompression algorithms or, moreover, for pre-allocating a
    MEMORY_FILE for storing chunks. This is useful to avoid wasting
    time and memory with the incremental allocation, remember only
    to use the command "Log MEMORY_FILE 0 0" after it for resetting
    the position of the MEMORY_FILE.

    arguments
      VAR       Variable or memory file in which you want to put
                the element
      OFFSET    Position of the output where placing the element,
                it can also be negative in which case it will work
                from the end of the variable (may not work in some
                conditions)
      VAR       Source variable which will contain the element to
                write, it's also possible to store the address of
                the variable which may be useful with external DLLs
      TYPE      Type of the element to read and assign to the first
                variable, if not specified it's a BYTE (8bit).
                You can specify most of the available datatypes
                like short, long, longlong and so on.

    Examples:
      For i = 0 < SIZE
          GetVarChr TMP MEMORY_FILE i
          Math TMP ^= 0xff
          PutVarChr MEMORY_FILE i TMP
      Next i


...................................................................

Debug [MODE]

    Switch command that enables the -v option in real-time for a
    specific portion of the script, used only for debugging.

    If MODE is specified and it's a positive number, QuickBMS will
    only display the content of the variables read/written with the
    Get/Put commands. This is very useful and cool for debugging
    file formats and protocols in an easy way just like -V.

    If MODE is negative, it will disable the verbose mode.

    Examples:
      debug     # like -v
      debug 0   # like -v
      debug 1   # like -V
      debug -1  # disable -v/V


...................................................................

Padding VAR [FILENUM] [BASE_OFF]

    When called it performs an automatic GoTo to the next position
    of the file skipping the aligned data.
    Imagine to have a file where it's used an alignment of 4 bytes
    and your current file offset is 0x39, if you use Padding 4 the
    offset will be automatically changed to 0x3c.
    By default the padding is referred to the beginning of the file
    (offset 0).

    Arguments:
      VAR       Size of the alignment, like 4 or 16 and so on
      FILENUM   Number of the file associated to the archive (0)
      BASE_OFF  base offset from where calculating the padding (0)

    Examples:
      Get NAME string
      Padding 4
      get OFFSET long


...................................................................

Append [DIRECTION]

    Command to enable the append mode in the *Log commands, so if
    the output filename already exists it will not be overwritten,
    the new content is concatenated (appended) to the existent one.
    Note that with real files (not memory files) the user may be
    prompted before writing the output file if it already existed
    before the running of the script.
    Note that the reimport mode may not work correctly when you use
    a combo of MEMORY_FILE and Append, so the direct and more
    simple Log to file + Append is suggested.
    Note that from QuickBMS 0.11 the Append command also affects
    the Put* commands (Put/PutDString/PutCT).

    Arguments:
      DIRECTION This is a new optional argument that allows to
                specify where placing the new content:
                -1  prefix:
                    a negative value means that the new content
                    will be placed before the current file, so the
                    old content will be appended to the new one
                0   append:
                    the new content will be appended to the current
                    one (default, just like without DIRECTION)
                1   overwrite:
                    the new content will overwrite the current one
                    without changing the file size if the new one
                    is smaller, use goto to set the offset where
                    placing the new content.
                2   insert:
                    the new content will be inserted in the current
                    position (size = position + data + remaining)

    Examples:
      append
      Log "dump.dat" 0 0x10
      Log "dump.dat" 0x10 0x100

    The following is a particular example for allocating a
    MEMORY_FILE and using it instead of TEMPORARY_FILE saving space
    on the disk and performances:

      math TMP = CHUNKS
      math TMP *= 0x8000
      log MEMORY_FILE 0 0
      putvarchr MEMORY_FILE TMP 0   # pre-allocation for speed
      log MEMORY_FILE 0 0           # reset the position and size
      append
      for i = 0 < CHUNKS
          ...
          clog MEMORY_FILE OFFSET ZSIZE 0x8000
      next i
      append
      get SIZE asize MEMORY_FILE


...................................................................

Encryption ALGO KEY [IVEC] [MODE] [KEYLEN]

    One of the most interesting commands which allows to set a
    decryption algorithm used for the Log and CLog command.
    QuickBMS supports also the hashing algorithms of OpenSSL, the
    binary hash will be placed in the variable QUICKBMS_HASH while
    the hexadecimal hash in QUICKBMS_HEXHASH (capital letters) and
    QUICKBMS_HEXHASHL (low).
    Note that the hashing algorithms don't need a key, but you can
    use that field for performing a direct hash operation on the
    provided key without using the log command, eg: encryption sha1
    "mystring".
    You can also specify the size in case it's a binary variable,
    eg: encryption md5 "mystring" "" 0 8
    For the HMAC hash algorithm you must use the IVEC field, anyway
    remember that this feature is just optional.
    Regarding the OpenSSL algorithms, it's possible to enable the
    "Final" mode by using one of the following prefixes:
    CipherFinal, DecryptFinal or EncryptFinal (it's a way used by
    OpenSSL to get the original size from block-cipher data).

    Arguments:
      ALGO      aes, Rijndael
                blowfish, you should try also bf_ecb if the result
                  is not the expected one
                des
                3des-112
                3des-168
                rc4
                tea, use IVEC to specify custom delta, sum, endian
                  (0/1), cycles and if_invert_delta_operation
                xtea, use IVEC to specify custom delta, endian
                  (0/1), cycles and if_invert_delta_operation
                xxtea, use IVEC to specify custom delta, endian
                  (0/1), cycles and if_invert_delta_operation
                idea
                swap, use the bytes to swap as key, it works just
                  like reverseshort and reverselong:
                  encryption swap 2: 2211 -> 1122
                reverseshort, swap 2
                reverselong, swap 4
                math, exactly like the bms command plus the size of
                  the numbers:
                    encryption math "^u= 0x11223344 1" 32
                    encryption math "n #INPUT#" # decrypt = -encrypt
                  it means that this encryption can do tons of
                  operations including xor, rot, rotate and so on.
                  the "1" after the math operation means if we want
                  to respect the exact size of each element like a
                  sort of AND SIZE (default ignore).
                  ivec is the size of each element (8bits default)
                xmath, key is the operation to perform for each
                  element, ivec is the size of each element (8 bits
                  default)
                  use #INPUT# to identify the element in the data:
                    encryption xmath "((#INPUT# + 1) << 2) + #INPUT#" 8
                random, pseudo random incrementer (Linear
                  congruential generator) xored with the input key
                  contains a number corresponding to the algorithms
                  listed on
                  http://en.wikipedia.org/wiki/Linear%5Fcongruential%5Fgenerator#Parameters_in_common_use
                  (0 is the first one) plus other algorithms like
                  mersenne and so on.
                  the second parameter in the key is the seed.
                  the third one is the mask of bits of the key to
                  use for the operation.
                  ivec is the size of each element (8bits default).
                    encryption random "0 0x12345678"
                    encryption random "0 0x12345678" 32         # 32bit values
                    encryption random "0 0x12345678 0x7fff0000" # value ^ ((key >> 16) & 0x7fff)
                  you must check the src\myenc.c source code to
                  have the full list, currently over 17 implemented
                xor
                rot
                rotate, an 8/16/32/64bit ror or any other bit as
                  key, element size as ivec
                reverse, flip the file from the end to beginning
                flip, flip the bits of input file, reverse flip 8
                incremental,
                    # 8bit xor incremented by 1 each time
                  encryption "incremental xor" 0 0x01
                    # 32bit rot starting from 0x100 incremented by
                    # 0x11223344 each time, if the initial value
                    # is <= 0xffff it's a 16bit, <= 0xff is 8bit,
                    # the increment is checked too for guessing
                  encryption "incremental rot"   0x12345 0x11223344
                    # forced 32bit
                  encryption "incremental rot32" 0x12345 0x11223344
                charset, the substitution algorithm which uses a
                  charset of 256 chars
                charset2, as above but substitution is inverted
                twofish
                cast5
                seed
                serpent
                ice
                icecrypt, ICE algorithm with key implemented as in
                  the homonym program, the difference with "ice" is
                  ONLY in the key
                rotor, ivec contains the number of rotors (6 by
                  default, it was 12 till version 0.10.0)
                ssc, Leverage SSC
                wincrypt, aka cryptdecrypt or cryptencrypt
                  use the ivec field to specify:
                  (only those you need, not all are necessary):
                  - the hashing algorithm - CryptCreateHash, you
                    can find the key here
                  - the encryption algorithm - CryptDeriveKey
                  - the provider type - CryptAcquireContext
                  - Microsoft provider name, like MS_DEF_DH_SCHANNEL_PROV
                  - CryptDeriveKey flags, like CRYPT_CREATE_SALT
                  - CryptDecrypt flags, like CRYPT_OAEP
                  example:
                    encryption CryptDecrypt "mykey" "CALG_MD5 CALG_RC4 PROV_RSA_FULL"
                    encryption CryptDecrypt "1111" "CALG_MD5 CALG_RC4 PROV_RSA_FULL CRYPT_CREATE_SALT CRYPT_OAEP"
                cryptunprotect, key is used to specify the entropy
                  so the default is ""
                zipcrypto, the first 12 bytes are the encryption
                  header set the ivec to 1 for automatically
                  cutting the first 12 bytes
                md_null, from OpenSSL (does nothing)
                md2, from OpenSSL (not available)
                md4, from OpenSSL
                md5, from OpenSSL
                sha, from OpenSSL
                sha1, from OpenSSL
                dss, from OpenSSL
                dss1, from OpenSSL
                ecdsa, from OpenSSL
                sha224, from OpenSSL
                sha256, from OpenSSL
                sha384, from OpenSSL
                sha512, from OpenSSL
                mdc2, from OpenSSL
                ripemd160, from OpenSSL
                whirlpool, from OpenSSL
                hmac ..., hmac plus an OpenSSL hash algorithm, it's
                  an encrypted hash so you must provide a key.
                  example for a hmac sha1 on the fly:
                    encryption "hmac sha1" "mykey" "mydata"
                  or
                    encryption "hmac sha1" "mykey"
                    log MEMORY_FILE 0 SIZE
                    print "%QUICKBMS_HEXHASH%"
                enc_null, from OpenSSL (does nothing)
                des_ecb, from OpenSSL
                des_ede, from OpenSSL
                des_ede3, from OpenSSL
                des_ede_ecb, from OpenSSL
                des_ede3_ecb, from OpenSSL
                des_cfb64, from OpenSSL
                des_cfb1, from OpenSSL
                des_cfb8, from OpenSSL
                des_ede_cfb64, from OpenSSL
                des_ede_cfb1, from OpenSSL
                des_ede_cfb8, from OpenSSL
                des_ede3_cfb64, from OpenSSL
                des_ede3_cfb1, from OpenSSL
                des_ede3_cfb8, from OpenSSL
                des_ofb, from OpenSSL
                des_ede_ofb, from OpenSSL
                des_ede3_ofb, from OpenSSL
                des_cbc, from OpenSSL
                des_ede_cbc, from OpenSSL
                des_ede3_cbc, from OpenSSL
                desx_cbc, from OpenSSL
                dev_crypto_des_ede3_cbc, from OpenSSL
                dev_crypto_rc4, from OpenSSL
                dev_crypto_md5, from OpenSSL
                rc4, from OpenSSL
                rc4_40, from OpenSSL
                idea_ecb, from OpenSSL
                idea_cfb64, from OpenSSL
                idea_ofb, from OpenSSL
                idea_cbc, from OpenSSL
                rc2_ecb, from OpenSSL
                rc2_cbc, from OpenSSL
                rc2_40_cbc, from OpenSSL
                rc2_64_cbc, from OpenSSL
                rc2_cfb64, from OpenSSL
                rc2_ofb, from OpenSSL
                bf_ecb, from OpenSSL (bf stands for blowfish)
                  the result is different than the "blowfish" type
                  because the other uses big endian, try both
                bf_cbc, from OpenSSL
                bf_cfb64, from OpenSSL
                bf_ofb, from OpenSSL
                cast5_ecb, from OpenSSL
                cast5_cbc, from OpenSSL
                cast5_cfb64, from OpenSSL
                cast5_ofb, from OpenSSL
                rc5_32_12_16_cbc, from OpenSSL (not available)
                rc5_32_12_16_ecb, from OpenSSL (not available)
                rc5_32_12_16_cfb64, from OpenSSL (not available)
                rc5_32_12_16_ofb, from OpenSSL (not available)
                aes_128_ecb, from OpenSSL
                aes_128_cbc, from OpenSSL
                aes_128_cfb1, from OpenSSL
                aes_128_cfb8, from OpenSSL
                aes_128_cfb128, from OpenSSL
                aes_128_ofb, from OpenSSL
                aes_128_ctr, from OpenSSL
                aes_192_ecb, from OpenSSL
                aes_192_cbc, from OpenSSL
                aes_192_cfb1, from OpenSSL
                aes_192_cfb8, from OpenSSL
                aes_192_cfb128, from OpenSSL
                aes_192_ofb, from OpenSSL
                aes_192_ctr, from OpenSSL
                aes_256_ecb, from OpenSSL
                aes_256_cbc, from OpenSSL
                aes_256_cfb1, from OpenSSL
                aes_256_cfb8, from OpenSSL
                aes_256_cfb128, from OpenSSL
                aes_256_ofb, from OpenSSL
                aes_256_ctr, from OpenSSL
                camellia_128_ecb, from OpenSSL
                camellia_128_cbc, from OpenSSL
                camellia_128_cfb1, from OpenSSL
                camellia_128_cfb8, from OpenSSL
                camellia_128_cfb128, from OpenSSL
                camellia_128_ofb, from OpenSSL
                camellia_192_ecb, from OpenSSL
                camellia_192_cbc, from OpenSSL
                camellia_192_cfb1, from OpenSSL
                camellia_192_cfb8, from OpenSSL
                camellia_192_cfb128, from OpenSSL
                camellia_192_ofb, from OpenSSL
                camellia_256_ecb, from OpenSSL
                camellia_256_cbc, from OpenSSL
                camellia_256_cfb1, from OpenSSL
                camellia_256_cfb8, from OpenSSL
                camellia_256_cfb128, from OpenSSL
                camellia_256_ofb, from OpenSSL
                seed_ecb, from OpenSSL
                seed_cbc, from OpenSSL
                seed_cfb128, from OpenSSL
                seed_ofb, from OpenSSL
                mcrypt blowfish
                mcrypt des
                mcrypt tripledes
                mcrypt threeway
                mcrypt gost
                mcrypt safer-sk64
                mcrypt safer-sk128
                mcrypt cast-128
                mcrypt xtea
                mcrypt rc2
                mcrypt twofish
                mcrypt cast-256
                mcrypt saferplus
                mcrypt loki97
                mcrypt serpent
                mcrypt rijndael-128
                mcrypt rijndael-192
                mcrypt rijndael-256
                mcrypt enigma
                mcrypt arcfour
                mcrypt wake
                  note that for the algorithms supported by mcrypt
                  you can force their loading by preceeding ALGO
                  with "mcrypt" like "mcrypt_enigma" and you can
                  decide also their mode like "mcrypt_enigma_ecb"
                  or "mcrypt_enigma_cbc", list:
                  cbc, ecb, cfb, ofb and nofb
                3way
                skipjack
                anubis
                aria
                crypton
                frog
                gost
                lucifer
                mars
                misty1
                noekeon
                seal
                safer
                kirk, used in PSP eboot encryption, use the ivec to
                  specify the keys/encryption (default is 1, refer
                  to libkirk for more information)
                pc1, automatic 128/256 bit selection on key length
                blake224
                blake256
                blake384
                blake512
                bmw224
                bmw256
                bmw384
                bmw512
                cubehash224
                cubehash256
                cubehash384
                cubehash512
                echo224
                echo256
                echo384
                echo512
                fugue224
                fugue256
                fugue384
                fugue512
                groestl224
                groestl256
                groestl384
                groestl512
                hamsi224
                hamsi256
                hamsi384
                hamsi512
                haval128_3
                haval128_4
                haval128_5
                haval160_3
                haval160_4
                haval160_5
                haval192_3
                haval192_4
                haval192_5
                haval224_3
                haval224_4
                haval224_5
                haval256_3
                haval256_4
                haval256_5
                jh224
                jh256
                jh384
                jh512
                keccak224
                keccak256
                keccak384
                keccak512
                luffa224
                luffa256
                luffa384
                luffa512
                md2
                md4
                md5
                panama
                radiogatun32
                radiogatun64
                ripemd
                ripemd128
                ripemd160
                sha0
                sha1
                sha224
                sha256
                sha384
                sha512
                shabal192
                shabal224
                shabal256
                shabal384
                shabal512
                shavite224
                shavite256
                shavite384
                shavite512
                simd224
                simd256
                simd384
                simd512
                skein224
                skein256
                skein384
                skein512
                tiger
                tiger2
                whirlpool
                whirlpool0
                whirlpool1
                sph
                mpq
                rc6
                xor_prev    < data[i] ^= data[i - 1]    use key + or - to use operations
                                                        different than xor and the value to
                                                        use for the last byte, "^ 0x8e"
                xor_prev2   < data[i] ^= data[i + 1]    "
                xor_next    > data[i] ^= data[i - 1]    "
                xor_next2   > data[i] ^= data[i + 1]    "
                PKCS5_PBKDF2_HMAC, example PKCS5_PBKDF2_HMAC_sha1
                Rfc2898DeriveBytes
                BytesToKey, example "BytesToKey_sha1 aes"
                ZIP_AES followed by 128, 192 or 256 (gladman cwc)
                rsa
                rsa_tomcrypt
                modpow, just a simple RSA BN_mod_exp performed on
                  chunks of 256 bytes
                modpow_zed
                abc
                achterbahn
                achterbahn128
                cryptmt
                dicing
                dragon
                edon80
                ffcsr8
                fubuki
                grain
                grain128
                hc128
                hc256
                hermes128
                hermes80
                lex
                mag
                mickey
                mickey128
                mir1
                mosquito
                moustique
                nls
                polarbear
                pomaranch
                py
                rabbit
                salsa20
                sfinks
                sosemanuk
                sss
                trivium
                tsc3
                tsc4
                wg
                yamb
                aes_ige
                aes_bi_ige
                aes_heat, used in the game Heat Online
                isaac
                isaac_vernam
                isaac_caesar
                hsel
                rng, just random data, useful with filecrypt for
                  generating (read/write) random fields, currently
                  key is ignored so use ""
                bcrypt, it supports various options like:
                  encryption "bcrypt aes cbc block_padding" KEY IVEC
                molebox
                replace, replace the bytes of KEY with IVEC
                  (currently must be smaller or equal)
                rc4_nokey, the specified key will be used directly
                  as the 256-bytes context
                d3des
                spookyhash, 32/64/128
                murmurhash, qhashfnv1_32, qhashfnv1_64,
                  qhashmurmur3_32, qhashmurmur3_128
                xxhash, XXH32, XXH3_64bits_withSecret,
                  XXH3_128bits_withSecret, XXH3_128bits_withSecret,
                  XXH128, XXH3_64bits_withSecret, XXH64
                tomcrypt
                  modes: ecb, cfb, ofb, cbc, ctr, lrw, f8, xts,
                    hmac, omac, pmac, eax, ocb3, ocb, ccm, gcm,
                    pelican, xcbc, f9, poly1305, chacha20poly1305,
                    blake2smac, blake2bmac
                  encryptions: blowfish, rc5, rc6, rc2, saferp,
                    safer_k64, safer_k128, safer_sk64, safer_sk128,
                    rijndael, aes, rijndael_enc, aes_enc, xtea,
                    twofish, des, des3, cast5, noekeon, skipjack,
                    khazad, anubis, kseed, kasumi, camellia
                  hashing: multi2, chc, whirlpool, sha512,
                    sha512-256, sha512-224, sha384, sha256, sha224,
                    sha1, md5, md4, md2, tiger, rmd128, rmd160,
                    rmd256, rmd320
                  example:
                    Encryption "tomcrypt rijndael ecb" "0123456789abcdef"
                PBKDF1
                PBKDF1_openssl
                PBKDF2
                crc, a complete and powerful checksum function that
                  can be fully configured:
                  - key is the polynomial ("" for crc32 0x77073096)
                  - ivec contains:
                    - size of the crc (8/16/32/64)
                    - initial value (like -1)
                    - final xor value (-1, the complement)
                    - type, many types listed in crc_calc in crc.c,
                      it also includes qhashmurmur3_32, qhashfnv1_32,
                      qhashfnv1_64, jenkins_one_at_a_time_hash,
                      XXH32, XXH64, xPear16, CityHash32, CityHash64,
                      CityHash64WithSeed, StormHash,
                      jenkins_hashlittle, adler32, fnv32, UHash,
                      spookyhash_32, spookyhash_64
                    - reverse/reflect mode for generating the table
                      (0 or 1)
                    - bitmask_side (0 or 1, latter is most used)
                  default values: 0xedb88320 32 -1 -1 0 0 1
                  if you need the classical crc16 (0xc0c1) use:
                    encryption crc 0xa001 "16 0 0 0 0 1"
                  or
                    encryption crc "" 16
                  the result is placed in the variable QUICKBMS_CRC
                  example for type 39:
                    encryption crc 0 "0 0 0 39 0 1"
                  for additional info:
                    http://aluigi.org/bms/quickbms_crc_engine.txt
                  for technical information about the operations
                  check the crc_calc function in crc.c, it's quite
                  easy to understand because it contains the simple
                  operations performed in each cycle, copy below:
                    0   table[(BYTE ^ CRC) & 0xff] ^ (CRC >> 8)
                    1   table[(BYTE ^ (CRC >> (bits - 8))) & 0xff] ^ (CRC << 8)
                    2   ((CRC << 8) | BYTE) ^ table[(CRC >> (bits - 8)) & 0xff]
                    3   ((CRC >> 1) + ((CRC & 1) << (bits - 1))) + BYTE
                    4   crc_in_cksum(CRC)
                    5   CRC ^ BYTE
                    6   CRC + BYTE    // lose lose
                    7   table[(BYTE ^ CRC) & 0xff] ^ CRC
                    8   table[(BYTE ^ CRC) & 0xff] ^ (CRC >> (bits - 8))
                    9   (CRC << 1)  ^ BYTE
                    10  (CRC << 1)  + BYTE
                    11  rol(CRC, 1, 0) ^ BYTE
                    12  rol(CRC, 1, 0) + BYTE
                    13  ror(CRC, 1, 0) ^ BYTE
                    14  ror(CRC, 1, 0) + BYTE
                    15  (CRC << 5) + CRC + BYT) // djb2 5381
                    16  (CRC * poly) + BYTE // djb2 and sdbm
                    17  (CRC * poly) ^ BYTE // djb2 and FNV-1
                    18  (CRC ^ BYTE) * poly) // FNV-1a
                    19  BYTE + (CRC << 6) + (CRC << 16) - CRC // sdbm 65599
                    20  poly * (CRC + BYTE * (i + 1))
                    21  qhashmurmur3_32
                    22  qhashfnv1_32
                    23  qhashfnv1_64
                    24  XXH32(poly)
                    25  XXH64(, poly)
                    26  jenkins_one_at_a_time_hash
                    27  xPear16
                    28  CityHash32
                    29  CityHash64
                    30  CityHash64WithSeed(poly)
                    31  StormHash(MPQ_HASH_TABLE_INDEX)
                    32  StormHash(MPQ_HASH_NAME_A)
                    33  StormHash(MPQ_HASH_NAME_B)
                    34  StormHash(MPQ_HASH_FILE_KEY)
                    35  StormHash(MPQ_HASH_KEY2_MIX)
                    36  jenkins_hashlittle(poly)
                    37  adler32
                    38  fnv32(crc ? crc : 0x811c9dc5)
                    39  UHash(crc, poly, 0x7fffffff)
                    40  spookyhash_32(poly)
                    41  spookyhash_64(poly)
                    42  XXH3_64bits
                    43  XXH3_64bits_withSeed(poly)
                  note that some crc types use the polynomial value
                  as constant in each cycle
                  crc64 and 64bit crc are only supported by
                  quickbms_4gb_files.exe
                  if you are a dev and need the generated table,
                  use the following command with quickbms -V or -v:
                    print "%QUICKBMS_CRC_TABLE%"
                EXECUTE:
                  use KEY to specify the command to execute with
                  #INPUT# instead of input filename and #OUTPUT#
                  for the output one, you can also specify a
                  variable by using the %VAR% notation.
                  IMPORTANT NOTE: do NOT use "encryption execute"
                  if the output will be bigger than the input, use
                  Clog in that case!
                  example:
                    encryption EXECUTE "mycrypt.exe d #INPUT# #OUTPUT#"
                  another full example:
                    get SIZE asize
                    encryption EXECUTE "lame.exe -V 4 #INPUT# #OUTPUT#"
                    log "newfile.mp3" 0 SIZE
                    encryption EXECUTE "otherprog.exe #INPUT# #OUTPUT# %SIZE%"
                    log "newfile2.mp3" 0 SIZE
                CALLDLL:
                  exactly as above except that the variables don't
                  need to be specified within '%' because calldll
                  already handles them, but don't worry because even
                  if you do that the result should not change:
                    encryption calldll "test.dll myfunction cdecl RET #INPUT# #INPUT_SIZE# MYVAR"
                    get SIZE asize
                    log "newfile.mp3" 0 SIZE
                  full support also for pointers using the '&' or
                  '*' prefix like &MEMORY_FILE, &VAR, &#INPUT#,
                  &INPUT_SIZE
                "" "", disable the encryption
      KEY       The key to use with escapes like "\x11\x22\x33\x44"
                or "this is my key" (cstring)
                This value can be also a variable or a memory file
                set ALGO and KEY to "" for disabling the encryption
      IVEC      The ivec to use in C notation (cstring), an ivec is
                an additional key used for increasing the security
                of encryption algorithms that are usually defined
                as ECB without ivec and CBC (and others) with ivec
      MODE      0 for decryption (default), 1 for forcing the
                encryption mode, if no ivec is used remember to
                place a "" at its place
      KEYLEN    Forces the usage of a certain length of the key,
                this one has been introduced only for avoiding the
                problem of using a variable as KEY containing
                zeroes in it and for the non-block ciphers when you
                use KEY as a variable in which a certain length is
                used and not strlen. KEYLEN is also necessary with
                some algorithms when you set the key as a "string"
                variable (Set KEY string "blah"), that's because
                QuickBMS will pass a different larger length to the
                algorithm

    Examples:
      Encryption aes "0123456789abcdef" "" 1 # encrypt without ivec
      Log MEMORY_FILE 0 SIZE
      Encryption aes "0123456789abcdef"      # decrypt without ivec
      Log "redecrypted_file.dat" 0 SIZE MEMORY_FILE
      Encryption aes "\x12\x34\x56\x78"
      set MEMORY_FILE binary "\x12\x34\x56\x78"
      Encryption aes MEMORY_FILE
      Encryption aes MY_VARIABLE
      Encryption md5 ""


...................................................................

Print MESSAGE

    It prints a string in C notation with the values of the
    variables if their names are specified between '%' chars.
    It's also possible to specify the maximum amount of bytes to
    visualize (or a variable containing such value) and if they
    must be displayed in hex or dump mode specifying some flags
    after a '|' like
    in the examples:
    - x/h/hex: hexadecimal numbers and chars
    - dump:    hexadecimal dump, left in hex and right in chars
    - number:  amount of bytes to show
    - var:     variable containing the amount of bytes to show

    Arguments:
      MESSAGE   C notation string, each %VAR% word is converted to
                its value (cstring)
                From version 0.11 it also supports combinations of
                colors using the {FB} notation for Foreground and
                Background color using ANSI notation (it also
                supports the full name):
                  0: Black          8: Bright Black
                  1: Red            9: Bright Red
                  2: Green          a: Bright Green
                  3: Yellow         b: Bright Yellow
                  4: Blue           c: Bright Blue
                  5: Magenta        d: Bright Magenta
                  6: Cyan           e: Bright Cyan
                  7: White          f: Bright White

    Examples:
      print "the variable OFFSET of the file %FILENAME% has the value %OFFSET|x%"
      print "this is the first line\nthis is the second line\n"
      print "variable %VAR% and %VAR2%"
      print "variable %VAR|h% and %VAR2|hex%"
      print "variable %VAR|3% and %VAR2|4%"
      print "variable %VAR|3h% and %VAR2|h4%"
      print "variable %VAR|dump16%"
      print "variable %VAR|dumpVARSZ%"
      print "\x68\x65\x6c\x6c\x6f"
      print "Hello, {1}how are you?{}\n{f}Fine!{} ... {0f}also this {green}closing{/green} works"


...................................................................

GetArray VAR ARRAY VAR_IDX
...
PutArray ARRAY VAR_IDX VAR

    Commands to store variables in bidimensional arrays.
    They work on a dynamic array where it's possible to store the
    variables. Something like a temporary place or a stack.
    It's highly suggested to pre-allocate the array if you know
    the max value, example: PutArray 0 FILES 0
    If the array index (VAR_IDX) is negative like -1:
    - getarray will take the element located at that position from
      the end of the array, so "getarray VAR 0 -1" will take the
      last element while "getarray VAR 0 -2" will take the one
      before
    - putarray will ever append the element at the end of the
      array, currently there is no difference if you use VAR_IDX
      -1, -2, -1000
    - getarray will return the number of elements in the array if
      the negative amount is bigger than the elements... a sort of
      work-around like: getarray ELEMENTS 0 -0x80000000

    Examples:
      PutArray 0 0 FIRST_VAR
      PutArray 0 1 SECOND_VAR
      GetArray FIRST_VAR 0 0
      GetArray SECOND_VAR 0 1
      getarray FILES 0 -0x80000000
      for i = 0 < FILES
        putarray 0 -1 VAR
      next i

    Note: since quickbms 0.11 both getarray and putarray also
      support multiple variables, for example:
        putarray 0 i NAME OFFSET SIZE
        getarray NAME OFFSET SIZE 0 i
      it's the same of:
        putarray 0 i NAME
        putarray 1 i OFFSET
        putarray 2 i SIZE
        getarray NAME   0 i
        getarray OFFSET 1 i
        getarray SIZE   2 i


...................................................................

SortArray ARRAY [ALL]

    Experimental sorting of the arrays in ascending order (like 0
    to 99) based on the values in ARRAY.
    If ALL is a number different than zero, the sorting will affect
    ALL the available arrays created till that moment, which means
    that their positions will match those of the sorted array, so
    if the array 0 was "1" "3" "2" and array 1 was "hello" "test"
    "bye", with ALL set to 1 you will have 1 2 3 and "hello" "bye"
    "test".
    From QuickBMS 0.7.7 the sorting is unsigned, so -1 is handled
    as 0xffffffff, the highest element of the array.

    Examples:
      putarray 0 0 "zzz"
      putarray 0 1 "aaa"
      putarray 0 2 "bbb"
      sortarray 0
      for i = 0 < 3
        getarray TMP 0 i
        print "%TMP%"
      next i


...................................................................

SearchArray VAR ARRAY VAR

    Experimental search in arrays.

    Examples:
      searcharray IDX 0 "search value"
      searcharray IDX 0 0x11223344


...................................................................

CallFunction NAME [KEEP_VAR] [ARG1] [ARG2] ... [ARGn]
StartFunction NAME
...
EndFunction

    Calling and declaration of a function identified by NAME where
    the values of the variables are backed up till the termination
    of the function when they are restored.
    It works very well for recursive/nested archives like those
    used by "The Void" and "Another Day".
    If KEEP_VAR is not specified or zero, QuickBMS will make a
    backup of the current values and the function will work on a
    copy, when the function terminates the variables will be
    restored to their backup.
    If KEEP_VAR is set to 1, there will be no backup and any change
    made in the function will remain when it terminates.
    Do not use KEEP_VAR if you are working on a nested/recursive
    file table, use it to 1 if you are creating a macro or a
    function called many times to perform a task.
    It's a good idea to place all the functions (from StartFunction
    till EndFunction) at the end of the scripts.
    It's also possible to pass optional arguments to the function,
    they will have the name of the function plus ARGnumber, eg:
      MYFUNCTION_ARG1 and MYFUNCTION_ARG2.
    Doesn't exist a return value at the moment but it's possible to
    do it in other ways like saving the value in an array or on a
    MEMORY_FILE.

    Arguments:
      NAME      Name assigned to the function
      KEEP_VAR  Set to 1 if you want to keep the content of the
                variables without resetting them, in short words:
                0 = for recursive functions (default)
                1 = for normal functions that change variables,
                    this is faster and suggested in most cases
      ARGs      Arguments, they are seen inside the function as
                "name of the function" + ARG + argument_number

    Examples:
      http://aluigi.org/bms/thevoid.bms
      http://aluigi.org/bms/fear.bms


...................................................................

ScanDir PATH NAME SIZE [FILTER]

    Function without a real usage, it simply scans the PATH folder
    and fills the NAME and SIZE variables with the name and the
    size of each file found.
    Currently this function doesn't have a purpose so ignore it.
    If you want to filter the scanned files located in the folder
    you specified as input, use the -F option of quickbms (I tell
    this information because some users may think to "wrongly" use
    this command for that purpose).

    Arguments:
      PATH      Must be ".", the current folder and the returned
                filenames will start with ./
                From quickbms 0.9.1 you can just use ""
                There are also some "experimental" values not meant
                for normal usage, with optional file number after
                "://" (heap://10 will work on the file number 10):
                    process://  processes on the system: NAME=process SIZE=pid
                    module://   modules in the opened process: NAME=address SIZE=size
                    memory://   allocated blocks of memory: NAME=address SIZE=size
                    heap://     every single allocated heap (slow!): NAME=address SIZE=size
      NAME      Output variable receiving the name of the file, it
                will be "" when there are no other files
      SIZE      Output variable receiving the size of the file, it
                will be -1 when there are no other files
      FILTER    Same job as -F, this filter is valid only if -F
                wasn't specified

    Examples:
      For
          ScanDir "." NAME SIZE
          if NAME == ""
              cleanexit
          endif
      Next
      ...
      For
          ScanDir "." NAME SIZE "*.jpg"
          if NAME == ""
              cleanexit
          endif
      Next


...................................................................

CallDLL DLLNAME FUNC/OFF CONV RET [ARG1] [ARG2] ... [ARGn]

    Command that allows to use plugins inside QuickBMS.
    The idea came from the possibility of using the custom
    decompression and decryption functions located in executables
    and DLLs avoiding the boring reverse engineering of all the
    functions.
    It works with both real files and MEMORY_FILEs (even if they
    contain dll data!).
    Unfortunately this is not much possible with the functions got
    from executables where are used static variables due to some
    technical reasons, in fact it's not possible to relocate them
    in memory.
    For example if the function uses the memory between 006c0000
    and 006d0000 it's highly possible that such range of memory is
    not allocated or is already in use because the executable has
    not been loaded (LoadLibrary) in its original address, that
    space is already occupied.
    There are no problems with DLLs, they are made to be relocated.
    You can even use a dll inside a MEMORY_FILE but be sure it's
    not packed because it may not work.
    And it's also possible to use a raw data containing maching
    instructions, basically you can dump a function "as is" and
    putting it in a MEMORY_FILE.
    That means that the following situations are OK:
    - raw dumped functions or static memory
    - DLLs
    - "maybe" some executables loaded as DLLs

    Arguments:
      DLLNAME   Name of the dll, executable, file or MEMORY_FILE
                where is located the function, example "mylib.dll"
      FUNC/OFF  It can be the name of the function to import in
                which case it must be exported by the dll/exe with
                a name (pay attention to mangled names!)
                Or the relative offset where is located the
                function, remember that the relative offset is NOT
                the absolute one but it's the offset related to the
                image base of the exe/dll, so if normally the dll
                loads at offset 10000000 and the function is at
                10012345 then the offset is 0x12345
      CONV      Calling convention, check calling_convention.h:
                  usercall: allows to set all the 6 x86 registers,
                    any argument after 6th will be pushed on stack
                  cdecl: used by default in many C/C++ compiler
                  stdcall: aka winapi, used by default in Visual C
                  thiscall
                  msfastcall: Microsoft fastcall
                  fastcall: native fastcall
                  borland: the fastcall convention used by the
                    Borland compilers like Delphi
                  pascal
                  watcom
                  safecall
                  syscall
                  optlink
                  carion
                  tcc: use this type to compile the text of the
                    input memory file like a C source code, it
                    automatically wraps the file operations like
                    fgetc/fputc/fread/fwrite on the input/output
                  python: use the memory file like a python script
                    and invoking its functions, QuickBMS variables
                    are globally visible in the script (read-only
                    numbers, writable strings and data)
                  lua: Lua script
                  imagebase: address where the code/dll is loaded
                  address: just imagebase plus OFF
                  entrypoint: the entry point of the executable
                  file2rva: relative virtual address of OFF
                  file2va: imagebase plus file2rva
                  rva2file: file offset of the provided RVA
                  va2file: file offset of the virtual address
      RET       The variable which will contain the value returned
                by the function, use "" if there is no return value
                If you use *RET or &RET then the return value will
                be copied in the RET variable as a string
      [ARGS]    All the arguments of the function, it's also
                possible to use pointers to arguments if they are
                preceded by a & or a * like &SIZE which means that
                the dll/code receives the address of that variable
                and can modify its content. It works only with
                numeric variables

    Examples:
      idstring LZ2K
      get SIZE long
      get ZSIZE long
      log MEMORY_FILE 0xc ZSIZE
      putvarchr MEMORY_FILE2 SIZE 0 # like malloc
      #calldll "TransformersDemo.exe" 0x263c50 cdecl "" MEMORY_FILE MEMORY_FILE2 ZSIZE SIZE    # 00663C50
      calldll "unlzk.dll" "unlz2k" cdecl SIZE MEMORY_FILE MEMORY_FILE2 ZSIZE SIZE
      log "dump.dat" 0 SIZE MEMORY_FILE2

      set MEMORY_FILE binary "
        int foo(int n)
        {
            return n * 1234;
        };
      "
      calldll MEMORY_FILE "foo" "tcc" RET 100


...................................................................

Put VAR TYPE [FILENUM]
...
PutDString VAR LENGTH [FILENUM]
...
PutCT VAR TYPE CHAR [FILENUM]
...

    These commands are EXACTLY like the Get* functions except for
    the fact that they perform write operations.
    For using these commands on a physical file (so MEMORY_FILEs
    excluded) the user MUST use the -w option at runtime, that's
    necessary for both technical and security reasons.
    If you want to write a string without the NULL delimiter use:
    putct "your_string" string -1


...................................................................

GetBits VAR BITS [FILENUM]

    Function for reading bits from the files.
    When you use a GoTo function or change the current offset of
    the file with a Get* command, the variable containing the bit
    position (basically the amount of bits read from the previously
    byte taken from the file) will be reset to 0.
    Note that the function is 100% endian compatible so the result
    changes if you choose the little or big endian mode, remember
    it in case the results don't match what you expected.

    Arguments:
      VAR       Destination variable, can be a number if the bits
                are from 0 to 32 or a string for bigger sizes
      BITS      Number of bits to read
      FILENUM   Number of the file associated to the archive (0)


...................................................................

PutBits VAR BITS [FILENUM]

    Write mode, same format as GetBits


...................................................................

Include FILENAME

    This command loads another script in the current one, it can be
    useful if you have many general functions and you want to avoid
    to copy&paste them in any new script.
    You can place it in any part of your script.

    include "general.bms"
    ->
        load the part of the current script till "include"
        load general.bms
        load the remaining part of the current script after "include"


...................................................................

NameCRC VAR CRC [LISTFILE] [TYPE] [POLYNOMIAL] [PARAMETERS]

    It's not rare to have archives containing files without names,
    just a crc (checksum) calculated on the original filename to
    identify it.
    The NameCRC command exists just in these situations, where you
    have a file containing a list of filenames (maybe collected
    with a debugger or via hooking) and you want to assign them to
    the output files.
    What this command does is just loading the names in LISTFILE,
    calculating some checksums on them using the provided crc
    parameters, and compare the CRC field read in the archive with
    the calculated ones.
    When a crc matches the one in the database, the original
    filename is moved in the VAR variable.
    Note that QuickBMS will automatically calculate various CRC for
    the same filename, by using only slash or backslahs, or all the
    chars to lower or upper case, by removing any "\/.:" char from
    the beginning of the name and so on. This is necessary to grant
    the catching of the right filename in any situation.
    The feature is very fast and the "database" is not so big, so
    you will notice almost no performance issues while using this
    command.

    The function must be used before the *log operations and it can
    be used as initializer at the beginning of the script and then
    with only the first mandatory arguments to retrieve the
    filenames matching the provided crc, for example:
        namecrc DUMMY 0 "names.list" 32
        ...
            get NAME_CRC long
            namecrc NAME NAME_CRC
            log NAME OFFSET SIZE

    Or you can just use it with all the arguments:
        ...
            get NAME_CRC long
            namecrc NAME NAME_CRC "names.list" 32
            log NAME OFFSET SIZE

    Arguments
        VAR     Destination variable that will contain the filename
                or just a "" in case no name has been found (it
                will use a particular variable called
                QUICKBMS_FILENAME).
                If VAR is "" then you can use *log "" and the
                retrieved filename will be automatically applied to
                the output file.
                This behaviour has been thought to write scripts
                easily without providing a NAME variable.
        CRC     This is the CRC field read from the file.
                It must be a HEXADECIMAL value, decimal are no
                longer supported by default.
                Remember, quickbms.exe reads longlong as a 32bit
                field, you must use quickbms_4gb_files.exe to read
                real 64bit fields.
                If necessary, in future will be supported also hash
                algorithms.
        LISTF   The name of the file that contains the filenames,
                each name must be on a new line.
                QuickBMS automatically recognize if the entry is
                just the name or contains also a pre-calculated
                crc, for example:
                    path\folder\file.txt
                    0x11223344  path\folder\file.txt
                    287454020           path\folder\file.txt
                    # path\folder\file.txt
                The list file will be automatically searched and
                loaded from the input and output folders, till it's
                found.
                The filenames located after a comment are good to
                use the same bms script as a file list.
                You can also specify a memory file, good it's a
                compressed type, set MEMORY_FILE10 compressed "..."
        TYPE    Currently only CRC is supported:
                crc32 (default), crc8, crc16 and crc64
        POLY    Polynomial value used to calculate the CRC tables
        PARAM   Both POLY and PARAM work in the same way you can
                set the CRC in the Encryption command through KEY
                and IVEC, so refer to that command for additional
                information and example.
                Anyway PARAM contains:
                BITS, INIT, FINAL, TYPE, REVER and BITMASK_SIDE
                If you are a developer and wants to have a full
                understanding of what you can customize, please
                check the crc_calc function in crc.c, it's very
                simple and self explanatory.
                Example for fnv32:
                namecrc DUMMY 0 "names.list" 32 "" "32 0 0 38 0 1"


...................................................................

Codepage VAR

    Allows to specify a codepage/charset (number or name) for the
    unicode operations like:
      getdstring VAR2 SIZE ; Set VAR1 unicode VAR2
    Currently it works only on Windows and the full list of
    codepages is available at:
    https://msdn.microsoft.com/en-us/library/windows/desktop/dd317756%28v=vs.85%29.aspx

    Arguments
        VAR     the codepage, like utf8 or 950 and so on.

    Example for converting a string from JIS (932) to UTF8:
      get NAME string           # read it as a sequence of bytes
      codepage 932
      codepage cp932
      set NAME to_unicode NAME  # jis -> utf16 (unicode)
      codepage "utf8"
      set NAME unicode NAME     # utf16 -> utf8


...................................................................

SLog NAME OFFSET SIZE [TYPE] [FILENUM] [ID]

    This command allows to export strings to an output file and
    being able to reimport them later with reimport.bat.
    The reimporting feature of this command has the same
    limitations of the global one, so you cannot reimport strings
    that are longer than the original.
    Currently QuickBMS will simply tell you that the new string is
    longer without interrupting the importing so pay attention.
    The space between the end of the new and old string will be
    filled with zeroes but this behaviour "may" be changed in
    future to avoid situations in which there are sequential NUL
    delimited strings and using zeroes will causes problems in the
    software that reads the file.
    Anyway in these situations you can just insert the spaces by
    yourself in the new string.
    The dumped strings are handles as C strings, basically the '\'
    char (backslash) is an escape that allows you to specify any
    byte you desire, the \r and \n you will see are just the 0x0d
    and 0x0a bytes that allow to insert the whole multiline string
    in one line.
    Each line is an exported string.
    Notepad++ is highly suggested to avoid mistakes.
    Small tip: Use INS (the replace character) mode of your text
    editor for editing the string file to reimport so that you will
    have no problems with longer new strings.
    The SLog function is new so any feedback and suggestion is
    highly appreciated.

    Arguments:
        NAME    Name of the output file. It will be created from
                scratch the first time and then will be used in
                append mode from the second line on. Automatic,
                simple and error-proof.
                The output file is just in UTF-8 with Windows line
                feeds ("\r\n") and the conversion of unicode
                strings is performed by QuickBMS automatically with
                the codepage in use.
                Set it to "" for automatic output filename.
        OFFSET  Offset where is located the string you want to dump
                >=0 works just like the other *log commands and
                    dumps the string located at that OFFSET, it
                    doesn't change the current offset
                <0  dumps the string at the current offset and
                    updates it, so it advances in the file
        SIZE    >=0 it works just like the Getdstring command
                    allowing you to dump a string of a certain
                    amount of bytes
                <0  just like the Get command
        TYPE    This is the type of data to read just like the Get
                command, if not specified it's considered String.
                You can dump most of the types and even the
                non-string ones so if you use the Long type you
                will have the decimal value easy to edit inside the
                output file
        FILENUM the input file
        ID      if specified:
                  ""      output: ID_NUMBER=String read from file
                  VAR     output: VAR=String read from file
                only the strings matching the current ID (which is
                a sequential number if "" or a variable if VAR is
                used) will be reimported, so you can delete all the
                other strings and keep only those you edited

    Examples:
      # the test file is available here http://aluigi.org/bms/slog_test.dat
      set STRINGS_FILE string "strings.txt"
      slog STRINGS_FILE -1 0xb string
      slog STRINGS_FILE -1 -1
      slog STRINGS_FILE -1 -1 long
      endian big
      #slog STRINGS_FILE -1 -1 unicode
      slog STRINGS_FILE -1 0x18 unicode
      slog STRINGS_FILE 0x2e -1


...................................................................

Reimport [MODE]

    Just a way for enabling and disabling the reimport mode inside
    the script in specific moments.
    It's experimental and the files will be collected from the
    output folder, therefore they must be already there.
    Remember that you need to use the -w option for activating the
    writing mode.
    The variable QUICKBMS_REIMPORT contains the current mode, check
    it at runtime if you need different actions.

    Arguments:
        MODE        if not set, it will just switch the current
                    mode between 0 (disabled) and reimport mode 1
                0   disabled
                1   like reimport.bat
                2   like reimport2.bat
                3   reimport 3


...................................................................

ImpType MODE VAR [...]

    This is an experimental expansion of a command that existed in
    the original MultiEx language but wasn't needed in QuickBMS.
    It's a way for replacing and updating any field of the file in
    reimport mode and it's unrelated to any *Log operation.

    Arguments and examples:

        imptype var ORIGINAL_VAR NEW_VALUE
            put NEW_VALUE at the location of ORIGINAL_VAR field

        imptype var ORIGINAL_VAR
            update the field where ORIGINAL_VAR was read with its
            current value

        imptype off -16 NEW_VALUE
            put NEW_VALUE at the offset 16 bytes at the end of the
            file, its size is the same of the field that has been
            read at that location or a 32bit LONG if never read

        imptype off 16 NEW_VALUE
            as above but it's at offset 16

        imptype crc ORIGINAL_VAR
            replace the field of ORIGINAL_VAR with the current
            content of QUICKBMS_CRC or QUICKBMS_HASH, it requires
            the usage of Encryption with the relative crc/hash
            algorithm used in the *Log operation
            this command must be used AFTER *Log

        imptype crc ORIGINAL_VAR ALGO ...
            replace the field of ORIGINAL_VAR with the crc or hash
            calculated during a *Log operation and based on the
            chosen ALGO and its parameters, SAME as Encryption.
            this command must be used BEFORE *Log.
            the crc/hash is calculated on the data before
            encryption and before compression
            Example:
                get MYCRC long          # read the field
                imptype crc MYCRC md5 ""
                comtype zlib
                encryption xor "mykey"  # compatible with imptype
                clog NAME OFFSET ZSIZE SIZE
                imptype crc ""          # optional disable


...................................................................

CRCHash ALGO ARG1 ARG2

    Experimental and unsupported command working just like
    Encryption but meant mainly for hashing algorithms and
    performed in different moments, it was meant to help in some
    cases in reimport mode for example when both an encryption and
    an hashing operation are necessary.
    The command was introduced for testing the new behavior of
    imptype and the different order of encryption and hashing in
    reimport mode:
        extraction: decryption + crchash     + decompression
        reimport:   crchash    + compression + encryption
        
...................................................................

NAME:           # it works like the labels in C
Label NAME      # it works like the labels in C
Break           # used in cycles
Break NAME      # it works like goto in C
Continue        # used in cycles
Continue NAME   # it works like goto in C

    Example:
      print "000"
      test:
        print "AAA"
      continue test2
      print "BBB"
      label test2     # "test2:" or "label test2" is the same
        print "CCC"
      continue test

...................................................................
```

QuickBMS can handle also some minimalistic and experimental C
structures like:

```c
debug 1     # needed to show the collected information
struct test {
  int       var1;
  char      var2;
  char      *mystring;
  uint8_t   data[10];
}
```

These operations are all converted to Get* commands while they are
converted in Put* if there is a '=' after them, like:

```c
debug 1
struct test {
  int       var1 = 0x11111111;
  char      var2 = 0x22;
  char      *mystring = "hello";
  uint8_t   data[10] = OTHER_VAR;
}
```

Maybe in future versions it could be improved but at the moment
it's tagged as an experimental and alternative feature only in case
you don't know the bms syntax or it takes time to convert a C
struct in bms language.

Example of -s option, zlib decompression in one command-line
without using script files:

  `quickbms.exe -s "comtype zlib ; get ZSIZE asize ; xmath SIZE \"ZSIZE * 10\" ; clog \"dump.dat\" 0 ZSIZE SIZE" "" input_file.dat output_folder`

  `quickbms.exe -s "comtype gzip_compress ; get SIZE asize ; clog new.gz 0 SIZE SIZE" "" YOUR_INPUT_FILE`

###################################################################


### 5) Experimental input and output, other features


A] Experimental input and output
--------------------------------

From version 0.5.1 of QuickBMS I started to implement some
alternative input/output methods.
At the moment these alternatives cover the following operations:

- Network socket operations specified by an URL like udp:// and
  tcp:// so the tool can be used to send custom packets and data
  via TCP and UDP to one or more network hosts.
  Required command-line option: -n or -network
  URL format:
    tcp://host:port
    tcp://host:port,ssl,force_new_socket

- Process operations specified by an URL like process:// or
  memory:// and allow to read and write the various processes
  running on the system.
  Required command-line option: -p or -process
  URL format:
    process://process_name
    process://pid
    process://pid:module_name

  Experimental debug mode available by adding "debug" after the
  parameters: process://pid/debug
  In this way the process will be debugged by QuickBMS and when
  there is a breakpoint or an exception the process will be
  freezed and all the registers dumped in variables with their
  names.
  additionally QuickBMS will keep in memory all the INT3 you set
  and automatically restore them when you want to continue the
  execution.
  You can find an example script here:
    http://aluigi.org/bms/simraceway_getkey.bms

- Audio operations specified by an URL like audio:// or wave://
  and allow to record audio from the default input device (like
  microphone) and play.
  Currently the "device" parameter is not used.
  Required command-line option: -A or -audio
  URL format:
    audio://device,sample_rate,channels,bits

- Video operations specified by an URL like video:// or
  graphic:// and allow to grab the screen and display the images.
  set window_name to null or none for using the whole screen in
  read mode.
  Required command-line option: -g or -video
  URL format:
    video://window_name,width,height,bits

- Windows messages specified by an URL like winmsg:// but at the
  moment it's possible only to send messages and using 3 long
  numbers: message, wparam and lparam.
  Required command-line option: -m or -winmsg
  URL format:
    winmsg://window_name


I had this crazy idea in my mind for over one year and I decided to
implement it now just because it's completely crazy and can work
only if the user uses the needed options at command-line for
security reasons.

After all QuickBMS implements a lot of algorithms so for me it's a
lot more comfortable to be able to use it for my tests with the
network data and I guess some modders could find useful the process
operations for dumping textures and other models directly from the
memory.
Anyway keep in mind that this is all experimental stuff.

The following is an example script for the network operations:

    log MEMORY_FILE 0 0
    put 0x11111111 long MEMORY_FILE
    put 0x22222222 long MEMORY_FILE
    put 0x33333333 long MEMORY_FILE
    put "hello" string MEMORY_FILE
    put 0x44444444 long MEMORY_FILE
    get SIZE asize MEMORY_FILE
    log "tcp://127.0.0.1:1234" 0 SIZE MEMORY_FILE
    log "udp://localhost:1234" 0 SIZE MEMORY_FILE

or

    log MEMORY_FILE 0 0
    put "GET / HTTP/1.0" line MEMORY_FILE
    put "User-Agent: Mozilla" line MEMORY_FILE
    put "Referer: http://localhost/test.htm" line MEMORY_FILE
    put "" line MEMORY_FILE
    get SIZE asize MEMORY_FILE
    log "tcp://127.0.0.1:80" 0 SIZE MEMORY_FILE


Command-line:
  `quickbms -n script.bms "" ""`

While the following is a simple HTTP download to use with:

    quickbms -n script.bms "tcp://aluigi.org:80" "" > output.htm
    
    get HOST filename
    string HOST p= "Host: %s" HOST
    put "GET / HTTP/1.1" line
    put HOST line
    put "User-Agent: Mozilla" line
    put "Connection: close" line
    put "" line
    for
        get DATA line
        print "%DATA%"
    next


Example of results from "get" when used on a http:// input file in
QuickBMS 0.11 (older versions were different):

    filename     file.php
    fullname     https://zenhax.com/download/file.php?id=9002
    basename     file
    extension    php
    fullbasename download/file
    filepath     download

Funny example that inverts the colors of the first notepad window:

    set NAME string "video://notepad"
    open "" NAME
    get SIZE asize
    filexor 0xff
    log NAME 0 SIZE

Launch notepad and then run:
  `quickbms -g script.bms "" ""`

How to close Firefox:

    put 18 long   # WM_QUIT
    put 0 long    # wParam
    put 0 long    # lParam
    
    quickbms -m script.bms "winmsg://firefox" ""

In future I could decide to add other operations and I'm interested
in any other idea.



B] Other features
-----------------

Other experimental features are the support of most of the commands
used in templates of WinHEX:
http://www.x-ways.net/winhex/templates/index.html

Usually these templates work immediately while sometimes it's only
necessary to manually separate some arguments like "arg1""arg2"
into "arg1" "arg2".

QuickBMS has also the great feature of dumping an HTML file with
the parsed format highlighted through the option -H.
This is a very cool feature that can help many people and doesn't
require additional modifications, just use the original BMS scripts
as usual.
Unfortunately the generated HTML file is not optimized yet and so
it takes lot of memory and CPU to be loaded.

The QuickBMS process supports some return code numbers used when
the tool terminates due to a success or a fail, you can find the
list at the beginning of src\defs.h.
QUICKBMS_OK (success) is ever 0 while QUICKBMS_ERROR_* are referred
to problems.



C] Modkit distribution of quickbms.exe
--------------------------------------

In response to a request of a modder, I have decided to add a
simple feature to allow modders and modkits developers to embed a
script in quickbms.exe when they distribute it in their products,
so the user will not be asked to select the script.

How to do it:

- open quickbms.exe with a hex editor
- search the string "SET THIS BYTE X TO 0x00"
- replace the 'X' (0x58) with a NULL (0x00):


    53 45 54 20 54 48 49 53 20 42 59 54 45 20 58 20   SET THIS BYTE X
    53 45 54 20 54 48 49 53 20 42 59 54 45 20 00 20   SET THIS BYTE  

- `upx.exe -9 quickbms.exe`

- `copy /b  quickbms.exe  +  script.bms   output.exe`

That's all, anyway if you want to use the "classical" way and being
able to specify options, input file and output folder, it's better
to use the BAT solution with the -G option for the GUI mode:

  EXTRACT.BAT:
    `quickbms.exe -G OPTIONS SCRIPT INPUT OUTPUT`



D] web API and named pipe/mailslot IPC interface
------------------------------------------------

The -W command-line option starts the IPC mode which includes:
- web api running on the port specified with the -W option, if the
  port is negative (like -1) there will be no web API running
- named pipe IPC in byte mode on \\.\pipe\quickbms_byte
- named pipe IPC in message mode on \\.\pipe\quickbms
- mailslot IPC on \\.\mailslot\quickbms\send with
  \\.\mailslot\quickbms\recv open in write mode (create it on your tool)

These interfaces have been successfully tested on both Windows and
Linux and the following is a quick set of examples for how using
them for decompressing data, those 302 and 1028 are only an example
of input and output size:

Example of web API /compress:

    POST http://127.0.0.1:1234/compress?algo=zlib&size=1028
    Content is compressed input "as-is" (application/octet-stream)

Example of web API /crypt:

    POST http://127.0.0.1:1234/crypt?algo=aes&key=0123456789abcdef
    Content is the encrypted input "as-is" (application/octet-stream)

Example of web API /crypt (base64):

    POST http://127.0.0.1:1234/crypt?algo=aes&key64=MDEyMzQ1Njc4OWFiY2RlZg==&ivec64=MDEyMzQ1Njc4OWFiY2RlZg==&mode=1
    Content is the encrypted input "as-is" (application/octet-stream)

Example of Named pipe (byte mode):

    CreateFile      \\.\pipe\quickbms_byte
    send: "comtype zlib\n"
    send: "302\n"
    send: 302 bytes of compressed data
    send: "1028\n"
    recv: "1028\n"
    recv: 1028 bytes of decompressed data

Example of Named pipe (message mode):

    CreateFile      \\.\pipe\quickbms
    send: "comtype zlib"
    send: "302"
    send: 302 bytes of compressed data
    send: "1028"
    recv: "1028"
    recv: 1028 bytes of decompressed data

Example of Mailslot:

    CreateFile      \\.\mailslot\quickbms\send  GENERIC_WRITE
    send: "comtype zlib"
    send: "302"
    send: 302 bytes of compressed data
    send: "1028"
    CreateMailslot  \\.\mailslot\quickbms\recv
    recv: "1028"
    recv: 1028 bytes of decompressed data

The IPC interface supports the encryption command too and other
features and commands may be added in future.
Currently the web API supports also /script and /file that are
meant mainly for debugging an input script and an input file based
on the script previously provided. In the latter case there will be
no output file generated just like with the -0 option (the
TEMPORARY_FILE may be the only exception).
Please remember that it's all meant to be used in a single-thread
environment since quickbms can only handle one operation at time,
so two concurrent queries will make some mess.

Examples for IPC (two named pipes and mailslot) and quickbms.dll:
https://zenhax.com/viewtopic.php?p=35965#p35965



### E] quickbms.dll

http://aluigi.org/papers/quickbms_dll.zip

At that location you should be able to download quickbms.dll, I'm
not sure if I will continue to support this feature in future but
currently it's just the whole quickbms built as a shared library
for calling its decompression and encryption algorithms inside other
programs.
The dll exports all the functions of quickbms (cdecl) and its
libraries but the following are functions meant specifically for
external programs:

    int __stdcall quickbms_compression2(char *algo, void *dictionary, int dictionary_len, void *in, int zsize, void *out, int size);

    int __stdcall quickbms_compression(char *algo, void *in, int zsize, void *out, int size);

    int __stdcall quickbms_encryption(char *algo, void *key, int keysz, void *ivec, int ivecsz, int mode, void *data, int size);

Due to the huge size of the dll it's not suggested to use it,
better to implement the necessary algorithms (usually just one)
inside the own program.


###################################################################

# 6) Notes



The following are some exceptions in the usage of QuickBMS.
They are not real bugs, rather they are things that can't work (at
least at the moment) due to the very flexible nature of the tool
or things that it's useful or interesting to know:

#### ? (partially solved)

  Number and strings, due to the usage of the optimizations the
  following script will NOT result in "mytest46600x12349999999999",
  the result will be "mytest4660-1717986919":

    set NAME string "mytest"
    set NUM long 0x1234
    string NAME += NUM
    print "%NAME%"
    set NUM string "0x12349999999999"
    string NAME += NUM
    print "%NAME%"

  This is a good compromise because the previous situation is very
  very "rare" and in any case can be bypassed using multiple
  "string NAME += chr" and the gain in performance is incredible
  for the multiple in-script operations, so it's the best solution.
  Additionally you can use the printf-like string command and the
  binary type with Set:

    set NAME string "mytest"
    set NUM1 long 0x1234
    set NUM2 binary "0x12349999999999"
    string NAME p= "%s0x%x%s" NAME NUM1 NUM2
    print "%NAME%"

- Any Clog operation with a compressed or uncompressed size
  minor/equal than zero produces a file with a zero size, but this
  is not a problem of the tool because it's the perfectly logical
  behavior in these situations.

  If it's necessary to use a compression which gets the SIZE value
  automatically (like base64 or stalker_lza) is enough to specify
  the compressed size as uncompressed size:

    `clog NAME OFFSET ZSIZE ZSIZE`

  or

   `clog NAME OFFSET ZSIZE 1`

- The tool has been created to be 100% compatible with the original
  MexScript language and its syntax/logic, so I tried to add not
  many new commands and, if possible, providing an alternative
  using the original set of commands (for example the Strlen
  command and "Set VAR strlen VAR").
  I tried also to maintain the logic of the program (for example
  encryptions/compressions applied in the file operations only).
  If something looks complex or senseless, it has been made for
  matching the original structure and logic of the language.

- QuickBMS grants compatibility with the original MexScript
  language that implements also some static and partially
  undocumented variables like:
    `EXTRCNT, BytesRead, NotEOF, SOF, EOF`

  If you are writing a script for QuickBMS try to avoid these
  variable names except if you really need and know what they do.

- QuickBMS uses many third party code and, even if I tried to
  adjust them a bit where possible, unfortunately many of these
  external functions were a disaster or missed any security
  requirement.
  That's the reason why the comtype scanning feature causes so
  many crashes with invalid data.
  From version 0.5.5 I added a particular type of allocation
  management that allows a better debugging of the code and at the
  same time protects the heap from contiguous buffer overflow and
  underflow (so it can do nothing against "buff[0x11223344] = 'a').
  It's not a solution but at least helps me a lot and limits the
  problems caused by third party non-safe code.
  The only protection of the stack is provided by the
  -fstack-protector-all compiler option of Gcc.

- Security:
  It's hard to make the tool completely safe, anyway the following
  are some notes and solutions:
  - allocated memory set as read/write only with guarded page
    before and after the buffer, they act like a "cage" that
    delimits the buffer
  - usage of Gcc -fstack-protector-all
  - the user is EVER prompted of activating dangerous features like
    the usage of dlls and the calling of external executables
  - some checks to avoid the problems caused by the big redundant
    code of which QuickBMS is full (unfortunately, sorry for that)
  - keep in mind that QuickBMS is mainly a testing tool in which I
    preferred to insert strange and particular features rather than
    making it "secure" for any user, it's the responsibility of the
    user to use only trusted scripts and paying attention to the
    warnings displayed by the tool

- The EXECUTE mode of ComType and Encryption grants compatibility
  with any compression and encryption tool (command-line) based on
  algorithms not yet supported by QuickBMS, and at the same time
  avoids the rush of trying to implement "everything" as soon as
  possible.
  I used system() for this command just because I want that it's
  compatible with any possible program included those which require
  input from stdin and output to console (stdout).
  Example: "file.exe < #INPUT# > #OUTPUT#"

#### ? (partially solved)

  All the extracted files are loaded completely in memory before
  being dumped for various technical reasons, so if the file to
  dump has a size of 800 megabytes this is the same size which will
  be allocated in memory or even double or more if the file must be
  decompressed, so it's good to have a large amount of free RAM when
  handling big archives or a large virtual memory/swap space.
  This mechanism is not used for files that don't require encryption
  and compression in which case the operation is performed 1:1 using
  a temporary buffer of only 1 megabyte.

#### x (SOLVED!)

  Log "123.txt" OFFSET SIZE
  It creates the file 123 and not 123.txt, this happens because
  "123.txt" is considered a constant number due to the rule that
  everything starting with a number (or a '-') is handled as a
  constant number.
  This behavior didn't happen with the previous versions of the
  tool because wasn't used the number optimization which saves tons
  of CPU cycles in some scripts.
  * From version 0.3.12 I decided to implement the full
    verification of the string to know if it's a number or a
    string, luckily there is almost no loss of performances

#### x (SOLVED!)

  The following do NOT work because the QuickBMS variables are case
  INsensitive:
  if SIGN == "test"     # u== is the same
  elif SIGN == "TeSt"
  ...
  set SIGN1 binary "test"
  set SIGN2 binary "TeSt"
  if SIGN == SIGN1      # u== is the same
  elif SIGN == SIGN2
  ...
  The only way to fix it would be to make quickbms case SENSITIVE,
  this change should give no problems if you have written the
  scripts correctly but exists a 1% of possible issues, currently I
  don't know what to do.
  .
  * From QuickBMS 0.5.31 you can use the -I option to force the
    case sensitive mode on variable names

#### x (SOLVED!)

    set NAME string MEMORY_FILE
    log NAME 0 0

  It produces no physical file because it's considered a
  MEMORY_FILE, it happens because the dumping function receives
  "MEMORY_FILE" as output file name.
  At the moment there is no fix anyway it's a very very rare event
  (never happened to find an archive containing a file with that
  name) and so not a priority.
  * Fixed in version 0.5.17 by checking if the name of the file is
    the name of a variable or its content.

#### x (SOLVED!)

  Crash caused by HsSrv.dll.
  The Asus Xonar and Unixonar drivers cause the crash of QuickBMS
  for the following reason: HsSrv.dll is automatically injected in
  any process and this dll checks all the allocated memory for the
  presence of a "MZ" signature (the one used for the executables):
    1000B462  CALL DWORD PTR DS:[<&KERNEL32.VirtualQuery>]
    1000B468  TEST EAX,EAX
    1000B46A  JBE SHORT 1000B4BE
    1000B46C  CMP DWORD PTR SS:[EBP-24],1000 ; check if State is MEM_COMMIT
    1000B473  JNE SHORT 1000B48B
    1000B475  TEST WORD PTR SS:[EBP-20],0100 ; check if Protect contains PAGE_GUARD
    1000B47B  JNZ SHORT 1000B48B
    1000B47D  AND DWORD PTR SS:[EBP-4],00000000
    1000B481  CMP WORD PTR DS:[ESI],5A4D     ; check if the buffer starts with MZ
  QuickBMS uses a particular memory protection mechanism that in
  the recent versions switched from PAGE_GUARD to PAGE_NOACCESS,
  that's why HsSrv.dll crashes: if Protect contains PAGE_GUARD then
  it skips the MZ check but now it's PAGE_NOACCESS.
  Using MEM_COMMIT | MEM_RESERVE doesn't help to skip the code with
  the first check because VirtualQuery returns only MEM_COMMIT.
  Asus should fix the bug by checking if Protect is set to a
  non-readable flag, I have NOT contacted them.
  Some possible solutions are the following:
  - disable the GX mode (emulated EAX) of the Asus driver
  - disable the Asus HookSupport Manager application (HsMgr.exe)
  - start QuickBMS with the -9 option (create a shortcut)
  - contact Asus! :)
  Note that the problem seems to happen only when QuickBMS is
  launched with the GUI (double-click) while it's calling the
  Windows API GetOpenFileName.
  * From version 0.5.25c I use PAGE_GUARD to avoid any problem with
    buggy third party drivers.

? When you assign a string to a variable pay attention to the
  backslash char: \
  It's used as escape when parsing the bms script and a quoted
  string is found, like "test".
  The only limitation is caused by the presence of the same quote
  char after the backslash so the following command is wrongly
  interpreted:
    string VAR R "test1 and test2\" "/"
  In that case the \" is interpreted as " without terminating the
  handling of the quote string.
  For that specific case there is no solution at the moment because
  \\ is interpreted as \\ and not as \.
  Consider that this is a very rare case and if you want to replace
  the backslash with slashes it's enough to use:
    string VAR R \ /


Other things to know or strange behaviors will be listed when I
will figure and remember them.

A curiosity for who is crazy for the optimizations of compilers:
the PPMD (ppmd var.i rev.1) algorithm compiled with -O3 (Gcc) is a
lot slower than if compiled with -O2 and a similar situation is
valid also for other algorithms.
With -Os the code is smaller (about 300kb the Windows exe of an old
quickbms version) but there is a loss of performances of max 15/20%
with some algorithms (like PPMD) and scripts with many getvarchr,
putvarchr and math operations.


###################################################################

# 7) Support


QuickBMS, like many of my projects, is fully supported by me and is
ever in active development for adding new encryption and
compression algorithms, adding new features, fixing bugs and other
improvements.
I'm the first and biggest user of this tool, so I have a direct
interest in maintaining it.

The latest version is available on the following website:

  http://quickbms.com

RSS feeds available on my website so stay tuned for any update of
QuickBMS and my other tools:

  http://aluigi.org/rss.php

Remember to contact me for any doubt or new idea regarding QuickBMS
by e-mail at me@aluigi.org or on the forum in this topic
http://zenhax.com/viewtopic.php?f=13&t=556

You are also invited to post your doubts, feedback and suggestions
on the official support forum called ZenHAX: http://zenhax.com
It's a community where the users can write about game research,
file format reversing, game internals and security.

My old forum on http://forum.aluigi.org is no longer supported from
2011 but it contains some additional old information and examples.

QuickBMS is a free project, no donations or money are accepted.
If you like it feel free to spread the word about it.
You may also like to make tutorials and videos, they are welcome so
more people can learn to use it.

QuickBMS wants to be THE EXTRACTION TOOL for almost any game
related task so "help it to help yourself" :)


###################################################################

# 8) Additional credits


QuickBMS uses various public-domain code and code released under
GPL/LGPL or other open source and free licenses.

Compression:
- zlib, inflateback9 (for deflate64) and blast of Jean-loup Gailly and
  Mark Adler http://www.zlib.net
- LZO of Markus F.X.J. Oberhumer http://www.oberhumer.com/opensource/lzo/
- LZSS, LZARI, LZHUF of Haruhiko Okumura
- unlzx.c of Erik Meusel
- LZMA and LZMA2 of Igor Pavlov http://www.7-zip.org/sdk.html
- bzip2 of Julian Seward https://www.sourceware.org/bzip2/
- ascii85 partially derived from http://www.stillhq.com/svn/trunk/ascii85/decode85.c
- libmspack of Stuart Caie http://www.cabextract.org.uk/libmspack/
- lzjb from http://cvs.opensolaris.org/source/xref/onnv/onnv-gate/usr/src/uts/common/fs/zfs/lzjb.c
- iMatix Standard Function Library compression http://download.imatix.com/pub/
- UCL of Markus F.X.J. Oberhumer http://www.oberhumer.com/opensource/lzo/
- code from the uncompress utility of "The Regents of the University of California"
- Dynamic Markov Compression implementation of Gordon V. Cormack
  http://plg1.cs.uwaterloo.ca/~ftp/dmc/dmc.c
- many algorithms from ScummVM http://scummvm.sourceforge.net
- bpe of Philip Gage http://www.csse.monash.edu.au/cluster/RJK/Compress/bpd.c
- QuickLZ of Lasse Mikkel Reinhold http://www.quicklz.com
- Quake 3 Huffman code of Id Software http://www.idsoftware.com
- mszh from the LossLess Codec Library
- Doom Huffman code from the Doom/Hexen source code
- aPLib of Jorgen Ibsen http://www.ibsensoftware.com/products_aPLib.html
- LZF of Marc Alexander Lehmann http://home.schmorp.de/marc/liblzf.html
- LZ77 of Arkadi Kagan http://compressions.sourceforge.net/about.html
- LZRW algorithms of Ross Williams http://www.ross.net/compression/
- an Huffman implementation of Bill Demas on LDS
- the FIN algorithm (useless and very close to LZSS) on LDS
- LZAH/LZH12/13 of Dik T. Winter http://homepages.cwi.nl/~dik/english/ftp.html
- GRZipII/libGRZip of Grebnov Ilya (only the win32 code is linked to it)
  because it's composed by many files and it's not a priority)
- rle of Chilkat Software http://www.chilkatsoft.com/chilkatdx/ck_rle.htm#source
- Quad of Ilia Muraviev http://quad.sourceforge.net
- Balz of Ilia Muraviev http://balz.sourceforge.net
- unshrink of Info-Zip http://www.info-zip.org/UnZip.html
- PPMd algorithms of Dmitry Shkarin http://compression.ru/ds/
- BriefLZ of Jorgen Ibsen http://www.ibsensoftware.com/download.html
- PAQ6 of Matt Mahoney http://cs.fit.edu/~mmahoney/compression/paq.html#paq6
- shcodec of Simakov Alexander http://webcenter.ru/~xander/
- hstest of tom ehlert
- SixPack of Philip G. Gage
- ashford of Charles Ashford
- JCALG1 of Jeremy Collake http://www.bitsum.com/jcalg1.htm
- jam/unjam of W. Jiang
- lzhlib of Haruhiko Okumura and Kerwin F. Medina for the adaptation of the code
- Srank P M Fenwick http://www.cs.auckland.ac.nz/~peter-f/FTPfiles/srank.c
- Zziplib/Zzlib of Damien Debin http://damiendebin.net/archives/zzip/download.php#zzlib
- scpack of Philip Gage
- rle3 and bpe2:
  http://musyozoku211.blog118.fc2.com/blog-entry-13.html
  http://blog-imgs-17.fc2.com/m/u/s/musyozoku211/bpe2.txt
  http://blog-imgs-17.fc2.com/m/u/s/musyozoku211/rle3.txt
- Basic Compression Library of Marcus Geelnard http://bcl.comli.eu
- SCZ of Carl Kindman http://scz-compress.sourceforge.net
- szip of HDF Group http://www.hdfgroup.org/doc_resource/SZIP/
- sr3c of Kenneth Oksanen http://cessu.blogspot.com
- Huffman library of Douglas Ryan Richardson http://huffman.sourceforge.net
- SFastPacker of Aleksey Kuznetsov http://www.utilmind.com/delphi3.html
- lz77wii of Hector Martin http://wiibrew.org/wiki/Wii.py
- prs 8ing code posted by tpu http://forum.xentax.com/viewtopic.php?p=30387#p30387
- puyo compressions of not.nmn and nickwor https://github.com/nickworonekin/puyotools
- falcom compression of http://www.geocities.jp/pokan_chan/
- cpk of hcs http://hcs64.com/files/utf_tab04.zip
- DSDecmp/goldensun/luminousarc of Barubary http://code.google.com/p/dsdecmp/
- pglz_decompress PostgreSQL Global Development Group http://www.postgresql.org/
- SLZ: versions of Adisak Pochanayon and CUE
- LZH-Light of Sergey Ignatchenko ftp://66.77.27.238/sourcecode/cuj/1998/cujoct98.zip
- d3101 of Advanced Hardware Architectures/HP
- squeeze (R. Greenlaw, Amiga port by Rick Schaeffer ???)
- some algorithms of Mark Nelson & Jean-loup Gailly from The Data Compression Book
- Ed Ross Data Compression
- ilzr of Jose Renau Ardevol
- some code from the C User's Journal
- dmc by T.L. Yu
- 'Uses libLZR by BenHur' http://www.psp-programming.com/benhur/
- lzs of Matthew Chapman http://www.rdesktop.org
- yaz0 of thakis (http://www.amnoid.de/gc/)
- RNC by Jon http://www.yoda.arachsys.com/dk/ or fork by Simon Tatham
- PAK_explode of Cyril VOILA
- The KENS Project Development Team
- dragonballz by Geoffrey W. Curtis
- unstargun by Adam Nielsen / The_coder
- ntcompress from Nintendo Wii Revolution SDK
- crle of Arkadi Kagan http://compressions.sourceforge.net/about.html
- CTW by Frans Willems http://www.ele.tue.nl/ctw
- DACT by Roy Keene http://www.rkeene.org/oss/dact/
- algorithms by Brendan G Bohannon http://bgb-sys.sourceforge.net
- lzpxj by Ilia Muraviev and Jan Ondrus http://sourceforge.net/projects/lzpx/
- rle from ftp://ftp.elf.stuba.sk/pub/pc/pack/mar.rar
- rle from http://gdcm.sourceforge.net
- lzmat of Vitaly Evseenko
- dict from http://freearc.org/download/research/dict.zip
- rep from http://freearc.org/download/research/rep.zip
- lzp by Dmitry Shkarin http://www.compression.ru/ds/lzp.rar
- kzip by Ken Silverman http://advsys.net/ken/utils.htm
- enet http://enet.bespin.org
- eduke32 http://eduke32.com
- xu4 - Ultima IV recreated http://sourceforge.net/projects/xu4/
- Lemur http://www.lemurproject.org
- lzfu by Dave Smith and Carl Byington http://www.five-ten-sg.com/libpst/
- he3 by Eric Prevoteau http://savannah.nongnu.org/projects/dctc/
- Ultima Iris http://www.iris2.de http://ultimairis.sourceforge.net
- http://sourceforge.net/projects/linux-ntfs/
- pdb2txt http://code.google.com/p/pdb2txt/
- Comprlib http://sourceforge.net/projects/comprlib/
- prs by Fuzziqer http://www.fuzziqersoftware.com/projects.html
- sega_lz77 converted from an ICE decompression tool developed by
  scriptkiddie (XentaX's forum)
- saint_seya compression by MrAdults (Senor Casaroja's Noesis)
  http://forum.xentax.com/viewtopic.php?p=52279#p52279
- lz4 by Yann Collet https://github.com/Cyan4973/lz4
- Snappy http://google.github.io/snappy/
- Lunar compression dll by FuSoYa http://fusoya.eludevisibility.org
- lzv1 by Hermann Vogt
- FastLZ by Ariya Hidayat http://fastlz.org
- zax http://code.google.com/p/zax/
- data-shrinker by fusiyuan http://code.google.com/p/data-shrinker/
- mmini by Adam Ierymenko http://code.google.com/p/mmini/
- clzw by Vladimir Antonenko http://code.google.com/p/clzw/ - http://sourceforge.net/projects/clzw/
- lzham by Richard Geldreich https://github.com/richgel999/lzham_codec
- lpaq8 by Matt Mahoney http://www.cs.fit.edu/~mmahoney/compression/
- sega_lzs2 by Treeki
- Core Online decompression by Ekey http://www.progamercity.net
- lzlib http://lzip.nongnu.org/lzlib.html
- some compression tools from http://www.romhacking.net
- pucrunch by Pasi 'Albert' Ojala
- libzpaq by Matt Mahoney http://mattmahoney.net/dc/zpaq.html
- zyxel-revert http://git.kopf-tisch.de/?p=zyxel-revert
- Blosc https://github.com/Blosc/c-blosc
- Gipfeli by Jyrki Alakuijala https://github.com/google/gipfeli
- Crush, Balz, BCM and possibly others by Ilya Muravyov http://sourceforge.net/projects/crush/
- Yappy https://raw.github.com/richard-sim/Compression-Test-Suite/master/CompressionSuite/Yappy/yappy.cpp
- liblzg by Marcus Geelnard http://liblzg.bitsnbites.eu/
- Doboz by Attila T. Afra https://bitbucket.org/attila_afra/doboz
- XPK http://www.jormas.com/~vesuri/xpk/
- http://www.amiga-stuff.com/crunchers-download.html
- http://aminet.net/package/util/libs/ulib4271
- PackFire by Neural http://www.pouet.net/prod.php?which=54840
- Matt Mahoney for various compression algorithms
- http://blog-imgs-17.fc2.com/m/u/s/musyozoku211/bpe.txt
- CBPE by Izaya http://izaya.blog38.fc2.com/blog-entry-374.html
- Alba by xezz http://encode.ru/threads/1874-Alba?p=36612&viewfull=1#post36612
- http://download.wcnews.com/files/documents/sourcecode/shadowforce/transfer/asommers/mfcapp_src/engine/compress/
- QFS https://raw.githubusercontent.com/wouanagaine/SC4Mapper-2013/master/Modules/qfs.c
- Zen Studios decompression by Ekey http://www.progamercity.net
- OpenXRay https://github.com/OpenXRay/xray-16/blob/master/src/xrCore/LzHuf.cpp
- ZSTD https://github.com/Cyan4973/zstd
- AZO http://www.altools.com/ALTools/ALZip/Egg-format.aspx
- PowerPacker from libsidtune https://github.com/bithorder/sidplayer/blob/master/jni/libsidplay2/sidtune/PP20.cpp
- Nintendo DS/GBA compressions by CUE http://www.romhacking.net/utilities/826/
- pclzfg http://www.embedded-os.de/en/pclzfg.shtml
- Heatshrink https://github.com/atomicobject/heatshrink
- TurboRLE https://github.com/powturbo/TurboRLE
- Smaz https://github.com/antirez/smaz
- lzfx http://code.google.com/p/lzfx/
- Pithy https://github.com/johnezang/pithy
- libzling https://github.com/richox/libzling
- Density https://github.com/centaurean/density
- Brotli https://github.com/google/brotli
- code by Gerald Tamayo
- libbsc http://libbsc.com/
- Shoco https://ed-von-schleck.github.io/shoco/
- WFLZ https://github.com/ShaneWF/wflz
- FastAri https://github.com/davidcatt/FastARI
- Dicky https://github.com/jedisct1/Dicky
- Squish https://github.com/Bananattack/squish
- lzjody https://github.com/jbruchon/lzjody
- ms-compress https://github.com/coderforlife/ms-compress
- yay0dec by thakis http://www.amnoid.de/gc/
- dmsdos http://cmp.felk.cvut.cz/~pisa/dmsdos/
- iROLZ http://ezcodesample.com/rolz/rolz_article.html
- Mcomp http://msoftware.co.nz
- SimPE http://sims.ambertation.de/
- Adam Nielsen for Camoto http://www.shikadi.net/camoto
- OpenKB http://openkb.sourceforge.net/
- OpenTitus http://opentitus.sourceforge.net/
- deLZW http://cnub.ddns.net/deLZW.ashx
- various pseudocode from http://www.shikadi.net/moddingwiki/Category:Compression_algorithms
- Ladislav Zezula for PKLib
- Marc Winterrowd http://nodling.nullneuron.net/ultima/ultima.html
- tkatchev https://bitbucket.org/tkatchev/yalz77
- LZ5/Lizard https://github.com/inikep/lz5
- various compression algorithms from Lab313 https://github.com/lab313ru
- LZSS http://www.metroid2002.com/retromodding/wiki/LZSS_Compression
- SynLZ https://raw.githubusercontent.com/synopse/mORMot/master/SynLZ.pas
- PPMZ2 http://www.cbloom.com/src/ppmz.html
- OpenDark http://sourceforge.net/projects/dark/
- Oodle http://www.radgametools.com/oodle.htm (DLL from Warframe)
- jdlz recompressor http://encode.ru/threads/2417-Creating-A-Compressor-for-JDLZ?p=46247&viewfull=1#post46247
- rfpk http://www.rockraidersunited.com/topic/6675-is-there-a-way-i-could-rip-files-of-lego-city-undercovers-disc/#comment-120442
- wp16 http://romxhack.esforos.com/compresion-de-final-fantasy-1-de-psp-la-famosa-wp16-t44
- Nisto https://github.com/Nisto/bio-tools/tree/master/bio0/alz-tool
- Ekey for Revelation Online / TianYu
- ps_lz77 by TheUkrainianBard http://zenhax.com/viewtopic.php?p=14313#p14313
- lzfse https://github.com/lzfse/lzfse
- dzip https://www.madewithmarmalade.com/developer
- CSC https://github.com/fusiyuan2010/CSC
- Gundam Ghiren converted to C http://zenhax.com/viewtopic.php?p=18646#p18646
- glza by Kennon Conrad http://encode.ru/threads/1909-Tree-alpha-v0-1-download?p=50293&viewfull=1#post50293
- m99coder by Yuta Mori
- https://github.com/solaris573/taikotools
- https://github.com/nekomiko/recetunpack/blob/master/data_ext.c
- https://github.com/BlackDragonHunt/Danganronpa-Tools/blob/master/drv3/drv3_dec.py
- https://github.com/gildor2/UModel/blob/master/Unreal/UnCoreCompression.cpp
- https://forum.xentax.com/viewtopic.php?p=119352#p119352
- https://encode.ru/threads/2772-Finding-custom-lzss-on-arcade-game-dat-file?p=52946&viewfull=1#post52946
- liblzs by Craig McQueen https://github.com/cmcqueen/lzs-compression
- shrek decompression by ShrekBoards https://github.com/ShrekBoards/shrek-decompress
- qcmp converted from https://github.com/gibbed/Gibbed.SleepingDogs/blob/master/Gibbed.SleepingDogs.FileFormats/QuickCompression.cs
- ykcmp converted from https://github.com/Xkeeper0/disgaea-pc-tools/blob/master/disgaea/compressionhandler.php
- swzap https://github.com/wasaylor/unzap
- mzx converted from https://github.com/Hintay/PS-HuneX_Tools/blob/master/tools/mzx/decomp_mzx0.py
- lzrrv by bnm
- SLZ3 by akderebur
- mppc by Marc-Andre Moreau
- ALZSS by Elijah H. Brolijah https://github.com/Brolijah/Aqualead_LZSS
- CLZ by https://github.com/sukharah/CLZ-Compression
- https://raw.githubusercontent.com/zhaihj/konami-lz77/master/src/lz77.rs
- GARBro by morkt https://github.com/morkt/GARbro
- HCS for puff8, lzh8 and romchu: https://hcs64.com/vgm_ripping.html
- Inuk Syooperstar for Okage XPK compression
- Nisto https://github.com/Nisto/lzsd
- infval https://github.com/infval/pzzcompressor_jojo
- libdeflate https://github.com/ebiggers/libdeflate
- zopfli https://github.com/google/zopfli
- https://github.com/powzix/ooz
- rage_xfs by Benjamin Haisch
- Allen for Wangan decompressions
- Exo Decrunch by Magnus Lind https://bitbucket.org/magli143/exomizer/wiki/Home
- BitBuster https://www.teambomba.net/bombaman/downloadd26a.html
- nibrans by BareRose https://github.com/BareRose/nibrans
- lze by GORRY https://gorry.haun.org/pw/?lze
- Shelwien of encode.su
- Bitbuster of Team Bomba https://www.teambomba.net/bombaman/downloadd26a.html
- lz48 by roudoudou
- ZX compressions by Einar Saukas https://github.com/einar-saukas/ZX0

Encryption:
- all the algorithms provided by OpenSSL http://www.openssl.org
- xtea from PolarSSL http://www.polarssl.org
- some encryption algorithms from GnuPG and libgcrypt http://www.gnupg.org
- ICE of Matthew Kwan http://www.darkside.com.au/ice/index.html
- Rotor module from the Python source code
- http://mcrypt.sourceforge.net
- all the various public algorithms implemented in version 0.4.1 like
  3way, anubis, gost, skipjack and so on
- libkirk of Draan http://code.google.com/p/kirk-engine/
- PC1 Encryption Algorithm of Alexander Pukall http://membres.multimania.fr/pc1/
- LibTomCrypt https://github.com/libtom/libtomcrypt
- LibTomMath https://github.com/libtom/libtommath
- libmcrypt http://sourceforge.net/projects/mcrypt/files/Libmcrypt/
- sphlib http://www.saphir2.com/sphlib/
- cityhash https://code.google.com/p/cityhash/
- xxhash https://github.com/Cyan4973/xxHash
- qLibc https://github.com/wolkykim/qlibc
- StormLib https://github.com/ladislav-zezula/StormLib
- insane coder http://insanecoding.blogspot.com/

Others:
- MemoryModule of Joachim Bauch https://github.com/fancycode/MemoryModule
- various signatures from http://mark0.net/soft-trid-e.html
- various signatures from http://toorcon.techpathways.com/uploads/headersig.txt
- Capstone engine http://www.capstone-engine.org/
- Ollydbg disasm library http://www.ollydbg.de
- optional BeaEngine dissassembler library http://www.beaengine.org
- uthash and utlist http://troydhanson.github.io/uthash/
- TinyCC http://bellard.org/tcc/
- tiny-regex-c https://github.com/kokke/tiny-regex-c

Notes:

- Some (many?) of the original codes have been modified a bit to
  make them usable in QuickBMS for the memory2memory (in-memory)
  decompression and for other possible fixes or for decreasing the
  amount of code, for example removing the compression routine
  leaving only the decompression one.
  Note that I avoided to make this third-party code more secure
  because it's not the job of QuickBMS, so almost all the code
  (except some rare cases) has been used "as-is", the only security
  protections come from the general protection mechanisms adopted
  in QuickBMS like my own heap handling and -fstack-protector-all.

- The files/libraries which have been modified have the header
  "// modified by Luigi Auriemma" which is meant just to show that
  it's not the 100% original code and it must be NOT considered
  like a credit.
  I claim nothing about them, the original license and authors are
  still untouched.

- If the files have been modified or don't have the original
  license information (may happen only with small functions that
  didn't contain a license header in origin) please follow the
  provided links for more details.

- Almost all the algorithms implemented here have been selected by
  me because they:
  - have been used
  - "may" have been used
  - it has been claimed to have been used
  in real software and games, or they are enough known and famous
  to deserve their implementation in QuickBMS.
  Personally I prefer to have many algorithms implemented also to
  help my compression and encryption scanners:
  comtype_scan2.bat/bms and encryption_scan.bat/bms).

- Tell me if I forgot someone or something in this section, it may
  be possible that some credits are not complete.
  And tell me also if it's necessary to include other files or
  comments inside these third-party files or about them.
  I included the list to the original websites as additional
  reference also for having more information about their license in
  case the included files don't have it in their comments (/* */).


###################################################################
