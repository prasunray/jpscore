################################################################################
# Build lib
################################################################################
add_library(shared STATIC
    src/Logger.cpp
    src/Logger.hpp
    src/ResultHandling.cpp
    src/ResultHandling.hpp
)

target_link_libraries(shared PUBLIC
    spdlog::spdlog
    fmt::fmt
    tinyxml
)

target_include_directories(shared
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(shared PRIVATE
    ${COMMON_COMPILE_OPTIONS}
)

set_property(TARGET shared PROPERTY INTERPROCEDURAL_OPTIMIZATION ${USE_IPO})
set_property(TARGET shared PROPERTY INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

################################################################################
# unit tests
################################################################################
if(BUILD_TESTS)
    add_executable(libshared-tests
        test/ResultHandlingTest.cpp
    )
    target_link_libraries(libshared-tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        shared
    )

    target_compile_options(libcore-tests PRIVATE
        ${COMMON_COMPILE_OPTIONS}
    )

    set_property(TARGET libshared-tests PROPERTY INTERPROCEDURAL_OPTIMIZATION ${USE_IPO})
    set_property(TARGET libshared-tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
endif()
