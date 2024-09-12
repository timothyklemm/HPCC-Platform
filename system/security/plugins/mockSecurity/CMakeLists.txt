################################################################################
#    HPCC SYSTEMS software Copyright (C) 2024 HPCC Systems®.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
################################################################################


# Component: MockSecurity
#####################################################
# Description:
# ------------
#    Cmake Input File for MockSecurity
#####################################################

project( MockSecurity )

HPCC_ADD_SUBDIRECTORY (initfiles)

set (    SRCS
         ${HPCC_SOURCE_DIR}/system/security/shared/authmap.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/mockSecurity.cpp
    )

include_directories (
         ${HPCC_SOURCE_DIR}/system/include
         ${HPCC_SOURCE_DIR}/system/security/shared
         ${HPCC_SOURCE_DIR}/system/jlib
         ${HPCC_SOURCE_DIR}/esp/platform
         ${HPCC_SOURCE_DIR}/dali/base
    )

ADD_DEFINITIONS( -DMOCKSECURITY_EXPORTS -D_USRDLL )

HPCC_ADD_LIBRARY( mockSecurity SHARED ${SRCS} )
install ( TARGETS mockSecurity RUNTIME DESTINATION ${EXEC_DIR} LIBRARY DESTINATION ${LIB_DIR} )
target_link_libraries ( mockSecurity
         jlib
         dalibase
    )
