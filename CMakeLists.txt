project(paranoid-vector)
cmake_minimum_required(VERSION 3.10)

include(GNUInstallDirs)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(PARANOIA_LOGGING_ENABLED OFF)
if (PARANOIA_LOGGING_ENABLED)
    add_definitions(-DPARANOIA_LOGGING=1)
endif()

add_library(paranoid-vector SHARED
    src/paranoia_pool.cpp
    src/util.cpp
    )

target_include_directories(paranoid-vector
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/paranoia>  # <prefix>/include/paranoia
    )

set(PARANOIA_PUBLIC_HEADERS
    include/util.h
    include/paranoia_allocator.h
    include/paranoia_pool.h
    include/paranoid_vector.h
    )

set_target_properties(paranoid-vector
    PROPERTIES PUBLIC_HEADER "${PARANOIA_PUBLIC_HEADERS}"
    )

add_executable(unit-tests
    src/unit_tests.cpp
    )

target_link_libraries(unit-tests
    paranoid-vector
    )

install(
    TARGETS paranoid-vector
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/paranoia"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )


add_subdirectory(src/paranoid_malloc_free)

