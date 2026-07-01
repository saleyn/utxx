# vim:ts=2:sw=2:et
#
# Standalone reactor tests — no boost, no utxx dependency.
# Each test binary is self-contained.
#
cmake_minimum_required(VERSION 3.14)

set(REACTOR_INC ${CMAKE_SOURCE_DIR}/include)
set(REACTOR_SRC ${CMAKE_SOURCE_DIR}/src/reactor)

# Common compile options
set(REACTOR_FLAGS -std=c++17 -Wall -Wextra -Wno-unused-parameter)

# Shared source files compiled into every test binary
set(REACTOR_SRCS
    ${REACTOR_SRC}/reactor_misc.cpp
    ${REACTOR_SRC}/reactor_fd_info.cpp
    ${REACTOR_SRC}/reactor.cpp
)

# ---- test_compat ------------------------------------------------------------
add_executable(test_reactor_compat
    test_compat.cpp
)
target_compile_options(test_reactor_compat PRIVATE ${REACTOR_FLAGS})
target_include_directories(test_reactor_compat PRIVATE ${REACTOR_INC})
target_link_libraries(test_reactor_compat rt aio)

# ---- test_reactor_types -----------------------------------------------------
add_executable(test_reactor_types
    test_reactor_types.cpp
    ${REACTOR_SRCS}
)
target_compile_options(test_reactor_types PRIVATE ${REACTOR_FLAGS})
target_include_directories(test_reactor_types PRIVATE ${REACTOR_INC})
target_link_libraries(test_reactor_types rt aio)

# ---- test_reactor_misc ------------------------------------------------------
add_executable(test_reactor_misc
    test_reactor_misc.cpp
    ${REACTOR_SRC}/reactor_misc.cpp
)
target_compile_options(test_reactor_misc PRIVATE ${REACTOR_FLAGS})
target_include_directories(test_reactor_misc PRIVATE ${REACTOR_INC})
target_link_libraries(test_reactor_misc rt aio)

# ---- test_aio_reader --------------------------------------------------------
add_executable(test_reactor_aio
    test_aio_reader.cpp
)
target_compile_options(test_reactor_aio PRIVATE ${REACTOR_FLAGS})
target_include_directories(test_reactor_aio PRIVATE ${REACTOR_INC})
target_link_libraries(test_reactor_aio rt aio)

# ---- test_reactor (integration) ---------------------------------------------
add_executable(test_reactor
    test_reactor.cpp
    ${REACTOR_SRCS}
)
target_compile_options(test_reactor PRIVATE ${REACTOR_FLAGS})
target_include_directories(test_reactor PRIVATE ${REACTOR_INC})
target_link_libraries(test_reactor rt aio)

# ---- CTest registrations ----------------------------------------------------
add_test(NAME reactor_compat  COMMAND test_reactor_compat)
add_test(NAME reactor_types   COMMAND test_reactor_types)
add_test(NAME reactor_misc    COMMAND test_reactor_misc)
add_test(NAME reactor_aio     COMMAND test_reactor_aio)
add_test(NAME reactor_integ   COMMAND test_reactor)
