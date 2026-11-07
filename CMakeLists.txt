cmake_minimum_required(VERSION 3.20)
project(pes2017recomp CXX)

option(PES_ARMV82 "Build for ARMv8.2-A (LSE atomics, fp16, dotprod) on Android" ON)
if(ANDROID AND PES_ARMV82)
    string(APPEND CMAKE_C_FLAGS " -march=armv8.2-a+fp16+dotprod")
    string(APPEND CMAKE_CXX_FLAGS " -march=armv8.2-a+fp16+dotprod")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(ANDROID)
    set(PES17_USE_SDL OFF CACHE BOOL "Use SDL3 (not on Android)" FORCE)
endif()
option(PES17_USE_SDL "Use SDL3 for gamepad input (and later window/audio)" ON)
option(PES17_TRACE_INDIRECT "Report indirect calls with no recompiled target instead of jumping to null" ON)
option(PES17_LOGGING "Emit the runtime's log" ON)
option(PES17_PORTABLE "Link the C++ runtime statically, for a binary that runs on other machines" OFF)
option(PES17_DIAGNOSTICS "Build the traces, dumps, statistics, reports and test harness (runtime/diagnostics.h)" ON)

set(PES17_TITLE "sp")
set(PES17_VERSION "tu0")
set(PES17_PPC_DIR "${CMAKE_SOURCE_DIR}/ppc")

file(GLOB PPC_SOURCES CONFIGURE_DEPENDS "${PES17_PPC_DIR}/ppc_recomp.*.cpp")
list(APPEND PPC_SOURCES "${PES17_PPC_DIR}/ppc_func_mapping.cpp")

if(APPLE)
    link_libraries(iconv)
endif()

if(NOT PPC_SOURCES)
    message(FATAL_ERROR "${PES17_PPC_DIR} is empty -- run TITLE=${PES17_TITLE} VERSION=${PES17_VERSION} ./build.sh first to generate it")
endif()

add_library(ppc_recomp STATIC ${PPC_SOURCES})
set(SIMDE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/XenonRecomp/thirdparty/simde")
if(NOT EXISTS "${SIMDE_INCLUDE_DIR}/simde/simde-common.h" AND EXISTS "${CMAKE_SOURCE_DIR}/third_party/simde/simde/simde-common.h")
    set(SIMDE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/simde")
endif()
target_include_directories(ppc_recomp PUBLIC
    "${PES17_PPC_DIR}"
    "${SIMDE_INCLUDE_DIR}"
    "${SIMDE_INCLUDE_DIR}/simde")
target_compile_options(ppc_recomp PRIVATE -w)
target_compile_options(ppc_recomp PRIVATE -ffp-contract=off -fno-strict-aliasing)
target_compile_options(ppc_recomp PRIVATE -include "${CMAKE_SOURCE_DIR}/runtime/mmio_hook.h")

if(NOT PES17_LOGGING)
    add_compile_definitions(PES17_NO_LOGGING=1)
endif()

if(PES17_TRACE_INDIRECT)
    target_compile_definitions(ppc_recomp PUBLIC PES17_TRACE_INDIRECT=1)
endif()

file(GLOB RUNTIME_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/runtime/*.cpp"
     "${CMAKE_SOURCE_DIR}/runtime/kernel/*.cpp"
     "${CMAKE_SOURCE_DIR}/runtime/gpu/*.cpp"
     "${CMAKE_SOURCE_DIR}/runtime/apu/*.cpp"
     "${CMAKE_SOURCE_DIR}/runtime/install/*.cpp"
     "${CMAKE_SOURCE_DIR}/runtime/gpu/vulkan/*.cpp"
     "${CMAKE_SOURCE_DIR}/runtime/gpu/xe/*.cpp")

if(APPLE)
    include_directories(
        "${CMAKE_PREFIX_PATH}/include"
        "${CMAKE_BINARY_DIR}/include"
        "${CMAKE_SOURCE_DIR}/third_party/include"
        "${CMAKE_SOURCE_DIR}/deps/include"
        "/opt/homebrew/include"
        "/usr/local/include"
    )
endif()

set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/runtime/gpu/xe/interpreter.cpp"
    "${CMAKE_SOURCE_DIR}/runtime/gpu/xe/memexport.cpp"
    "${CMAKE_SOURCE_DIR}/runtime/gpu/xe/ucode.cpp"
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/runtime/gpu/xe/shim")

set(PES17_ONLINE "none" CACHE STRING "Online service: none, lan or steam")
set(PES17_ONLINE_BACKENDS "")
set(PES17_ONLINE_SOURCES "")
set(PES17_ONLINE_DEFINITIONS "")

add_executable(pes17 MACOSX_BUNDLE ${RUNTIME_SOURCES} "${CMAKE_SOURCE_DIR}/runtime/kernel/kernel.mm" "${CMAKE_SOURCE_DIR}/runtime/online/service.cpp")
target_compile_definitions(pes17 PRIVATE ${PES17_ONLINE_DEFINITIONS})

target_link_libraries(pes17 PRIVATE 
    ppc_recomp 
    "-framework AudioToolbox" 
    "-framework VideoToolbox" 
    "-framework CoreMedia"
    "-framework CoreVideo"
    "-framework UIKit"
    "-framework Foundation"
)

if(APPLE)
    target_link_options(pes17 PRIVATE "-Wl,-undefined,dynamic_lookup")
    
    # ATIVAÇÃO DIRETA DA PASTA PÚBLICA DE ARQUIVOS E ITUNES VIA PROPRIEDADES NATIVAS DO CMAKE
    set_target_properties(pes17 PROPERTIES
        MACOSX_BUNDLE_BUNDLE_IDENTIFIER "com.community.pes2017recomp"
        MACOSX_BUNDLE_BUNDLE_NAME "PES17-Launcher"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.community.pes2017recomp"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/runtime/Info.plist"
    )
endif()

target_include_directories(pes17 PRIVATE "${CMAKE_SOURCE_DIR}/runtime")
