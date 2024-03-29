# Copyright (c) 2019 Manuel Bouyer
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.1.1)

PROJECT(wxbm)

SET(CMAKE_VERBOSE_MAKEFILE ON)
message(STATUS "cmake version: ${CMAKE_VERSION}")

SET(CMAKE_BUILD_TYPE Debug)

SET(PACKAGE_NAME wxbm)
SET(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/build
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_SOURCE_DIR}/cmake
)


set (CMAKE_CXX_STANDARD 11)

SET(PKGINC /usr/pkg/include)
SET(PKGLIB /usr/pkg/lib)

INCLUDE_DIRECTORIES(${PKGINC})
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${PKGLIB} -Wl,-R${PKGLIB}")

SET(MPDIR ${CMAKE_SOURCE_DIR}/wxmathplot/mathplot)
INCLUDE_DIRECTORIES(${MPDIR})

MESSAGE (STATUS "*** Will install to ${CMAKE_INSTALL_PREFIX}  ***")
SET(PREFIX_BIN bin)
SET(PREFIX_INCLUDE include)
SET(PREFIX_DATA share)
SET(PREFIX_PKGDATA ${PREFIX_DATA}/${PACKAGE_NAME})
SET(PREFIX_LIB "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

#FIND_PACKAGE(GTK2)
IF(GTK2_FOUND)
  set(wxWidgets_CONFIG_OPTIONS ${wxWidgets_CONFIG_OPTIONS} --toolkit=gtk2)
  INCLUDE_DIRECTORIES(${GTK2_INCLUDE_DIRS})
  SET(GTK_LIBRARIES ${GTK2_LIBRARIES})
  MESSAGE(STATUS "Building against GTK2...")
ELSE(GTK2_FOUND)
  FIND_PACKAGE(GTK3)
  INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
  SET(GTK_LIBRARIES ${GTK3_LIBRARIES})
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__WXGTK3__")
  set(wxWidgets_CONFIG_OPTIONS ${wxWidgets_CONFIG_OPTIONS} --toolkit=gtk3)
  MESSAGE(STATUS "Building against GTK3...")
ENDIF(GTK2_FOUND)
FIND_PACKAGE(wxWidgets REQUIRED propgrid)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DwxDEBUG_LEVEL=0")

MESSAGE (STATUS "Found wxWidgets..." )
MESSAGE (STATUS " wxWidgets Include: ${wxWidgets_INCLUDE_DIRS}")
MESSAGE (STATUS " wxWidgets Libraries: ${wxWidgets_LIBRARIES}")

INCLUDE(${wxWidgets_USE_FILE})

INCLUDE_DIRECTORIES(${wxWidgets_INCLUDE_DIRS})

SET(N2KHDRS
    N2K/NMEA2000.h
    N2K/NMEA2000Properties.h
    N2K/NMEA2000PropertiesDialog.h
    N2K/nmea2000_frame.h
    N2K/nmea2000_defs.h
    N2K/nmea2000_defs_rx.h
    N2K/nmea2000_defs_tx.h
)

SET(MPHDRS
    ${MPDIR}/mathplot.h
)

SET(HDRS
    wxbm.h
    bmstatus.h
    bmlog.h
    bmlogstorage.h
    bmmathplot.h
    ${N2KHDRS}
    ${MPHDRS}
)

SET(N2KSRCS
    N2K/NMEA2000.cpp
    N2K/NMEA2000Properties.cpp
    N2K/NMEA2000PropertiesDialog.cpp
    N2K/nmea2000_rxtx.cpp
    N2K/nmea2000_energy_rx.cpp
    N2K/nmea2000_log.cpp
)

SET(MPSRCS
    ${MPDIR}/mathplot.cpp
)

SET(SRCS
    main.cpp
    bmstatus.cpp
    bmlog.cpp
    bmlogstorage.cpp
    bmmathplot.cpp
    ${N2KSRCS}
    ${MPSRCS}
)

SET( PACKAGE_HEADERS "" )
ADD_EXECUTABLE(${PACKAGE_NAME} ${HDRS} ${SRCS})
TARGET_LINK_LIBRARIES(${PACKAGE_NAME} ${GTK_LIBRARIES} ${wxWidgets_LIBRARIES})
