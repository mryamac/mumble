# Copyright 2021-2023 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# Mumble source tree or at <https://www.mumble.info/LICENSE>.

find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)

pkg_check_modules(PC_CZMQ REQUIRED libzmq)

include_directories(${PC_CZMQ_INCLUDE_DIRS})
include_directories(${Protobuf_INCLUDE_DIRS})

# Manually set library directories
link_directories("/usr/lib")
link_directories("/usr/local/lib")
link_directories("/usr/lib/x86_64-linux-gnu") # Adjust this path as needed
link_directories(${PC_CZMQ_LIBRARY_DIRS})


add_library(agent SHARED "AgentPlugin.cpp" "Agent.cpp" "Config.cpp" "Logger.cpp" "mumble.pb.cc")
target_link_libraries(agent ${PC_CZMQ_LIBRARIES} ${Protobuf_LIBRARIES})
