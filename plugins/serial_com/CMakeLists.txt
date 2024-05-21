# Copyright 2021-2023 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# Mumble source tree or at <https://www.mumble.info/LICENSE>.

# Find the QtCore module
find_package(Qt5 COMPONENTS Core REQUIRED)
# Manually set include directories
include_directories("/usr/include/libserial")
include_directories("/usr/local/include/libserial")
include_directories("/usr/lib/x86_64-linux-gnu/include/libserial") # Adjust this path as needed

# Manually set library directories
link_directories("/usr/lib")
link_directories("/usr/local/lib")
link_directories("/usr/lib/x86_64-linux-gnu") # Adjust this path as needed


add_library(serial_com SHARED "SerialCom.cpp" "USBSerialCom.cpp" )
target_link_libraries(serial_com Qt5::Core serial)
