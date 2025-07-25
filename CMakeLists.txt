cmake_minimum_required(VERSION 3.16)

project(FlexRIC VERSION 2.0.0 LANGUAGES C)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION MATCHES "^11.*")
  message(FATAL_ERROR "\
  FlexRIC doesn't support gcc-11.
  We use C11 _Generics and nested functions (e.g. defer with ASan), which gcc-11 does not support.
  Please change gcc version to gcc-10 or gcc-12 or gcc-13.
  Follow the steps indicated in section 1.1.1 of general README.")
endif()

set(CMAKE_C_STANDARD 11)

#######
## Reduce compile time
#######

# use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "Found ccache in ${CCACHE_PROGRAM}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
  message(STATUS "Ccache not found, please consider installing it for faster compilation running the command: sudo apt install -y ccache")
endif()

#Unity build
set(UNITY_BUILD ON)
set(UNITY_BUILD_BATCH_SIZE 10)
# cmake -GNinja -DCMAKE_UNITY_BUILD=ON -DCMAKE_CXX_COMPILER=clang++-DCMAKE_CXX_FLAGS="-fuse-ld=lld -ftime-trace"

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON )
# For YouCompleteMe configuration
file(CREATE_LINK
  "${CMAKE_BINARY_DIR}/compile_commands.json"
  "${CMAKE_SOURCE_DIR}/compile_commands.json"
  SYMBOLIC
)

#######
## Scan build
#######

# mkdir build && cd build && scan-build cmake .. && scan-build make -j8 && scan-view /tmp/scan-buildXXXXX-1

set(default_build_type "Debug")

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

#######
## SANITIZER
#######

#https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110157

set(SANITIZER "NONE" CACHE STRING "Sanitizers")
set_property(CACHE SANITIZER PROPERTY STRINGS "NONE" "ADDRESS" "THREAD")
message(STATUS "Selected SANITIZER TYPE: ${SANITIZER}")

if(${SANITIZER} STREQUAL "ADDRESS")
   add_compile_options("-fno-omit-frame-pointer;-fsanitize=address")
   add_link_options("-fsanitize=address")
elseif(${SANITIZER} STREQUAL "THREAD")
  add_compile_options("-fsanitize=thread;-g;")
  add_link_options("-fsanitize=thread;")
elseif(${SANITIZER} STREQUAL "STACK")
  add_compile_options("-fstack-protector-all")
  add_link_options("-fstack-protector-all")
endif()

#######
## Clang compiler
#######

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  # using Clang
  find_library(BLOCKS_RUNTIME_LIBRARY BlocksRuntime)
  if(NOT BLOCKS_RUNTIME_LIBRARY)
    message(FATAL_ERROR "\
    BlocksRuntime library not found.
    Installation on ubuntu: sudo apt-get install libblocksruntime-dev
    Installation on fedora: sudo dnf install libblocksruntime-devel")
  endif()
  add_link_options("-lBlocksRuntime")
endif()

set(BUILDING_LIBRARY "STATIC" CACHE STRING "Static or dynamic library")
set_property(CACHE BUILDING_LIBRARY PROPERTY STRINGS "STATIC" "DYNAMIC")
message(STATUS "Selected LIBRARY TYPE: ${BUILDING_LIBRARY}")

#######
## CppCheck
#######

option(CPP_CHECK "CppCheck" OFF)
if(CPP_CHECK)
  set(CMAKE_C_CPPCHECK "cppcheck")
endif()

#######
## Clang Tidy
#######

option(Clang_Tidy "Clang Tidy" OFF)
if(Clang_Tidy)
  set(CMAKE_C_CLANG_TIDY clang-tidy -checks=-*,readability-*)
endif()

#######
## Number of nearRT-RIC threads
#######

set(NUM_THREADS_RIC "2" CACHE STRING "Number of threads in the RIC's task manager")

#######
## xApp
#######

set(XAPP_DB "SQLITE3_XAPP" CACHE STRING "xApp DB")
set_property(CACHE XAPP_DB PROPERTY STRINGS "SQLITE3_XAPP" "NONE_XAPP")
message(STATUS "Selected xApp DB : ${XAPP_DB}")

set(XAPP_DB_DIR "/tmp/" CACHE STRING "The xApp DB write directory")
if(NOT XAPP_DB_DIR  MATCHES "/$")
  message(FATAL_ERROR "xApp directory path needs to terminate in one / char")
endif()

if(NOT EXISTS ${XAPP_DB_DIR})
  message(FATAL_ERROR "Selected XAPP_DB_DIR does not exist. Please create it or change path")
endif()

message(STATUS "Selected xApp dir : ${XAPP_DB_DIR}")

option(XAPP_MULTILANGUAGE "xApp Multilanguage" OFF)
message(STATUS "xApp multilanguage support e.g., python : ${XAPP_MULTILANGUAGE}")

#######
## Coverage
#######

option(CODE_COVERAGE "Code coverage" OFF)
if(CODE_COVERAGE)
  add_compile_options("-fprofile-arcs;-ftest-coverage")
  add_link_options("-lgcov;-coverage")
  message(STATUS "Code Coverage ON. Example usage: lcov --capture --directory . --output-file coverage.info && genhtml coverage.info --output-directory out && cd out && firefox index.html")
  message(STATUS "To merge different coverages: lcov --add-tracefile coverage1.info -a coverage2.info ...coverageN -o merged.info")

endif()

#######
## Profiler
#######

option(CODE_PROFILER "Code Profiler" OFF)
if( CODE_PROFILER )
  add_compile_options("-pg")
  add_link_options("-pg")
  message(STATUS "Code Profiler ON. Example usage: gprof test/test_near_ric gmon.out > prof.txt && vim prof.txt")
endif()

#######
## Include What You Use (IWYU)
#######

option(INCL_WHAT_YOU_USE "Include what you use tool: iwyu" OFF)
if(INCL_WHAT_YOU_USE)
  message(STATUS "IWYU: Include what you use ON")
endif()

#option(IAPP_EMULATOR "iApp emulator" OFF)
#if(IAPP_EMULATOR)
#  message(STATUS "iApp emulator ON")
#endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(BUILDING_LIBRARY STREQUAL "STATIC")
  add_compile_options("-W;-Wall;-Wextra;-g;-Wno-unused-result;-Warray-bounds;-Wempty-body;-fstack-protector-strong;-fstack-clash-protection;-D_GLIBCXX_ASSERTIONS") #-fopt-info -fhardened
elseif(BUILDING_LIBRARY STREQUAL "DYNAMIC")
  # -fPIC flag
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  add_compile_options("-W;-Wall;-Wextra;-g")
else()
  message(FATAL_ERROR "Unknown building type. Either choose a static or a dynamic library")
endif()

#######
## E2AP encoding and versioning
#######

set(E2AP_ENCODING "ASN" CACHE STRING "The E2AP encoding to use")
set_property(CACHE E2AP_ENCODING PROPERTY STRINGS "ASN" "FLATBUFFERS")
message(STATUS "Selected E2AP_ENCODING: ${E2AP_ENCODING}")

set(E2AP_VERSION "E2AP_V2" CACHE STRING "E2AP version")
set_property(CACHE E2AP_VERSION PROPERTY STRINGS "E2AP_V1" "E2AP_V2" "E2AP_V3")
message(STATUS "Selected E2AP_VERSION: ${E2AP_VERSION}")

if(E2AP_VERSION STREQUAL "E2AP_V1")
#  add_compile_definitions(E2AP_V1)
  add_library(e2ap_ran_func_obj OBJECT
              src/lib/e2ap/v1_01/e2ap_types/common/e2ap_ran_function.c )
  add_library(e2ap_plmn_obj OBJECT
              src/lib/e2ap/v1_01/e2ap_types/common/e2ap_plmn.c )
  add_library(e2ap_global_node_id_obj OBJECT
              src/lib/e2ap/v1_01/e2ap_types/common/e2ap_global_node_id.c )
elseif(E2AP_VERSION STREQUAL "E2AP_V2")
#  add_compile_definitions(E2AP_V2)
  add_library(e2ap_ran_func_obj OBJECT
              src/lib/e2ap/v2_03/e2ap_types/common/e2ap_ran_function.c )
  add_library(e2ap_plmn_obj OBJECT
              src/lib/e2ap/v2_03/e2ap_types/common/e2ap_plmn.c )
  add_library(e2ap_global_node_id_obj OBJECT
              src/lib/e2ap/v2_03/e2ap_types/common/e2ap_global_node_id.c )
elseif(E2AP_VERSION STREQUAL "E2AP_V3")
#  add_compile_definitions(E2AP_V3)
  add_library(e2ap_ran_func_obj OBJECT
              src/lib/e2ap/v3_01/e2ap_types/common/e2ap_ran_function.c )
  add_library(e2ap_plmn_obj OBJECT
              src/lib/e2ap/v3_01/e2ap_types/common/e2ap_plmn.c )
  add_library(e2ap_global_node_id_obj OBJECT
              src/lib/e2ap/v3_01/e2ap_types/common/e2ap_global_node_id.c )
else()
  message(FATAL_ERROR "E2AP Unknown version selected")
endif()

target_compile_options(e2ap_ran_func_obj PRIVATE -fPIC -fvisibility=hidden)

#######
## Service Models encoding and versioning
#######

add_definitions(-DSERVICE_MODEL_DIR_PATH="${SM_DIR_PATH}/")

# KPM SM
set(SM_ENCODING_KPM "ASN" CACHE STRING "The KPM SM encoding to use")
set_property(CACHE SM_ENCODING_KPM PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected KPM SM_ENCODING: ${SM_ENCODING_KPM}")

set(KPM_VERSION "KPM_V2_03" CACHE STRING "The KPM SM version to use")
# add_compile_definitions(KPM_V2_03)
set_property(CACHE SM_ENCODING_KPM PROPERTY STRINGS  "KPM_V2_01" "KPM_V2_03" "KPM_V3_00")
message(STATUS "Selected KPM Version: ${KPM_VERSION}")

# RC SM
set(SM_ENCODING_RC "ASN" CACHE STRING "The RC SM encoding to use")
set_property(CACHE SM_ENCODING_RC PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected RC SM_ENCODING: ${SM_ENCODING_RC}")

# MAC SM
set(SM_ENCODING_MAC "PLAIN" CACHE STRING "The MAC SM encoding to use")
set_property(CACHE SM_ENCODING_MAC PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected MAC SM_ENCODING: ${SM_ENCODING_MAC}")

# RLC SM
set(SM_ENCODING_RLC "PLAIN" CACHE STRING "The RLC SM encoding to use")
set_property(CACHE SM_ENCODING_RLC PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected RLC SM_ENCODING: ${SM_ENCODING_RLC}")

# PDCP SM
set(SM_ENCODING_PDCP "PLAIN" CACHE STRING "The PDCP SM encoding to use")
set_property(CACHE SM_ENCODING_PDCP PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected PDCP SM_ENCODING: ${SM_ENCODING_PDCP}")

# SLICE SM
set(SM_ENCODING_SLICE "PLAIN" CACHE STRING "The SLICE SM encoding to use")
set_property(CACHE SM_ENCODING_SLICE PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected SLICE SM_ENCODING: ${SM_ENCODING_SLICE}")

# GTP SM
set(SM_ENCODING_GTP "PLAIN" CACHE STRING "The GTP SM encoding to use")
set_property(CACHE SM_ENCODING_GTP PROPERTY STRINGS "PLAIN")
message(STATUS "Selected GTP SM_ENCODING: ${SM_ENCODING_GTP}")

#######
## Flatbuffer encoding
#######

set(FlatCC_INCLUDE_DIR "" CACHE STRING "The Flatbuffers include directory")
set(FlatCC_LIB_DIR "" CACHE STRING "The Flatbuffers lib directory")

if(E2AP_ENCODING STREQUAL "FLATBUFFERS")
  find_library(FlatCC
    NAMES flatccrt_d
    HINTS ${FlatCC_LIB_DIR}
  )
endif()

#######
## Source code
#######

include_directories(src)
add_subdirectory(src)
add_subdirectory(examples)

#######
## Unit testing
#######

set(UNIT_TEST "TRUE" CACHE STRING "Build unit tests")
set_property(CACHE UNIT_TEST PROPERTY STRINGS "TRUE" "FALSE")
if (UNIT_TEST)
  add_subdirectory(test)
  set(CTEST_PARALLEL_LEVEL 4)
  enable_testing()
endif ()

#######
## Install the default configuration file
#######

message(STATUS "install prefix path ${CMAKE_INSTALL_PREFIX}/flexric")
include(GNUInstallDirs)
install(FILES flexric.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/flexric)

#######
## Install the Service Models
#######

install(TARGETS mac_sm DESTINATION ${CMAKE_INSTALL_LIBDIR}/flexric)
install(TARGETS rlc_sm DESTINATION ${CMAKE_INSTALL_LIBDIR}/flexric)
install(TARGETS pdcp_sm DESTINATION ${CMAKE_INSTALL_LIBDIR}/flexric)
install(TARGETS slice_sm DESTINATION ${CMAKE_INSTALL_LIBDIR}/flexric)
install(TARGETS tc_sm DESTINATION ${CMAKE_INSTALL_LIBDIR}/flexric)
install(TARGETS gtp_sm DESTINATION ${CMAKE_INSTALL_LIBDIR}/flexric)
install(TARGETS kpm_sm DESTINATION ${CMAKE_INSTALL_LIBDIR}/flexric)
install(TARGETS rc_sm DESTINATION ${CMAKE_INSTALL_LIBDIR}/flexric)

#######
## Install globally nearRT-RIC
#######

set(NEAR_RIC_INSTALL "FALSE" CACHE STRING "nearRT-RIC installation")
set_property(CACHE NEAR_RIC_INSTALL PROPERTY STRINGS "TRUE" "FALSE")
if (NEAR_RIC_INSTALL)
  install(TARGETS nearRT-RIC DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

#######
## Install globally E2 agent emulators
#######

set(EMU_AGENT_INSTALL "FALSE" CACHE STRING "E2 agent emulators installation")
set_property(CACHE EMU_AGENT_INSTALL PROPERTY STRINGS "TRUE" "FALSE")
if (EMU_AGENT_INSTALL)
  install(TARGETS emu_agent_gnb emu_agent_gnb_cu emu_agent_gnb_du emu_agent_enb DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

#######
## Install globally xApps in C
#######

set(XAPP_C_INSTALL "FALSE" CACHE STRING "xApps in C installation")
set_property(CACHE XAPP_C_INSTALL PROPERTY STRINGS "TRUE" "FALSE")
if (XAPP_C_INSTALL)
  install(TARGETS xapp_mac_rlc_pdcp_moni xapp_slice_moni_ctrl xapp_kpm_moni xapp_gtp_moni xapp_hw DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

#######
## Uninstall the Service Models and the configuration file
#######
add_custom_target("uninstall" COMMENT "Uninstall installed files")
add_custom_command(
        TARGET "uninstall"
        POST_BUILD
        COMMENT "Uninstall files with install_manifest.txt"
        COMMAND xargs rm -vf < install_manifest.txt || echo Nothing in
        install_manifest.txt to be uninstalled!
)
