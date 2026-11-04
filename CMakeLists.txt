cmake_minimum_required(VERSION 3.20)
project(lejuice C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra)

include_directories(include source)

set(CORE_SOURCES
        source/engine/config.c
        source/engine/engine.c
        source/engine/parser.c
        source/engine/queue.c
        source/frontend/menu.c
)

if (WIN32)
    set(PLATFORM_SOURCES
            source/platform/win32/notify.c
            source/platform/win32/system.c
            source/platform/win32/tray.c
            source/platform/win32/watcher.c
    )
    set(PLATFORM_LIBS shlwapi advapi32 shell32 ole32 uuid)
elseif (UNIX)
    set(PLATFORM_SOURCES
            source/platform/linux/notify.c
            source/platform/linux/system.c
            source/platform/linux/tray.c
            source/platform/linux/watcher.c
            include/menu.h
    )
    set(PLATFORM_LIBS pthread)
endif()

add_executable(lejuice
        source/main.c
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
)
target_link_libraries(lejuice ${PLATFORM_LIBS})