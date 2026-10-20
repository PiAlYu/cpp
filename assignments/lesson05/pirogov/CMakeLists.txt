cmake_minimum_required(VERSION 3.20)
project(pirogov_local_chat LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
    httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(nlohmann_json httplib)

add_library(chat_core
    src/chat.cpp
    src/config.cpp
    src/http_client.cpp
)
target_include_directories(chat_core PUBLIC include)
target_compile_features(chat_core PUBLIC cxx_std_17)
set_target_properties(chat_core PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(chat_core
    PUBLIC nlohmann_json::nlohmann_json
    PRIVATE httplib::httplib
)

add_executable(local_chat src/main.cpp)
target_link_libraries(local_chat PRIVATE chat_core)

if(WIN32)
    target_sources(local_chat PRIVATE src/utf8.manifest)
endif()

if(MSVC)
    target_compile_options(chat_core PRIVATE /utf-8)
    target_compile_options(local_chat PRIVATE /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(chat_test tests/chat_test.cpp)
    target_link_libraries(chat_test PRIVATE chat_core)
    if(MSVC)
        target_compile_options(chat_test PRIVATE /utf-8)
    endif()
    add_test(NAME chat_test COMMAND chat_test)
endif()
