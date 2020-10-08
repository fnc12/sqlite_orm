add_executable(Test)
target_link_libraries(Test PRIVATE sqlite_orm::sqlite_orm)

target_sources(Test 
    PRIVATE 
        main.cpp 
)