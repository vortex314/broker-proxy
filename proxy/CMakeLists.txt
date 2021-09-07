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

# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
# target_include_directories(target PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
# target_link_libraries(target ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
find_path(REDIS_PLUS_PLUS_HEADER sw)
# target_include_directories(target PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
# target_link_libraries(target ${REDIS_PLUS_PLUS_LIB})

add_definitions(-DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1 -DLINUX -std=c++11)
# set (ZENOH_TRANSPORT_UDP "ON")

# set(ZENOHC /home/lieven/workspace/zenoh-c)
set(HIREDIS ../hiredis )
set(LIMERO /home/lieven/workspace/limero )
set(TINYCBOR ../tinycbor )
set(ETL ../etl )
set(COMMON ../common )


include_directories(
  src 
  /usr/local/include
#  ${ZENOHC}/include
  ${COMMON}
  ${TINYCBOR}/src
  ${ETL}/include
  ${HIREDIS}
/home/lieven/workspace/ArduinoJson/src )

add_definitions(-DCATCH_CONFIG_DISABLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -g -DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1")
# set(CMAKE_BUILD_TYPE Debug)

set (CMAKE_CXX_STANDARD 11)
 
# set up our program
# add_executable(zenoh-proxy)
add_executable(redis-proxy)

add_executable(redis-brain)
# add_executable(zenoh-brain)

add_executable(test)
add_executable(redis-test)
add_executable(udp-client)
add_executable(dtr_rts)

target_compile_options(redis-proxy PRIVATE "-DBROKER_REDIS")
# target_compile_options(zenoh-proxy PRIVATE "-DBROKER_ZENOH")
target_compile_options(redis-brain PRIVATE "-DBROKER_REDIS")
# target_compile_options(zenoh-brain PRIVATE "-DBROKER_ZENOH")


target_sources(dtr_rts PRIVATE 
  src/DtrRts.cpp
  src/logger.cpp
  src/Sys.cpp
 )

target_sources(test PRIVATE 
  src/limero.cpp
  src/test.cpp 
  src/logger.cpp
  src/Sys.cpp
  ${COMMON}/CborSerializer.cpp
  ${COMMON}/CborDeserializer.cpp
  ${COMMON}/CborDump.cpp
 ) 

 target_sources(redis-test PRIVATE 
  src/limero.cpp
  src/redis-test.cpp 
  src/logger.cpp
  src/Sys.cpp
  ${COMMON}/CborSerializer.cpp
  ${COMMON}/CborDeserializer.cpp
  ${COMMON}/CborDump.cpp
 ) 

  target_sources(redis-proxy PRIVATE 
  src/proxy.cpp 
  src/config.cpp 
  src/SessionSerial.cpp
  src/serial.cpp 
  src/SessionUdp.cpp
  src/Udp.cpp 
  src/BrokerRedis.cpp
  src/logger.cpp
  src/Sys.cpp
  src/limero.cpp
  ${COMMON}/ppp_frame.cpp 
  ${COMMON}/CborSerializer.cpp
  ${COMMON}/CborDeserializer.cpp
  ${COMMON}/CborDump.cpp
  ${COMMON}/Frame.cpp
  ${COMMON}/util.cpp ) 

target_sources(redis-brain PRIVATE
src/Sys.cpp
src/limero.cpp
src/logger.cpp
${COMMON}/CborSerializer.cpp
${COMMON}/CborDeserializer.cpp
src/BrokerRedis.cpp
${COMMON}/CborDump.cpp
src/brain.cpp
${COMMON}/util.cpp )



target_sources(udp-client PRIVATE
src/Sys.cpp
src/limero.cpp
src/logger.cpp
${COMMON}/CborSerializer.cpp
${COMMON}/CborDeserializer.cpp
${COMMON}/CborDump.cpp
src/udp_client.cpp
src/Udp.cpp
${COMMON}/util.cpp )

target_link_libraries(redis-proxy 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-lpthread  
-lhiredis
-lrt  
-latomic)
#[[
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
  ${COMMON}/Frame.cpp
  ${COMMON}/util.cpp ) 

target_sources(zenoh-brain PRIVATE
src/Sys.cpp
src/limero.cpp
src/logger.cpp
${COMMON}/CborSerializer.cpp
${COMMON}/CborDeserializer.cpp
src/BrokerZenoh.cpp
${COMMON}/CborDump.cpp
src/brain.cpp
${COMMON}/util.cpp )

target_link_libraries(zenoh-proxy 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-L/usr/local/lib/ -lzenohc
-lpthread  
-lrt 
-latomic)

target_link_libraries(zenoh-brain 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-L/usr/local/lib/ -lzenohc
-lpthread  
-lrt  
-latomic)
]]

target_link_libraries(redis-test 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
gtest_main
-lhiredis
-lpthread  
-lrt  
-latomic)

target_link_libraries(dtr_rts 
-lpthread  
-lrt  
-latomic)

target_link_libraries(test 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
gtest_main
-lpthread  
-lrt  
-latomic)

target_link_libraries(redis-test 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
gtest_main
-lpthread  
-lrt  
-latomic)

target_link_libraries(redis-brain 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-lpthread  
-lhiredis
-lrt  
-latomic)

target_link_libraries(udp-client 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
gtest_main
-lpthread  
-lrt  
-latomic)



include(GoogleTest)
gtest_discover_tests(test)
