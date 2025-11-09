idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "src"
    REQUIRES arduino-esp32 nvs_flash
)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Wno-switch -Wno-deprecated-enum-enum-conversion")
endif()
