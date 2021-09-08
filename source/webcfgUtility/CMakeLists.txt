#   Copyright 2021 Comcast Cable Communications Management, LLC
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

set(PROJ_WEBCFGUTIL webcfgUtil)

file(GLOB HEADERS webcfg_util.h)
set(SOURCES webcfg_util.c)

add_library(${PROJ_WEBCFGUTIL} STATIC ${HEADERS} ${SOURCES})
add_library(${PROJ_WEBCFGUTIL}.shared SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJ_WEBCFGUTIL}.shared PROPERTIES OUTPUT_NAME ${PROJ_WEBCFGUTIL})

install (TARGETS ${PROJ_WEBCFGUTIL} DESTINATION lib${LIB_SUFFIX})
install (TARGETS ${PROJ_WEBCFGUTIL}.shared DESTINATION lib${LIB_SUFFIX})
install (FILES webcfg_util.h DESTINATION include/${PROJ_WEBCFGUTIL})
