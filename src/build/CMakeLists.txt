cmake_minimum_required(VERSION 2.8.12)

project(takeoff_and_land_and_data)

# socket.io

include_directories("./build/include")
link_directories("./build/lib/Release")
if (NOT ENABLE_SSL)
  list(APPEND extra_libs "sioclient")
else()
  list(APPEND extra_libs "sioclient_tls")
  find_package(OpenSSL)
  list(APPEND extra_libs ${OPENSSL_LIBRARIES})
endif()

# Boost
find_package(Boost REQUIRED COMPONENTS system date_time random)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
list(APPEND extra_libs ${Boost_LIBRARIES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # MaxOSX
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

else()
  # Linux or other UNIX like system
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# add_executable(client client.cpp)
# target_link_libraries(client ${extra_libs})

set(CMAKE_CXX_STANDARD 11) # This is equal to QMAKE_CXX_FLAGS += -std=c++0x
# set(CMAKE_INCLUDE_CURRENT_DIR ON)


# if(NOT MSVC)
#     add_definitions("-std=c++11 -Wall -Wextra -Werror")
# else()
#     add_definitions("-std=c++11 -WX -W2")
# endif()

find_package(MAVSDK REQUIRED)


add_executable(takeoff_and_land_and_data
    takeoff_and_land_and_data.cpp
)

target_link_libraries(takeoff_and_land_and_data
    MAVSDK::mavsdk_telemetry
    MAVSDK::mavsdk_action
    MAVSDK::mavsdk
    ${extra_libs}
)
