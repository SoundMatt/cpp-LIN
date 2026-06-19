add_executable(cpp-lin-cli main.cpp)
target_link_libraries(cpp-lin-cli PRIVATE cpplin_lib)
target_include_directories(cpp-lin-cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
