set(SCRIPT_PATH "${CURRENT_INSTALLED_DIR}/share/qtbase")
include("${SCRIPT_PATH}/qt_install_submodule.cmake")

vcpkg_buildpath_length_warning(44)

set(${PORT}_PATCHES
    24205cd-qquickwindow-child-window-stacking.patch
)

 set(TOOL_NAMES
        qml
        qmlaotstats
        qmlcachegen
        qmlcontextpropertydump
        qmleasing
        qmlformat
        qmlimportscanner
        qmllint
        qmlplugindump
        qmlpreview
        qmlprofiler
        qmlscene
        qmltestrunner
        qmltime
        qmltyperegistrar
        qmldom
        qmltc
        qmlls
        qmljsrootgen
        svgtoqml
    )

set(QTDECLARATIVE_CONFIGURE_OPTIONS
    -DCMAKE_DISABLE_FIND_PACKAGE_LTTngUST:BOOL=ON
)
if(VCPKG_CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    list(APPEND QTDECLARATIVE_CONFIGURE_OPTIONS
        -DCMAKE_AUTOGEN_COMMAND_LINE_LENGTH_MAX:STRING=4096
    )
endif()
if(VCPKG_TARGET_IS_WINDOWS AND
   TARGET_TRIPLET STREQUAL "x64-windows-rg-host-release")
    # This package only provides host tools for the Wasm cross-build.
    # Keep the Wasm target's optional style features at upstream defaults.
    list(APPEND QTDECLARATIVE_CONFIGURE_OPTIONS
        -DFEATURE_quickcontrols2_fluentwinui3:BOOL=OFF
        -DFEATURE_quickcontrols2_universal:BOOL=OFF
    )
endif()

qt_install_submodule(PATCHES    ${${PORT}_PATCHES}
                     TOOL_NAMES ${TOOL_NAMES}
                     CONFIGURE_OPTIONS
                      ${QTDECLARATIVE_CONFIGURE_OPTIONS}
                     CONFIGURE_OPTIONS_RELEASE
                     CONFIGURE_OPTIONS_DEBUG
                    )
