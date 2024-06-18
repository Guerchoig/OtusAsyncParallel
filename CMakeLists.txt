cmake_minimum_required(VERSION 3.10)

set(PATCH_VERSION "0" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})

project(proj_libasync VERSION ${PROJECT_VESRION})

# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
# )
# For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# configure_file(config.h.in config.h)
set(SOURCE_EXE main_test.cpp)
# set(SOURCE_LIB libasync.cpp)	

add_executable(main_test main_test.cpp)

add_library(async SHARED async.cpp cmd_output.cpp async.cpp)
# add_executable(test_main_control test_main_control.cpp)

set_target_properties(main_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
# target_include_directories(main_test 
#     PRIVATE "${CMAKE_BINARY_DIR}"
# )
# target_include_directories(async PRIVATE  "${CMAKE_BINARY_DIR}")
target_include_directories(main_test PRIVATE  "${CMAKE_BINARY_DIR}")

link_directories("${CMAKE_BINARY_DIR}")

target_link_libraries(main_test async)
# target_link_libraries(test_main_control
#     GTest::gtest_main_control
#     main_control_lib
# )

if (MSVC)
    target_compile_options(main_test 
        /W4
    )
    target_compile_options(async 
    /W4 )
else ()
    target_compile_options(main_test PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    target_compile_options(async PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    
endif()



# install(TARGETS async RUNTIME DESTINATION bin)

# set(CPACK_GENERATOR DEB)

# set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
# set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
# set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

# set(CPACK_PACKAGE_CONTACT alex-guerchoig@yandex.ru)

# include(CPack)

# enable_testing()
# include(GoogleTest)
# gtest_discover_tests(test_main_control)
# add_test(test_main_control  test_main_control)



