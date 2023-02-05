/**
 *  This example was grabbed from here https://www.sqlitetutorial.net/sqlite-trigger/
 */

#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <string>

using namespace sqlite_orm;
using std::cout;
using std::endl;

struct Lead {
    int id = 0;
    std::string firstName;
    std::string lastName;
    std::string email;
    std::string phone;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
    Lead() = default;
    Lead(int id, std::string firstName, std::string lastName, std::string email, std::string phone) :
        id{id}, firstName{std::move(firstName)}, lastName{std::move(lastName)}, email{std::move(email)},
        phone{std::move(phone)} {}
#endif
};

struct LeadLog {
    int id = 0;
    int oldId = 0;
    int newId = 0;
    std::string oldPhone;
    std::string newPhone;
    std::string oldEmail;
    std::string newEmail;
    std::string userAction;
    std::string createdAt;
};

int main() {
    auto storage = make_storage("",

                                //  CREATE TRIGGER validate_email_before_insert_leads
                                //     BEFORE INSERT ON leads
                                //  BEGIN
                                //     SELECT
                                //        CASE
                                //      WHEN NEW.email NOT LIKE '%_@__%.__%' THEN
                                //           RAISE (ABORT,'Invalid email address')
                                //         END;
                                //  END;
                                make_trigger("validate_email_before_insert_leads",
                                             before()
                                                 .insert()
                                                 .on<Lead>()
                                                 .begin(select(case_<int>()
                                                                   .when(not like(new_(&Lead::email), "%_@__%.__%"),
                                                                         then(raise_abort("Invalid email address")))
                                                                   .end()))
                                                 .end()),

                                //  CREATE TRIGGER log_contact_after_update
                                //     AFTER UPDATE ON leads
                                //     WHEN old.phone <> new.phone
                                //          OR old.email <> new.email
                                //  BEGIN
                                //      INSERT INTO lead_logs (
                                //          old_id,
                                //          new_id,
                                //          old_phone,
                                //          new_phone,
                                //          old_email,
                                //          new_email,
                                //          user_action,
                                //          created_at
                                //      )
                                //  VALUES
                                //      (
                                //          old.id,
                                //          new.id,
                                //          old.phone,
                                //          new.phone,
                                //          old.email,
                                //          new.email,
                                //          'UPDATE',
                                //          DATETIME('NOW')
                                //      ) ;
                                //  END;
                                make_trigger("log_contact_after_update",
                                             after()
                                                 .update()
                                                 .on<Lead>()
                                                 .when(is_not_equal(old(&Lead::phone), new_(&Lead::phone)) and
                                                       is_not_equal(old(&Lead::email), new_(&Lead::email)))
                                                 .begin(insert(into<LeadLog>(),
                                                               columns(&LeadLog::oldId,
                                                                       &LeadLog::newId,
                                                                       &LeadLog::oldPhone,
                                                                       &LeadLog::newPhone,
                                                                       &LeadLog::oldEmail,
                                                                       &LeadLog::newEmail,
                                                                       &LeadLog::userAction,
                                                                       &LeadLog::createdAt),
                                                               values(std::make_tuple(old(&Lead::id),
                                                                                      new_(&Lead::id),
                                                                                      old(&Lead::phone),
                                                                                      new_(&Lead::phone),
                                                                                      old(&Lead::email),
                                                                                      new_(&Lead::email),
                                                                                      "UPDATE",
                                                                                      datetime("NOW")))))
                                                 .end()),

                                //  CREATE TABLE leads (
                                //      id integer PRIMARY KEY,
                                //      first_name text NOT NULL,
                                //      last_name text NOT NULL,
                                //      email text NOT NULL,
                                //      phone text NOT NULL
                                //  );
                                make_table("leads",
                                           make_column("id", &Lead::id, primary_key()),
                                           make_column("first_name", &Lead::firstName),
                                           make_column("last_name", &Lead::lastName),
                                           make_column("email", &Lead::email),
                                           make_column("phone", &Lead::phone)),

                                //  CREATE TABLE lead_logs (
                                //      id INTEGER PRIMARY KEY,
                                //      old_id int,
                                //      new_id int,
                                //      old_phone text,
                                //      new_phone text,
                                //      old_email text,
                                //      new_email text,
                                //      user_action text,
                                //      created_at text
                                //  );
                                make_table("lead_logs",
                                           make_column("id", &LeadLog::id, primary_key()),
                                           make_column("old_id", &LeadLog::oldId),
                                           make_column("new_id", &LeadLog::newId),
                                           make_column("old_phone", &LeadLog::oldPhone),
                                           make_column("new_phone", &LeadLog::newPhone),
                                           make_column("old_email", &LeadLog::oldEmail),
                                           make_column("new_email", &LeadLog::newEmail),
                                           make_column("user_action", &LeadLog::userAction),
                                           make_column("created_at", &LeadLog::createdAt)));
    storage.sync_schema();

    //  Insert a row with an invalid email into the leads table:
    //
    //  INSERT INTO leads (first_name, last_name, email, phone)
    //  VALUES('John', 'Doe', 'jjj', '4089009334');
    try {
        storage.insert(Lead{0, "John", "Doe", "jjj", "4089009334"});
    } catch(const std::system_error& systemError) {
        cout << "error: " << systemError.what() << endl;
    }

    //  Insert a row with a valid email.
    //  INSERT INTO leads (first_name, last_name, email, phone)
    //  VALUES ('John', 'Doe', 'john.doe@sqlitetutorial.net', '4089009334');
    storage.insert(Lead{0, "John", "Doe", "john.doe@sqlitetutorial.net", "4089009334"});

    cout << "Leads:" << endl;
    for(auto& lead: storage.iterate<Lead>()) {
        cout << storage.dump(lead) << endl;
    }

    //  UPDATE leads
    //  SET last_name = 'Smith'
    //  WHERE id = 1;
    storage.update_all(set(c(&Lead::lastName) = "Smith"), where(c(&Lead::id) == 1));

    cout << "Logs count = " << storage.count<LeadLog>() << endl;

    //  UPDATE leads
    //  SET
    //     phone = '4089998888',
    //     email = 'john.smith@sqlitetutorial.net'
    //  WHERE id = 1;
    storage.update_all(set(c(&Lead::phone) = "4089998888", c(&Lead::email) = "john.smith@sqlitetutorial.net"),
                       where(c(&Lead::id) == 1));

    cout << "Logs count = " << storage.count<LeadLog>() << endl;

    for(auto& leadLog: storage.iterate<LeadLog>()) {
        cout << storage.dump(leadLog) << endl;
    }

    return 0;
}
