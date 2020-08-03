# TO DO: Switch to cmake 3.14 when travis does support it and use the newer command below instead
#FetchContent_MakeAvailable(catch2)

FetchContent_GetProperties(catch2)
if(NOT catch2_POPULATED)
    FetchContent_Populate(catch2)
    add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
endif()
