cmake_minimum_required(VERSION 3.1)
project(bfm_decoder CXX)


add_executable(nexdecode cli/nexdecode.cpp src/csi_reader_func.cpp src/csi_reader.cpp)
target_compile_options(nexdecode PUBLIC -O2 -Wall -std=c++17)
if(UNIX AND NOT APPLE)
  add_executable(nexlive cli/nexlive.cpp src/csi_reader_func.cpp src/csi_capture.cpp src/csi_realtime_graph.cpp)
  target_compile_options(nexlive PUBLIC -O2 -Wall -std=c++17)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_COMPILER g++)

# pcapplusplus
set(PCAPPP_INCLUDES /usr/local/include/pcapplusplus include/cmdline src)
set(PCAPPP_LIBS_DIR /usr/local/lib)
set(PCAPPP_LIBS Pcap++ Packet++ Common++ pcap pthread)
set(PCAPPP_ENABLE_CPP_FEATURE_DETECTION ON)

if(APPLE)
  message(STATUS "OS: APPLE")
elseif(UNIX)
  message(STATUS "OS: UNIX")
  set(PCAPPP_LIBS ${PCAPPP_LIBS} stdc++fs)
endif()

include_directories(${PCAPPP_INCLUDES})
link_directories(${PCAPPP_LIBS_DIR})

if(APPLE)
  target_link_libraries(nexdecode ${PCAPPP_LIBS})
  install(TARGETS nexdecode RUNTIME DESTINATION /usr/local/bin)
elseif(UNIX)
  target_link_libraries(nexdecode ${PCAPPP_LIBS})
  target_link_libraries(nexlive ${PCAPPP_LIBS})
  install(TARGETS nexdecode nexlive RUNTIME DESTINATION /usr/local/bin)
endif()

