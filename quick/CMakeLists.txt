qt_add_executable(HMCSupplyMobile
  main.cpp
)

set_source_files_properties(Theme.qml PROPERTIES
  QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(HMCSupplyMobile
  URI HMCMobile
  QML_FILES
    Main.qml
    Theme.qml
    SupplyPage.qml
    ChannelPage.qml
    Readout.qml
    ValueEntryDialog.qml
    AlertDialog.qml
    ConfirmDialog.qml
    SettingsPage.qml
  SOURCES
    channelstate.h
    channelstate.cpp
    supplybackend.h
    supplybackend.cpp
  RESOURCES
    icons/settings.svg
)

# Qt Quick Controls only reads its style configuration from the resource root.
qt_add_resources(HMCSupplyMobile "hmc_mobile_style"
  PREFIX "/"
  FILES
    qtquickcontrols2.conf
)

target_link_libraries(HMCSupplyMobile PRIVATE
  hmc_core
  Qt6::Quick
  Qt6::QuickControls2
  Qt6::Svg
)

set_target_properties(HMCSupplyMobile PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER eu.cs-lab.hmcsupplymobile
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
)

install(TARGETS HMCSupplyMobile
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
