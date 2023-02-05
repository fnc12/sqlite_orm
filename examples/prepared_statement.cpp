/**
 *  There are two member functions in storage_t class which you need to use to operate with
 *  prepared statements: storage_t::prepare and storage_t::execute.
 *  Also if you need to rebind arguments just use get<N>(statement) = ... syntax
 *  just like you do with std::tuple.
 *  Once a statement is prepared it holds a connection to a database inside. This connection will be open
 *  until at least one statement object exists.
 */
#include <sqlite_orm/sqlite_orm.h>
#include <iostream>

using namespace sqlite_orm;

using std::cout;
using std::endl;

struct Doctor {
    int doctor_id = 0;
    std::string doctor_name;
    std::string degree;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
    Doctor() = default;
    Doctor(int doctor_id, std::string doctor_name, std::string degree) :
        doctor_id{doctor_id}, doctor_name{std::move(doctor_name)}, degree{std::move(degree)} {}
#endif
};

struct Speciality {
    int spl_id = 0;
    std::string spl_descrip;
    int doctor_id = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
    Speciality() = default;
    Speciality(int spl_id, std::string spl_descrip, int doctor_id) :
        spl_id{spl_id}, spl_descrip{std::move(spl_descrip)}, doctor_id{doctor_id} {}
#endif
};

struct Visit {
    int doctor_id = 0;
    std::string patient_name;
    std::string vdate;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
    Visit() = default;
    Visit(int doctor_id, std::string patient_name, std::string vdate) :
        doctor_id{doctor_id}, patient_name{std::move(patient_name)}, vdate{std::move(vdate)} {}
#endif
};

int main() {
    auto storage = make_storage("prepared.sqlite",
                                make_table("doctors",
                                           make_column("doctor_id", &Doctor::doctor_id, primary_key()),
                                           make_column("doctor_name", &Doctor::doctor_name),
                                           make_column("degree", &Doctor::degree)),
                                make_table("speciality",
                                           make_column("spl_id", &Speciality::spl_id, primary_key()),
                                           make_column("spl_descrip", &Speciality::spl_descrip),
                                           make_column("doctor_id", &Speciality::doctor_id)),
                                make_table("visits",
                                           make_column("doctor_id", &Visit::doctor_id),
                                           make_column("patient_name", &Visit::patient_name),
                                           make_column("vdate", &Visit::vdate)));
    storage.sync_schema();
    storage.remove_all<Doctor>();
    storage.remove_all<Speciality>();
    storage.remove_all<Visit>();
    {
        //  first we create a statement object for replace query with a doctor object
        auto replaceStatement = storage.prepare(replace(Doctor{210, "Dr. John Linga", "MD"}));

        cout << "replaceStatement = " << replaceStatement.sql() << endl;

        //  next we execute our statement
        storage.execute(replaceStatement);

        //  now 'doctors' table has one row [210, 'Dr. John Linga', 'MD']
        //  Next we shall reuse the statement to replace another doctor
        //  replaceStatement contains a doctor inside it which can be obtained
        //  with get<0>(statement) function

        get<0>(replaceStatement) = {211, "Dr. Peter Hall", "MBBS"};
        storage.execute(replaceStatement);

        //  now 'doctors' table has two rows.
        //  Next we shall reuse the statement again with member assignment

        auto& doctor = get<0>(replaceStatement);  //  doctor is Doctor &
        doctor.doctor_id = 212;
        doctor.doctor_name = "Dr. Ke Gee";
        doctor.degree = "MD";
        storage.execute(replaceStatement);

        //  now 'doctors' table has three rows.
    }
    {
        //  also prepared statement can store arguments by reference. To do this you can
        //  pass a std::reference_wrapper instead of object value.
        Doctor doctorToReplace{213, "Dr. Pat Fay", "MD"};
        auto replaceStatementByRef = storage.prepare(replace(std::ref(doctorToReplace)));
        cout << "replaceStatementByRef = " << replaceStatementByRef.sql() << endl;
        storage.execute(replaceStatementByRef);

        //  now 'doctors' table has four rows.
        //  next we shall change doctorToReplace object and then execute our statement.
        //  Statement will be affected cause it stores a reference to the doctor
        doctorToReplace.doctor_id = 214;
        doctorToReplace.doctor_name = "Mosby";
        doctorToReplace.degree = "MBBS";

        storage.execute(replaceStatementByRef);

        //  and now 'doctors' table has five rows
    }

    cout << "Doctors count = " << storage.count<Doctor>() << endl;
    for(auto& doctor: storage.iterate<Doctor>()) {
        cout << storage.dump(doctor) << endl;
    }

    {
        auto insertStatement = storage.prepare(insert(Speciality{1, "CARDIO", 211}));
        cout << "insertStatement = " << insertStatement.sql() << endl;
        storage.execute(insertStatement);
        get<0>(insertStatement) = {2, "NEURO", 213};
        storage.execute(insertStatement);
        get<0>(insertStatement) = {3, "ARTHO", 212};
        storage.execute(insertStatement);
        get<0>(insertStatement) = {4, "GYNO", 210};
        storage.execute(insertStatement);
    }

    cout << "Specialities count = " << storage.count<Speciality>() << endl;
    for(auto& speciality: storage.iterate<Speciality>()) {
        cout << storage.dump(speciality) << endl;
    }
    {
        //  let's insert (replace) 5 visits. We create two vectors with 2 visits each
        std::vector<Visit> visits;
        visits.push_back({210, "Julia Nayer", "2013-10-15"});
        visits.push_back({214, "TJ Olson", "2013-10-14"});

        //  let's make a statement
        auto replaceRangeStatement = storage.prepare(replace_range(visits.begin(), visits.end()));
        cout << "replaceRangeStatement = " << replaceRangeStatement.sql() << endl;

        //  replace two objects
        storage.execute(replaceRangeStatement);

        std::vector<Visit> visits2;
        visits2.push_back({215, "John Seo", "2013-10-15"});
        visits2.push_back({212, "James Marlow", "2013-10-16"});

        //  reassign iterators to point to other visits. Beware that if end - begin
        //  will have different distance then you'll get a runtime error cause statement is
        //  already compiled with a fixed amount of arguments to bind
        get<0>(replaceRangeStatement) = visits2.begin();
        get<1>(replaceRangeStatement) = visits2.end();

        storage.execute(replaceRangeStatement);

        storage.replace(Visit{212, "Jason Mallin", "2013-10-12"});
    }
    cout << "Visits count = " << storage.count<Visit>() << endl;
    for(auto& visit: storage.iterate<Visit>()) {
        cout << storage.dump(visit) << endl;
    }
    {
        //  SELECT doctor_id
        //  FROM visits
        //  WHERE LENGTH(patient_name) > 8
        auto selectStatement = storage.prepare(select(&Visit::doctor_id, where(length(&Visit::patient_name) > 8)));
        cout << "selectStatement = " << selectStatement.sql() << endl;
        {
            auto rows = storage.execute(selectStatement);
            cout << "rows count = " << rows.size() << endl;
            for(auto& id: rows) {
                cout << id << endl;
            }
        }

        //  same statement, other bound values
        //  SELECT doctor_id
        //  FROM visits
        //  WHERE LENGTH(patient_name) > 11
        {
            get<0>(selectStatement) = 11;
            auto rows = storage.execute(selectStatement);
            cout << "rows count = " << rows.size() << endl;
            for(auto& id: rows) {
                cout << id << endl;
            }
        }
    }
    {
        //  SELECT rowid, 'Doctor ' || doctor_name
        //  FROM doctors
        //  WHERE degree LIKE '%S'
        auto selectStatement = storage.prepare(
            select(columns(rowid(), "Doctor " || c(&Doctor::doctor_name)), where(like(&Doctor::degree, "%S"))));
        cout << "selectStatement = " << selectStatement.sql() << endl;
        {
            auto rows = storage.execute(selectStatement);
            cout << "rows count = " << rows.size() << endl;
            for(auto& row: rows) {
                cout << get<0>(row) << '\t' << get<1>(row) << endl;
            }
        }
        //  SELECT rowid, 'Nice ' || doctor_name
        //  FROM doctors
        //  WHERE degree LIKE '%D'
        get<0>(selectStatement) = "Nice ";
        get<1>(selectStatement) = "%D";
        {
            auto rows = storage.execute(selectStatement);
            cout << "rows count = " << rows.size() << endl;
            for(auto& row: rows) {
                cout << get<0>(row) << '\t' << get<1>(row) << endl;
            }
        }
    }

    return 0;
}
