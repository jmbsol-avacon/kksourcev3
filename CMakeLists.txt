#
# Kwik Kuote - Distributor Information System
#
# Modern build of the 1994-2000 Borland C++ 3.1 / MS-DOS application.
#
#   kk        the application (Distributor variant by default)
#   makecfg   creates a default DB.CFG configuration file
#   kkdriver  creates the default printer driver files (*.DRV)
#
# Requirements: CMake 3.16+, a C11 and C++17 compiler (GCC, Clang, MSVC,
# MinGW).  See docs/PORTING.md for the design of the platform layer and for
# the Windows / Linux build instructions.
#
cmake_minimum_required(VERSION 3.16)
project(KwikKuote VERSION 3.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(KK_REP_VERSION   "Build the Representative variant (REP_VER) instead of the Distributor" OFF)
option(KK_DEBUG_ASSERTS "Enable the legacy Assert()/ErrorLog diagnostics (DEBUG)" OFF)

set(KK_SRC    ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(KK_COMPAT ${KK_SRC}/compat)
set(KK_TOOLS  ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# ---------------------------------------------------------------------------
# Platform layer: Borland/DOS run-time on top of the host operating system.
# ---------------------------------------------------------------------------
add_library(kkcompat STATIC
  ${KK_COMPAT}/kk_console.cpp
  ${KK_COMPAT}/kk_keys.cpp
  ${KK_COMPAT}/kk_term_posix.cpp
  ${KK_COMPAT}/kk_term_win32.cpp
  ${KK_COMPAT}/kk_cp437.c
  ${KK_COMPAT}/kk_dos.c
  ${KK_COMPAT}/kk_stdio.c
)
target_include_directories(kkcompat PUBLIC ${KK_COMPAT}/include)
target_include_directories(kkcompat PRIVATE ${KK_COMPAT})
if(NOT WIN32)
  # Borland headers that the Windows C run-time already provides.
  target_include_directories(kkcompat PUBLIC ${KK_COMPAT}/include_posix)
endif()
if(WIN32)
  target_link_libraries(kkcompat PUBLIC user32)
endif()
if(MSVC)
  target_compile_options(kkcompat PRIVATE /W3)
  target_compile_definitions(kkcompat PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
  target_compile_options(kkcompat PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Compile settings shared by the legacy C sources.
# ---------------------------------------------------------------------------
include(CheckCCompilerFlag)

function(kk_legacy_target name)
  target_link_libraries(${name} PRIVATE kkcompat)
  target_include_directories(${name} PRIVATE ${KK_SRC})
  if(KK_REP_VERSION)
    target_compile_definitions(${name} PRIVATE REP_VER)
  endif()
  if(KK_DEBUG_ASSERTS)
    target_compile_definitions(${name} PRIVATE DEBUG=1)
  endif()

  if(MSVC)
    # Force-include the compatibility header; the sources and their string
    # literals are in code page 437 and must stay that way in the binary.
    target_compile_options(${name} PRIVATE
      /FI${KK_COMPAT}/include/kk_compat.h
      /source-charset:.437 /execution-charset:.437
      /W3 /wd4996 /wd4018 /wd4244 /wd4267 /wd4305 /wd4113 /wd4028 /wd4047 /wd4024
      /wd4067 /wd4090 /wd4129 /wd4133)
  else()
    target_compile_options(${name} PRIVATE -include ${KK_COMPAT}/include/kk_compat.h)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
      check_c_compiler_flag("-finput-charset=CP437" KK_HAS_CP437_CHARSET)
      if(KK_HAS_CP437_CHARSET)
        target_compile_options(${name} PRIVATE -finput-charset=CP437 -fexec-charset=CP437)
      endif()
      # 1994 C: K&R-era pointer conversions are errors in GCC 14 unless relaxed.
      target_compile_options(${name} PRIVATE
        -Wno-error=incompatible-pointer-types
        -Wno-error=implicit-function-declaration
        -Wno-error=int-conversion)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
      target_compile_options(${name} PRIVATE
        -Wno-invalid-source-encoding
        -Wno-error=incompatible-pointer-types
        -Wno-error=implicit-function-declaration
        -Wno-error=int-conversion)
    endif()
    target_compile_options(${name} PRIVATE
      -Wall
      -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-value
      -Wno-format-security -Wno-parentheses -Wno-misleading-indentation
      -Wno-char-subscripts -Wno-pointer-sign -Wno-sign-compare)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# The application.  (VMOUSE.C of the original is replaced by the platform
# layer; REP_RPTS.C duplicates reports in DIS_RPTS.C and was never linked.)
# ---------------------------------------------------------------------------
set(KK_LEGACY_SOURCES
  check.c db.c db1.c db_defs.c db_fax.c db_help.c db_inpt.c db_misc.c
  db_print.c db_rpt.c db_sys.c dis_rpts.c editor.c editor1.c file.c input.c
  jb_inst.c kkuote.c kk_more.c menu.c output.c screen.c sort.c s_file.c
  vfile.c vmisc.c
)
list(TRANSFORM KK_LEGACY_SOURCES PREPEND ${KK_SRC}/)

add_executable(kk ${KK_LEGACY_SOURCES})
kk_legacy_target(kk)

# ---------------------------------------------------------------------------
# Utilities.
# ---------------------------------------------------------------------------
add_executable(makecfg ${KK_TOOLS}/makecfg.c)
kk_legacy_target(makecfg)

add_executable(kkdriver ${KK_TOOLS}/driver.c)
kk_legacy_target(kkdriver)

install(TARGETS kk makecfg kkdriver RUNTIME DESTINATION bin)
