##
##  grzip.spec -- OpenPKG RPM Package Specification
##  Copyright (c) 2000-2007 OpenPKG Foundation e.V. <http://openpkg.net/>
##  Copyright (c) 2000-2007 Ralf S. Engelschall <http://engelschall.com/>
##
##  Permission to use, copy, modify, and distribute this software for
##  any purpose with or without fee is hereby granted, provided that
##  the above copyright notice and this permission notice appear in all
##  copies.
##
##  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
##  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
##  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
##  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
##  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
##  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
##  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
##  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
##  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
##  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
##  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
##  SUCH DAMAGE.
##

#   package information
Name:         grzip
Summary:      Block-Sorting Lossless Data Compression Algorithm
URL:          http://magicssoft.ru/?folder=projects&page=GRZipII
Vendor:       Grebnov Ilya, Jean-Pierre Demailly
Packager:     OpenPKG Foundation e.V.
Distribution: OpenPKG Community
Class:        EVAL
Group:        Compression
License:      LGPL
Version:      0.2.9
Release:      20070107

#   list of sources
Source0:      ftp://ftp.ac-grenoble.fr/ge/compression/grzip-%{version}.tar.bz2

#   build information
Prefix:       %{l_prefix}
BuildRoot:    %{l_buildroot}
BuildPreReq:  OpenPKG, openpkg >= 20060823, gcc
PreReq:       OpenPKG, openpkg >= 20060823
AutoReq:      no
AutoReqProv:  no

%description
    GRZip II is a high-performance file compressor based on
    Burrows-Wheeler Transform (BWT), Schindler Transform (ST),
    Move-To-Front (MTF) and Weighted Frequency Counting (WFC). It uses
    The Block-Sorting Lossless Data Compression Algorithm (BSLDCA),
    which has received considerable attention over recent years for
    both its simplicity and effectiveness. This implementation has
    compression rate of 2.234bps on the Calgary Corpus (14 files)
    without preprocessing filters.

%track
    prog grzip = {
        version   = %{version}
        url       = ftp://ftp.ac-grenoble.fr/ge/compression/
        regex     = grzip-(__VER__)\.tar\.bz2
    }

%prep
    %setup -q

%build
    %{l_make} %{l_mflags} \
        CC="%{l_cc}" WARNINGS="" OPTFLAGS="%{l_cflags -O}" \
        libgrzip.a grzip_static

%install
    rm -rf $RPM_BUILD_ROOT
    %{l_shtool} mkdir -f -p -m 755 \
        $RPM_BUILD_ROOT%{l_prefix}/bin \
        $RPM_BUILD_ROOT%{l_prefix}/include \
        $RPM_BUILD_ROOT%{l_prefix}/lib
    %{l_shtool} install -c -s -m 755 \
        grzip_static $RPM_BUILD_ROOT%{l_prefix}/bin/grzip
    %{l_shtool} install -c -m 644 \
        grzip.h $RPM_BUILD_ROOT%{l_prefix}/include/
    %{l_shtool} install -c -m 644 \
        libgrzip.a $RPM_BUILD_ROOT%{l_prefix}/lib/
    %{l_rpmtool} files -v -ofiles -r$RPM_BUILD_ROOT %{l_files_std}

%files -f files

%clean
    rm -rf $RPM_BUILD_ROOT
