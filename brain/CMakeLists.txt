cmake_minimum_required(VERSION 3.12)
project(spine)
add_executable(redis-brain)
add_executable(ice-brain)

include(GNUInstallDirs)

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_hoofs CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

set(CMAKE_CXX_STANDARD 11)

add_definitions( -DLINUX  )
add_definitions(-DCATCH_CONFIG_DISABLE)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} )
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} )
target_compile_options(redis-brain PRIVATE "-DBROKER_REDIS")
target_compile_options(ice-brain PRIVATE "-DBROKER_ICE" ) #  ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}  )

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

set(HIREDIS ../hiredis )
set(BROKER ../broker/src)
set(TINYCBOR ../tinycbor/src )
set(ETL ../etl )
set(COMMON ../common/src )
set(LINUX ../linux/src )


include_directories(
  src 
  /usr/local/include
  ${COMMON}
  ${LINUX}
  ${TINYCBOR}
  ${ETL}/include
  ${HIREDIS}
  ${BROKER}
/home/lieven/workspace/ArduinoJson/src )

target_sources(redis-brain PRIVATE
${LINUX}/Sys.cpp
${LINUX}/limero.cpp
${LINUX}/logger.cpp
${COMMON}/CborSerializer.cpp
${COMMON}/CborDeserializer.cpp
${BROKER}/BrokerRedis.cpp
${COMMON}/CborDump.cpp
src/brain.cpp
${COMMON}/util.cpp )

target_sources(ice-brain PRIVATE
${LINUX}/Sys.cpp
${LINUX}/limero.cpp
${LINUX}/logger.cpp
${COMMON}/CborSerializer.cpp
${COMMON}/CborDeserializer.cpp
${BROKER}/BrokerIceoryx.cpp
${COMMON}/CborDump.cpp
src/brain.cpp
${COMMON}/util.cpp )

target_link_libraries(ice-brain iceoryx_posh::iceoryx_posh)

target_link_libraries(redis-brain 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-lpthread  
-lhiredis
-lrt  
-latomic)

target_link_libraries(ice-brain 
iceoryx_posh::iceoryx_posh
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-lpthread  
-lrt  
-latomic)

