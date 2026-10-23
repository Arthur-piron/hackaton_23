cmake_minimum_required(VERSION 3.20)
project(LastLight VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Type de build" FORCE)
endif()

# Runtime MSVC statique : un seul .exe, sans redistribuable Visual C++ à installer.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- raylib 5.5 -------------------------------------------------------------
# Archive plutôt que dépôt git : le clone échoue sous Windows (chemins trop longs).
include(FetchContent)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# --- Jeu --------------------------------------------------------------------
file(GLOB_RECURSE LASTLIGHT_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(lastlight ${LASTLIGHT_SOURCES})

# ${raylib_SOURCE_DIR}/src donne accès à raymath.h, rlgl.h et external/cgltf.h
# (les symboles cgltf sont déjà compilés dans la lib raylib).
target_include_directories(lastlight PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${raylib_SOURCE_DIR}/src"
)
target_link_libraries(lastlight PRIVATE raylib)

if(MSVC)
    target_compile_options(lastlight PRIVATE /W4 /permissive- /utf-8)
    # Pas de console en Release, mais main() reste le point d'entrée.
    set_target_properties(lastlight PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
    target_link_options(lastlight PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
else()
    target_compile_options(lastlight PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Les assets sont copiés à côté de l'exécutable : le jeu les cherche via GetApplicationDirectory().
add_custom_command(TARGET lastlight POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:lastlight>/assets"
    COMMENT "Copie des assets à côté de l'exécutable"
)

install(TARGETS lastlight RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets" DESTINATION .)
