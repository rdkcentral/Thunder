# - Try to find Intel os abstraction layer
# Once done this will define
#  INTELCE_OSAL_FOUND - System has Intel os abstraction layer
#  INTELCE_OSAL_INCLUDE_DIRS - The Intel os abstraction layer directories
#  INTELCE_OSAL_LIBRARIES - The Intel os abstraction layer libraries
#
# Copyright (C) 2015 Metrological.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(PkgConfig)

find_library(INTELCE_OSAL_LIB_DIR osal )

find_path(INTELCE_OSAL_INCLUDE_DIR osal.h
          PATH_SUFFIXES intelce)

find_path(INTELCE_OSAL_USER_INCLUDE_DIR os/osal_lib.h
          PATH_SUFFIXES linux_user intelce/linux_user)

set(INTELCE_OSAL_LIBRARIES ${INTELCE_OSAL_LIB_DIR})
set(INTELCE_OSAL_INCLUDE_DIRS ${INTELCE_OSAL_INCLUDE_DIR} ${INTELCE_OSAL_USER_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(INTELCE_OSAL DEFAULT_MSG INTELCE_OSAL_INCLUDE_DIRS INTELCE_OSAL_LIBRARIES)

mark_as_advanced(INTELCE_OSAL_INCLUDE_DIRS )
