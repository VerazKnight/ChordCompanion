if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE and OUTPUT_FILE are required")
endif()

get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

file(READ "${INPUT_FILE}" SOUNDFONT_HEX HEX)

# Wrap every 12 bytes before converting hex pairs into C++ literals.
string(REPEAT "[0-9a-fA-F]" 24 LINE_PATTERN)
string(REGEX REPLACE
    "(${LINE_PATTERN})"
    "\\1\n"
    SOUNDFONT_HEX
    "${SOUNDFONT_HEX}"
)

string(REGEX REPLACE
    "([0-9a-fA-F][0-9a-fA-F])"
    "0x\\1, "
    SOUNDFONT_BYTES
    "${SOUNDFONT_HEX}"
)

file(WRITE "${OUTPUT_FILE}"
    "#pragma once\n"
    "#include <cstddef>\n\n"
    "static const unsigned char embedded_sf2[] = {\n"
    "${SOUNDFONT_BYTES}\n"
    "};\n\n"
    "static constexpr std::size_t embedded_sf2_len = "
    "sizeof(embedded_sf2);\n"
)