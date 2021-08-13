cmake_minimum_required(VERSION 3.12)
project(spine)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_definitions(-DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1 -DLINUX)
set (ZENOH_TRANSPORT_UDP "ON")

set(ZENOHC /home/lieven/workspace/zenoh-c)
set(LIMERO /home/lieven/workspace/limero )
set(TINYCBOR ../tinycbor )
set(ETL ../etl )
set(COMMON ../common )


include_directories(
  src 
  ${ZENOHC}/include
  ${COMMON}
  ${TINYCBOR}/src
  ${ETL}/include
/home/lieven/workspace/ArduinoJson/src )

add_definitions(-DCATCH_CONFIG_DISABLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -g -DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1")
# set(CMAKE_BUILD_TYPE Debug)

set (CMAKE_CXX_STANDARD 11)
 
# set up our program
add_executable(zenoh-proxy)
add_executable(brain)
add_executable(test)
add_executable(udp-client)


target_sources(test PRIVATE 
src/limero.cpp
src/test.cpp 
  src/logger.cpp
  src/Sys.cpp
  ${COMMON}/CborSerializer.cpp
  ${COMMON}/CborDeserializer.cpp
  ${COMMON}/CborDump.cpp
 ) 

target_sources(zenoh-proxy PRIVATE 
  src/proxy.cpp 
  src/config.cpp 
  src/SessionSerial.cpp
  src/serial.cpp 
  src/SessionUdp.cpp
  src/Udp.cpp 
  src/BrokerZenoh.cpp
  src/logger.cpp
  src/Sys.cpp
  src/limero.cpp
  ${COMMON}/ppp_frame.cpp 
  ${COMMON}/CborSerializer.cpp
  ${COMMON}/CborDeserializer.cpp
  ${COMMON}/CborDump.cpp
  ${COMMON}/Frame.cpp) 

target_sources(brain PRIVATE
src/Sys.cpp
src/limero.cpp
src/logger.cpp
${COMMON}/CborSerializer.cpp
${COMMON}/CborDeserializer.cpp
src/BrokerZenoh.cpp
${COMMON}/CborDump.cpp
src/brain.cpp
)

target_sources(udp-client PRIVATE
src/Sys.cpp
src/limero.cpp
src/logger.cpp
${COMMON}/CborSerializer.cpp
${COMMON}/CborDeserializer.cpp
${COMMON}/CborDump.cpp
src/udp_client.cpp
src/Udp.cpp
)

target_link_libraries(zenoh-proxy 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-L/usr/local/lib/ -lzenohc
-lpthread  
-lrt  
-latomic)

target_link_libraries(test 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
gtest_main
-lpthread  
-lrt  
-latomic)

target_link_libraries(udp-client 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
gtest_main
-lpthread  
-lrt  
-latomic)

target_link_libraries(brain 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-L/usr/local/lib/ -lzenohc
-lpthread  
-lrt  
-latomic)

include(GoogleTest)
gtest_discover_tests(test)
