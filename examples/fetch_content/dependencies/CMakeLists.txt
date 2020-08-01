include(FetchContent)

FetchContent_Declare(sqliteOrm
  GIT_REPOSITORY https://github.com/Leon0402/sqlite_orm.git
  GIT_TAG        origin/feature/dependencyManagment
)

add_subdirectory(sqlite_orm)
