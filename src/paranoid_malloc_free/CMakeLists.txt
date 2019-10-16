project(paranoid-malloc-free)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_SHARED_LINKER_FLAGS "-z initfirst -z interpose")

add_library(paranoid-malloc-free SHARED
    paranoid_malloc_free.cpp
    paranoia_pool_real.cpp
    real_heap_funcs.cpp
    )

target_include_directories(paranoid-malloc-free
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    )

target_link_libraries(paranoid-malloc-free
    -ldl
    )

add_executable(test-paranoid-malloc-free
    test_paranoid_malloc_free.cpp)

install(
    TARGETS paranoid-malloc-free
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/paranoia"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

