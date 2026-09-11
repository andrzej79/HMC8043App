cmake_minimum_required(VERSION 3.16)

project(HMCSupplyApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Which front end to build. Both link the same UI-agnostic driver library.
set(HMC_UI "Widgets" CACHE STRING "User interface to build: Widgets (desktop) or Quick (phone)")
set_property(CACHE HMC_UI PROPERTY STRINGS Widgets Quick)
if(NOT HMC_UI MATCHES "^(Widgets|Quick)$")
  message(FATAL_ERROR "HMC_UI must be 'Widgets' or 'Quick', got '${HMC_UI}'")
endif()

if(HMC_UI STREQUAL "Quick")
  find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Network
    Quick
    QuickControls2
    Svg
  )
  qt_standard_project_setup(REQUIRES 6.8)
  set(QT_VERSION_MAJOR 6)
else()
  # The desktop UI still builds against Qt 5.15 as well as Qt 6.
  find_package(QT 5.15 NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Core
    Network
    Widgets
  )
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Network
    Widgets
  )
  if(QT_VERSION_MAJOR EQUAL 6)
    qt_standard_project_setup()
  else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
  endif()
endif()

# --- driver: SCPI over TCP on its own thread, no UI dependency ---
set(HMC_CORE_SOURCES
  hmcsupplyctrl.h
  hmcsupplyctrl.cpp
  hmcappglobal.h
  hmcpresets.h
)
if(QT_VERSION_MAJOR EQUAL 6)
  qt_add_library(hmc_core STATIC ${HMC_CORE_SOURCES})
else()
  add_library(hmc_core STATIC ${HMC_CORE_SOURCES})
endif()

target_include_directories(hmc_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Network is public: hmcsupplyctrl.h exposes QTcpSocket/QHostAddress.
target_link_libraries(hmc_core PUBLIC
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
)

if(HMC_UI STREQUAL "Quick")
  add_subdirectory(quick)
  return()
endif()

# --- desktop UI (Qt Widgets) ---
set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        hmcchannelwidget.h
        hmcchannelwidget.cpp
        hmcchannelwidget.ui
        valuesetdialog.h
        valuesetdialog.cpp
        valuesetdialog.ui
        hostaddrconfigdialog.h
        hostaddrconfigdialog.cpp
        hostaddrconfigdialog.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(HMCSupplyApp
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(HMCSupplyApp SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(HMCSupplyApp
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(HMCSupplyApp PRIVATE
  hmc_core
  Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(HMCSupplyApp PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS HMCSupplyApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(HMCSupplyApp)
endif()
