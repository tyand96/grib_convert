cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED PROJECT_NAME)
    project(GribConvert_tests CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    set(STANDALONE_BUILD TRUE)

    set(MAIN_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

    include_directories(${MAIN_PROJECT_DIR}/cpp/include)

    file(GLOB_RECURSE LIB_SOURCES
        "${MAIN_PROJECT_DIR}/cpp/src/*.cpp"
    )

    add_library(grib_convert ${LIB_SOURCES})
    target_include_directories(grib_convert PUBLIC ${MAIN_PROJECT_DIR}/cpp/include)

    enable_testing()
else()
    set(STANDALONE_BUILD FALSE)
    set(MAIN_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
endif()

find_package(Catch2 REQUIRED)

file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/unit/*.cpp)

add_executable(grib_tests ${TEST_SOURCES})

target_link_libraries(grib_tests PRIVATE
    grib_convert
    Catch2::Catch2WithMain
)

# For older Catch2 versions, adjust accordingly
if(NOT TARGET Catch2::Catch2WithMain)
    target_link_libraries(grib_tests PRIVATE Catch2::Catch2)
endif()

include(CTest)
if(${Catch2_VERSION} VERSION_GREATER_EQUAL 3.0.0)
    include(Catch)
    catch_discover_tests(grib_tests)
else()
    include(Catch2/Catch)
    catch_discover_tests(grib_tests)
endif()