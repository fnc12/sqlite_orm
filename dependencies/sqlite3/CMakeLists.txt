include(FetchContent)

FetchContent_Declare(
  sqlite3
  URL      https://www.sqlite.org/2020/sqlite-amalgamation-3340000.zip
)

FetchContent_GetProperties(sqlite3)
if(NOT sqlite3_POPULATED)
  FetchContent_Populate(sqlite3)
  add_library(sqlite3 ${sqlite3_SOURCE_DIR}/sqlite3.c)
  add_library(SQLite::SQLite3 ALIAS sqlite3)
  target_include_directories(sqlite3 INTERFACE $<BUILD_INTERFACE:${sqlite3_SOURCE_DIR}>)
  target_link_libraries(sqlite3 PRIVATE ${CMAKE_DL_LIBS})
endif()
