set(LABA1_SOURCES
        child.c
        parent.c
)

foreach(SRC_FILE ${LABA1_SOURCES})
    get_filename_component(EXE_NAME ${SRC_FILE} NAME_WE)
    add_executable(${EXE_NAME} ${SRC_FILE})
endforeach()
