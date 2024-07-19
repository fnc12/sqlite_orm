#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
using namespace sqlite_orm;

enum class Gender {
    Invalid,
    Male,
    Female,
};

struct SuperHero {
    int id;
    std::string name;
    Gender gender = Gender::Invalid;
};

std::string GenderToString(Gender gender) {
    switch(gender) {
        case Gender::Female:
            return "female";
        case Gender::Male:
            return "male";
        case Gender::Invalid:
            return "invalid";
    }
    throw std::domain_error("Invalid Gender enum");
}

std::optional<Gender> GenderFromString(const std::string& s) {
    if(s == "female") {
        return Gender::Female;
    } else if(s == "male") {
        return Gender::Male;
    }
    return std::nullopt;
}

template<>
struct sqlite_orm::type_printer<Gender> : public text_printer {};

template<>
struct sqlite_orm::statement_binder<Gender> {

    int bind(sqlite3_stmt* stmt, int index, const Gender& value) {
        return statement_binder<std::string>().bind(stmt, index, GenderToString(value));
        //  or return sqlite3_bind_text(stmt, index++, GenderToString(value).c_str(), -1, SQLITE_TRANSIENT);
    }
    void result(sqlite3_context* context, const Gender& value) const {
        return statement_binder<std::string>().result(context, GenderToString(value));
    }
};

template<>
struct sqlite_orm::field_printer<Gender> {
    std::string operator()(const Gender& t) const {
        return GenderToString(t);
    }
};

template<>
struct sqlite_orm::row_extractor<Gender> {
    Gender extract(const char* columnText) const {
        if(auto gender = GenderFromString(columnText)) {
            return *gender;
        } else {
            throw std::runtime_error("incorrect gender string (" + std::string(columnText) + ")");
        }
    }

    Gender extract(sqlite3_stmt* stmt, int columnIndex) const {
        auto str = sqlite3_column_text(stmt, columnIndex);
        return this->extract((const char*)str);
    }

    Gender extract(sqlite3_value* value) const {
        const unsigned char* str = sqlite3_value_text(value);
        return this->extract((const char*)str);
    }
};

struct IdentityFunction {

    Gender operator()(Gender gender) const {
        return gender;
    }

    static const std::string& name() {
        static const std::string result = "IDENTITY";
        return result;
    }
};

struct CustomConcatFunction {

    std::string operator()(const arg_values& args) const {
        std::string result;
        for(auto arg_value: args) {
            result += arg_value.get<std::string>();
        }
        return result;
    }

    static const std::string& name() {
        static const std::string result = "CUSTOM_CONCAT";
        return result;
    }
};

// This test case triggers the conceptual checks performed in all functions that use the row_extractor.
// Also it is representative for all stock row extractor specializations in regard to testing
// storage's machinery during select and function calls, and whether they return proper values or build objects with proper values.
TEST_CASE("Custom row extractors") {
    remove("cre.sqlite");
    struct fguard {
        ~fguard() {
            remove("cre.sqlite");
        }
    } g;
    auto storage = make_storage("cre.sqlite",
                                make_table("superheros",
                                           make_column("id", &SuperHero::id, primary_key()),
                                           make_column("name", &SuperHero::name),
                                           make_column("gender", &SuperHero::gender)));
    sqlite3* db = nullptr;
    storage.on_open = [&db](sqlite3* db_) {
        db = db_;
    };
    storage.open_forever();
    storage.sync_schema();

    storage.transaction([&storage]() {
        storage.insert(SuperHero{-1, "Batman", Gender::Male});
        storage.insert(SuperHero{-1, "Wonder woman", Gender::Female});
        storage.insert(SuperHero{-1, "Superman", Gender::Male});
        return true;
    });

    SECTION("object builder") {
        auto allSuperHeros = storage.get_all<SuperHero>(order_by(&SuperHero::gender));
        REQUIRE(allSuperHeros.at(0).gender == Gender::Female);
    }
    SECTION("rowset builder") {
        auto allGenders = storage.select(distinct(&SuperHero::gender), order_by(&SuperHero::gender));
        REQUIRE(allGenders.at(0) == Gender::Female);
    }
    SECTION("column text") {
        auto firstGender = select(distinct(&SuperHero::gender), order_by(&SuperHero::gender), limit(1));
        Gender gender = Gender::Invalid;
        internal::perform_exec(db, storage.dump(firstGender), &extract_single_value<Gender>, &gender);
        REQUIRE(gender == Gender::Female);
    }
    SECTION("udf") {
        storage.create_scalar_function<IdentityFunction>();
        auto genders = storage.select(func<IdentityFunction>(Gender::Male));
        REQUIRE(genders.at(0) == Gender::Male);
    }
    SECTION("dynamic-arg udf") {
        storage.create_scalar_function<CustomConcatFunction>();
        auto concatGenders = storage.select(func<CustomConcatFunction>(Gender::Male, Gender::Female));
        REQUIRE(concatGenders.at(0) == "malefemale");
    }
}
#endif
