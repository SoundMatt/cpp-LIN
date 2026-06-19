add_executable(cpplin_tests
    test_lin.cpp
    test_relay.cpp
    test_virtual.cpp
    test_relay_adapter.cpp
    test_safety.cpp
    test_ldf.cpp
    test_master.cpp
    test_slave.cpp
    test_cli.cpp
)

target_link_libraries(cpplin_tests
    PRIVATE
        cpplin_lib
        Catch2::Catch2WithMain
)

target_include_directories(cpplin_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../cli
)

include(CTest)
include(Catch)
catch_discover_tests(cpplin_tests)
