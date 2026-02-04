idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "."
    REQUIRES Somfy_Remote_Lib SmartRC-CC1101-Driver-Lib arduino-esp32 mqtt
)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Wno-switch -Wno-deprecated-enum-enum-conversion")
endif()
