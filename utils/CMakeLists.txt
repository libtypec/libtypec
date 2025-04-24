cmake_minimum_required(VERSION 3.16.3)
project(libtypec_utils VERSION 0.6.0)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

add_executable(lstypec lstypec.c names.c)
target_include_directories(lstypec PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(lstypec PUBLIC libtypec udev)

add_executable(usbcview usbcview.c names.c)
target_include_directories(usbcview PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(usbcview PUBLIC libtypec udev ${GTK3_LIBRARIES})

add_executable(typecstatus typecstatus.c names.c)
target_link_libraries(typecstatus PUBLIC libtypec udev)

add_executable(ucsicontrol ucsicontrol.c names.c)
target_link_libraries(ucsicontrol PUBLIC libtypec udev)

option(LIBTYPEC_STRICT_CFLAGS "Compile for strict warnings" ON)
if(LIBTYPEC_STRICT_CFLAGS)
    target_compile_options(lstypec PRIVATE -g -O2 -fstack-protector-strong -Wformat=1 -Werror=format-security -Wdate-time -fasynchronous-unwind-tables -D_FORTIFY_SOURCE=2)
    target_compile_options(typecstatus PRIVATE -g -O2 -fstack-protector-strong -Wformat=1 -Werror=format-security -Wdate-time -fasynchronous-unwind-tables -D_FORTIFY_SOURCE=2)
    target_compile_options(ucsicontrol PRIVATE -g -O2 -fstack-protector-strong -Wformat=1 -Werror=format-security -Wdate-time -fasynchronous-unwind-tables -D_FORTIFY_SOURCE=2)
endif()
