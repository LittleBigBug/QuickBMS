CTW (Context Tree Weighting) version 0.1
----------------------------------------

This archive contains the source code of CTW version 0.1. 
Please check our website for more information and updates:
    http://www.ele.tue.nl/ctw 

On this website you can find a manual, an overview of the algorithm,
results on the Calgary corpus and Canterbury corpus, and much more.



How to compile CTW
------------------
- On make based systems (e.g. unix and cygwin):
  Type make (or on some systems gmake) in the directory with the CTW
  source files.

- Other systems:
  Copy all source and header files, except ctw_gentables.c (which is
  a small separate program to recalculate some tables), to your project.
  Compile the project as usual in your environment. 
  
After the project is compiled successfully, you can first try to
execute the program without any arguments. You will get the following
output:

CTW (Context Tree Weighting) compression/decompression utility version 0.1

Usage: ctw e/d/i [-options] <input_filename> [<output_filename>]
Where e = encode, d = decode and i = show file information
For encoding, the default output_filename is input_filename.ctw, and for
decoding, the default output_filename is input_filename.d (without .ctw).
Available options:
 -dX : Set maximum tree depth to X
 -tX : Set maximum number of tries in tree array
 -nX : Set maximun number of nodes (size of the tree array)
       values can be specified using 'M' for millions of nodes and 'K' for
       thousands of nodes (eg. -n4M means 4194304 nodes)
       Tree array requires 8*X bytes of memory
 -fX : Set maximum file buffer size; values can be specified using
       'M' for megabytes and 'K' for kilobytes
 -bX : Set maximum value of log beta
 -s  : Disable strict tree pruning (enabled by default)
 -r  : Enable weighting at root nodes (disabled by default)
 -k  : Use Krichevski-Trofimov estimator instead of Zero-Redundancy estimator
 -y  : Force overwriting of existing files
 -lX : Enables logging to file X



Intellectual Property Rights
----------------------------

Please take note of the following intellectual property rights:
The Context Tree Weighting (CTW) algorithm is protected by patents,
assigned to / owned by Koninklijke KPN NV: EP0913035B1, EP0855803,
US6150966, US5986591, US5864308.

All information on the site is made available for Research &
Development purposes only.

For commercial utilization of the CTW algorithm, KPN NV is willing
to grant licenses on reasonable terms and conditions.

For more information, please contact:

KPN NV
Intellectual Property Department
P.O. Box 95321
2509 CH The Hague
The Netherlands
+ 31 70 4460904



Disclaimer
----------
THIS SOFTWARE, IS PROVIDED "AS IS". IN NO EVENT SHALL ANY OF THE
PARTIES INVOLVED BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.