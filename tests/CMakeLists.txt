FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(Catch2)

add_executable(JGMasteringEQTests
    PluginBasicsTests.cpp
    ParameterTests.cpp
    AudioTests.cpp)

# Link against the plugin's shared code so tests exercise the real processor
# without needing a DAW.
# The plugin target already contains the compiled JUCE code; borrow its include
# paths and settings instead of compiling JUCE a second time.
target_link_libraries(JGMasteringEQTests PRIVATE
    JGMasteringEQ
    Catch2::Catch2WithMain)
target_include_directories(JGMasteringEQTests PRIVATE
    $<TARGET_PROPERTY:JGMasteringEQ,INCLUDE_DIRECTORIES>)
target_compile_definitions(JGMasteringEQTests PRIVATE
    $<TARGET_PROPERTY:JGMasteringEQ,COMPILE_DEFINITIONS>)

target_compile_definitions(JGMasteringEQTests PRIVATE
    JGEQ_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    JGEQ_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")

# CPU benchmark (not a pass/fail test): prints per-instance CPU use.
add_executable(JGMasteringEQBench Benchmark.cpp)
target_link_libraries(JGMasteringEQBench PRIVATE JGMasteringEQ)
target_include_directories(JGMasteringEQBench PRIVATE $<TARGET_PROPERTY:JGMasteringEQ,INCLUDE_DIRECTORIES>)
target_compile_definitions(JGMasteringEQBench PRIVATE $<TARGET_PROPERTY:JGMasteringEQ,COMPILE_DEFINITIONS>)

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(JGMasteringEQTests)
