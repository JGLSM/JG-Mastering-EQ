FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(Catch2)

add_executable(JGMasteringEQTests
    PluginBasicsTests.cpp)

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

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(JGMasteringEQTests)
