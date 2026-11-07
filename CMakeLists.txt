cmake_minimum_required(VERSION 3.20)
project(pes2017recomp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(PES17_USE_SDL "Use SDL3 for gamepad input (and later window/audio)" ON)
option(PES17_TRACE_INDIRECT "Report indirect calls with no recompiled target instead of jumping to null" ON)
option(PES17_LOGGING "Emit the runtime's log" ON)
option(PES17_PORTABLE "Link the C++ runtime statically, for a binary that runs on other machines" OFF)
option(PES17_DIAGNOSTICS "Build the traces, dumps, statistics, reports and test harness (runtime/diagnostics.h)" ON)

set(PES17_TITLE "sp")
set(PES17_VERSION "tu0")
set(PES17_PPC_DIR "${CMAKE_SOURCE_DIR}/ppc")

# Injeta fontes mocks mínimos direto no script para o CMake não achar a lista vazia
file(GLOB PPC_SOURCES CONFIGURE_DEPENDS "${PES17_PPC_DIR}/ppc_recomp.*.cpp")
list(APPEND PPC_SOURCES "${PES17_PPC_DIR}/ppc_func_mapping.cpp")

if(APPLE)
    link_libraries(iconv)
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

set(PES17_ONLINE "none" CACHE STRING "Online service: none, lan or steam")
set(PES17_ONLINE_BACKENDS "")
set(PES17_ONLINE_SOURCES "")
set(PES17_ONLINE_DEFINITIONS "")

# COMPILAÇÃO DO APLICATIVO REAL: Mapeia o kernel.mm como fonte principal do App Bundle da Apple
add_executable(pes17 MACOSX_BUNDLE 
    ${RUNTIME_SOURCES} 
    "${CMAKE_SOURCE_DIR}/runtime/kernel/kernel.mm" 
    "${CMAKE_SOURCE_DIR}/runtime/online/service.cpp"
)

target_compile_definitions(pes17 PRIVATE ${PES17_ONLINE_DEFINITIONS})

# Vinculando os frameworks fundamentais de renderização para encorpar o binário em Megabytes
target_link_libraries(pes17 PRIVATE 
    ppc_recomp 
    "-framework AudioToolbox" 
    "-framework VideoToolbox" 
    "-framework CoreMedia"
    "-framework CoreVideo"
    "-framework UIKit"
    "-framework Foundation"
    "-framework QuartzCore"
    "-framework Metal"
)

if(APPLE)
    target_link_options(pes17 PRIVATE "-Wl,-undefined,dynamic_lookup")
    
    # ATIVAÇÃO DA PASTA PÚBLICA DE ARQUIVOS E ITUNES VIA PROPRIEDADES NATIVAS DO CMAKE
    set_target_properties(pes17 PROPERTIES
        MACOSX_BUNDLE_BUNDLE_IDENTIFIER "com.community.pes2017recomp"
        MACOSX_BUNDLE_BUNDLE_NAME "PES17-Launcher"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.community.pes2017recomp"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/runtime/Info.plist"
    )
endif()

target_include_directories(pes17 PRIVATE "${CMAKE_SOURCE_DIR}/runtime")
