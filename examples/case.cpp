/**
 *  Got it from here https://www.tutlane.com/tutorial/sqlite/sqlite-case-statement
 */
#include <sqlite_orm/sqlite_orm.h>
#include <iostream>

using namespace sqlite_orm;

using std::cout;
using std::endl;

int main() {
    struct Student {
        int id = 0;
        std::string name;
        std::string email;
        float marks = 0;
    };
    
    auto storage = make_storage({},
                                make_table("STUDENT",
                                           make_column("ID", &Student::id, primary_key()),
                                           make_column("NAME", &Student::name),
                                           make_column("EMAIL", &Student::email),
                                           make_column("MARKS", &Student::marks)));
    storage.sync_schema();
    
    storage.transaction([&storage]{
        storage.replace(Student{1, "Shweta", "shweta@gmail.com", 80});
        storage.replace(Student{2, "Yamini", "rani@gmail.com", 60});
        storage.replace(Student{3, "Sonal", "sonal@gmail.com", 50});
        storage.replace(Student{4, "Jagruti", "jagu@gmail.com", 30});
        return true;
    });
    
    //  list all students
    for(auto &student : storage.iterate<Student>()) {
        cout << storage.dump(student) << endl;
    }
    cout << endl;
    
    {   // without alias
        
        //  SELECT ID, NAME, MARKS,
        //      CASE
        //      WHEN MARKS >=80 THEN 'A+'
        //      WHEN MARKS >=70 THEN 'A'
        //      WHEN MARKS >=60 THEN 'B'
        //      WHEN MARKS >=50 THEN 'C'
        //      ELSE 'Sorry!! Failed'
        //      END
        //      FROM STUDENT;
        auto rows = storage.select(columns(&Student::id,
                                           &Student::name,
                                           &Student::marks,
                                           case_<std::string>()
                                           .when(greater_or_equal(&Student::marks, 80), then("A+"))
                                           .when(greater_or_equal(&Student::marks, 70), then("A"))
                                           .when(greater_or_equal(&Student::marks, 60), then("B"))
                                           .when(greater_or_equal(&Student::marks, 50), then("C"))
                                           .else_("Sorry!! Failed")
                                           .end()));
        for(auto &row : rows) {
            cout << std::get<0>(row) << ' ' << std::get<1>(row) << ' ' << std::get<2>(row) << ' ' << std::get<3>(row) << endl;
        }
        cout << endl;
    }
    {   // with alias
        
        struct GradeAlias : alias_tag {
            static const std::string &get() {
                static const std::string res = "Grade";
                return res;
            }
        };
        
        //  SELECT ID, NAME, MARKS,
        //      CASE
        //      WHEN MARKS >=80 THEN 'A+'
        //      WHEN MARKS >=70 THEN 'A'
        //      WHEN MARKS >=60 THEN 'B'
        //      WHEN MARKS >=50 THEN 'C'
        //      ELSE 'Sorry!! Failed'
        //      END as 'Grade'
        //      FROM STUDENT;
        auto rows = storage.select(columns(&Student::id,
                                           &Student::name,
                                           &Student::marks,
                                           as<GradeAlias>(case_<std::string>()
                                                           .when(greater_or_equal(&Student::marks, 80), then("A+"))
                                                           .when(greater_or_equal(&Student::marks, 70), then("A"))
                                                           .when(greater_or_equal(&Student::marks, 60), then("B"))
                                                           .when(greater_or_equal(&Student::marks, 50), then("C"))
                                                           .else_("Sorry!! Failed")
                                                           .end())));
        for(auto &row : rows) {
            cout << std::get<0>(row) << ' ' << std::get<1>(row) << ' ' << std::get<2>(row) << ' ' << std::get<3>(row) << endl;
        }
        cout << endl;
    }
    
    return 0;
}
