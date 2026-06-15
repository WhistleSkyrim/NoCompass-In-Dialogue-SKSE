cmake_minimum_required(VERSION 3.25)

project(DialogueHUDFade VERSION 1.0.0 LANGUAGES CXX)

option(DHF_BUILD_PLUGIN "Build the SKSE plugin DLL" ON)
option(DHF_VR_ONLY "Documented VR-focused build switch; commonlibsse-ng still uses one codebase" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

set(DHF_CORE_SOURCES
    src/HudFader.cpp
    src/Settings.cpp
)

add_library(DialogueHUDFadeCore STATIC ${DHF_CORE_SOURCES})
target_include_directories(DialogueHUDFadeCore PUBLIC src)
target_compile_features(DialogueHUDFadeCore PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(DialogueHUDFadeCore PRIVATE /W4 /permissive- /Zc:preprocessor /utf-8)
else()
    target_compile_options(DialogueHUDFadeCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DHF_BUILD_PLUGIN)
    find_package(CommonLibSSE CONFIG REQUIRED)

    set(DHF_PLUGIN_SOURCES
        src/DialogueMenuWatcher.cpp
        src/Logger.cpp
        src/Plugin.cpp
        src/ScaleformHudTarget.cpp
        src/main.cpp
        src/PCH.h
    )

    add_commonlibsse_plugin(${PROJECT_NAME}
        SOURCES
            ${DHF_PLUGIN_SOURCES}
            ${DHF_CORE_SOURCES}
    )

    target_include_directories(${PROJECT_NAME} PRIVATE src)
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
    target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

    if(DHF_VR_ONLY)
        target_compile_definitions(${PROJECT_NAME} PRIVATE DHF_VR_ONLY_BUILD=1)
    endif()

    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /Zc:preprocessor /EHsc /MP /utf-8)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            "$<$<CONFIG:Debug>:msvcprtd>"
            "$<$<NOT:$<CONFIG:Debug>>:msvcprt>"
        )
    endif()

    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION Data/SKSE/Plugins
        LIBRARY DESTINATION Data/SKSE/Plugins
    )
    install(FILES config/DialogueHUDFade.ini DESTINATION Data/SKSE/Plugins)
    install(FILES README.md LICENSE DESTINATION .)

    set(DHF_DIST_DIR "${PROJECT_SOURCE_DIR}/dist")
    set(DHF_ZIP_PATH "${PROJECT_SOURCE_DIR}/No-Compass-in-Dialogue-SKSE-v${PROJECT_VERSION}.zip")

    add_custom_target(stage_mod
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${DHF_DIST_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DHF_DIST_DIR}/Data/SKSE/Plugins"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DHF_DIST_DIR}/Data/SKSE/Plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${PROJECT_SOURCE_DIR}/config/DialogueHUDFade.ini" "${DHF_DIST_DIR}/Data/SKSE/Plugins/DialogueHUDFade.ini"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${PROJECT_SOURCE_DIR}/README.md" "${DHF_DIST_DIR}/README.md"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${PROJECT_SOURCE_DIR}/LICENSE" "${DHF_DIST_DIR}/LICENSE"
        DEPENDS ${PROJECT_NAME}
        VERBATIM
    )

    add_custom_target(package_mod
        COMMAND "${CMAKE_COMMAND}" -E rm -f "${DHF_ZIP_PATH}"
        COMMAND "${CMAKE_COMMAND}" -E tar cfv "${DHF_ZIP_PATH}" --format=zip Data README.md LICENSE
        WORKING_DIRECTORY "${DHF_DIST_DIR}"
        DEPENDS stage_mod
        VERBATIM
    )
endif()

if(BUILD_TESTING)
    add_executable(DialogueHUDFadeTests
        tests/FaderTests.cpp
        tests/SettingsTests.cpp
        tests/StateMachineTests.cpp
    )
    target_link_libraries(DialogueHUDFadeTests PRIVATE DialogueHUDFadeCore)
    target_compile_features(DialogueHUDFadeTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(DialogueHUDFadeTests PRIVATE /W4 /permissive- /utf-8)
    endif()

    add_test(NAME DialogueHUDFadeTests COMMAND DialogueHUDFadeTests)
endif()
