cmake_minimum_required(VERSION 3.1)
project(lab_intern_2022 CXX)

#  cli app
# set(EXE_NAME labint)
# add_executable(${EXE_NAME} cli/main.cpp src/csi_reader_func.cpp src/csi_reader.cpp)

#  develop
set(EXE_NAME a.out)
add_executable(${EXE_NAME} src/sample.cpp src/csi_reader_func.cpp src/csi_capture.cpp)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_COMPILER g++)

set(PCAPPP_INCLUDES /usr/local/include/pcapplusplus include/cmdline src)
set(PCAPPP_LIBS_DIR /usr/local/lib)
set(PCAPPP_LIBS Pcap++ Packet++ Common++ pcap pthread)

target_compile_options(${EXE_NAME} PUBLIC -O2 -Wall -std=c++2a)
set(PCAPPP_ENABLE_CPP_FEATURE_DETECTION ON)

include_directories(${PCAPPP_INCLUDES})
link_directories(${PCAPPP_LIBS_DIR})
target_link_libraries(${EXE_NAME} ${PCAPPP_LIBS})

install(TARGETS ${EXE_NAME} RUNTIME DESTINATION /usr/local/bin)
