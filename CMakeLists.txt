cmake_minimum_required(VERSION 3.24)

project(
    UtilityForge
    VERSION 0.1.0
    DESCRIPTION "Native Linux desktop utility collection"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(UtilityForgeOptions)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

add_library(utilityforge_core STATIC
    src/core/errors/AppError.cpp
    src/core/errors/AppError.h
    src/core/errors/ErrorFormatter.cpp
    src/core/errors/ErrorFormatter.h
    src/core/notifications/NotificationCenter.cpp
    src/core/notifications/NotificationCenter.h
    src/core/process/ExternalToolRegistry.cpp
    src/core/process/ExternalToolRegistry.h
    src/core/process/ProcessRequest.h
    src/core/process/ProcessRunner.cpp
    src/core/process/ProcessRunner.h
    src/core/settings/AppPaths.cpp
    src/core/settings/AppPaths.h
    src/core/settings/SettingsService.cpp
    src/core/settings/SettingsService.h
    src/core/tasks/CancellationToken.cpp
    src/core/tasks/CancellationToken.h
    src/core/tasks/TaskManager.cpp
    src/core/tasks/TaskManager.h
    src/core/tasks/TaskRecord.h
    src/core/tasks/TaskTypes.h
)

target_include_directories(utilityforge_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(utilityforge_core PUBLIC Qt6::Core)
utilityforge_enable_warnings(utilityforge_core)

add_library(utilityforge_dropzone STATIC
    src/modules/dropzone/DropZonePage.cpp
    src/modules/dropzone/DropZonePage.h
)

target_include_directories(utilityforge_dropzone PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(utilityforge_dropzone PUBLIC utilityforge_core Qt6::Widgets)
utilityforge_enable_warnings(utilityforge_dropzone)

add_library(utilityforge_app STATIC
    src/app/ApplicationBootstrap.cpp
    src/app/ApplicationBootstrap.h
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/SettingsDialog.cpp
    src/app/SettingsDialog.h
)

target_include_directories(utilityforge_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(
    utilityforge_app
    PUBLIC utilityforge_dropzone
    PRIVATE utilityforge_core Qt6::Widgets
)
utilityforge_enable_warnings(utilityforge_app)

add_executable(utilityforge src/app/main.cpp)
target_link_libraries(utilityforge PRIVATE utilityforge_app)
utilityforge_enable_warnings(utilityforge)

include(GNUInstallDirs)
install(TARGETS utilityforge RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(
    FILES packaging/linux/io.github.ARCns09.UtilityForge.desktop
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)
install(
    FILES packaging/linux/io.github.ARCns09.UtilityForge.metainfo.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
