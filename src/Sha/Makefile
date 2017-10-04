#############################################################################
#
# Makefile for SHA on Raspberry Pi
#
# License: GPL (General Public License)
# Author:  Charles-Henri Hallard 
# Date:    2013/03/13 
#
# Description:
# ------------
# use make all and mak install to install the library 
# You can change the install directory by editing the LIBDIR line
#
PREFIX=/usr/local

# Library parameters
# where to put the lib
LIBDIR=$(PREFIX)/lib
# lib name 
LIB=libSHA1
LIB2=libSHA256
# shared library name
LIBNAME=$(LIB).so.1.0
LIBNAME2=$(LIB2).so.1.0

# Where to put the header files
HEADER_DIR=${PREFIX}/include/SHA1
HEADER_DIR2=${PREFIX}/include/SHA256

# The recommended compiler flags for the Raspberry Pi
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

# make all
# reinstall the library after each recompilation
all: libSHA1 libSHA256

# Make the library
libSHA1: sha1.o
	g++ -shared -Wl,-soname,$@.so.1 ${CCFLAGS} -o ${LIBNAME} $^

# Library parts
sha1.o: sha1.cpp
	g++ -Wall -fPIC ${CCFLAGS} -c $^

# Make the library
libSHA256: sha256.o
	g++ -shared -Wl,-soname,$@.so.1 ${CCFLAGS} -o ${LIBNAME2} $^

# Library parts
sha256.o: sha256.cpp
	g++ -Wall -fPIC ${CCFLAGS} -c $^

# clear build files
clean:
	rm -rf *.o ${LIB}.* ${LIB2}.*
	rm -rf ${LIBDIR}/${LIB}.*
	rm -rf ${HEADER_DIR}

install: all install-libs install-headers

# Install the library to LIBPATH
install-libs: 
	@echo "[Installing Libs]"
	@if ( test ! -d $(PREFIX)/lib ) ; then mkdir -p $(PREFIX)/lib ; fi
	@install -m 0755 ${LIBNAME} ${LIBDIR}
	@ln -sf ${LIBDIR}/${LIBNAME} ${LIBDIR}/${LIB}.so.1
	@ln -sf ${LIBDIR}/${LIBNAME} ${LIBDIR}/${LIB}.so
	@install -m 0755 ${LIBNAME2} ${LIBDIR}
	@ln -sf ${LIBDIR}/${LIBNAME2} ${LIBDIR}/${LIB2}.so.1
	@ln -sf ${LIBDIR}/${LIBNAME2} ${LIBDIR}/${LIB2}.so
	@ldconfig

install-headers:
	@echo "[Installing Headers]"
	@if ( test ! -d ${HEADER_DIR} ) ; then mkdir -p ${HEADER_DIR} ; fi
	@install -m 0644 *1.h ${HEADER_DIR}
	@if ( test ! -d ${HEADER_DIR2} ) ; then mkdir -p ${HEADER_DIR2} ; fi
	@install -m 0644 *256.h ${HEADER_DIR2}
