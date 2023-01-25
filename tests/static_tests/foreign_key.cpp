#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include <type_traits>  //  std::is_same
#include <string>  //  std::string
#include <type_traits>  //  std::is_same

#include "static_tests_storage_traits.h"

using namespace sqlite_orm;

TEST_CASE("foreign key static") {
    struct FunctionDecl {
        std::string function_name;
        std::string file_path;
        std::string return_type;
    };
    struct FunctionDef {
        std::string function_name;
        std::string file_path;
        std::string return_type;
    };
    struct FunctionCall {
        unsigned int line_num;
        std::string called_function_name;
        std::string parent_function_name;
    };
    struct File {
        std::string path;
    };
    struct CppInclusion {
        std::string includer_path;
        std::string includee_path;
    };
    struct VarDecl {
        int id;
        std::string name;
        std::string type;
        bool is_global;
        std::string file_path;
    };
    auto cppInclusionIncluderPathFk = foreign_key(&CppInclusion::includer_path).references(&File::path);
    STATIC_REQUIRE(std::is_same<decltype(cppInclusionIncluderPathFk)::target_type, File>::value);
    STATIC_REQUIRE(std::is_same<decltype(cppInclusionIncluderPathFk)::source_type, CppInclusion>::value);

    auto cppInclusionIncludeePathFk = foreign_key(&CppInclusion::includee_path).references(&File::path);
    STATIC_REQUIRE(std::is_same<decltype(cppInclusionIncludeePathFk)::target_type, File>::value);
    STATIC_REQUIRE(std::is_same<decltype(cppInclusionIncludeePathFk)::source_type, CppInclusion>::value);

    auto functionDeclFilePathFk = foreign_key(&FunctionDecl::file_path).references(&File::path);
    STATIC_REQUIRE(std::is_same<decltype(functionDeclFilePathFk)::target_type, File>::value);
    STATIC_REQUIRE(std::is_same<decltype(functionDeclFilePathFk)::source_type, FunctionDecl>::value);

    auto varDeclFilePathFk = foreign_key(&VarDecl::file_path).references(&File::path);
    STATIC_REQUIRE(std::is_same<decltype(varDeclFilePathFk)::target_type, File>::value);
    STATIC_REQUIRE(std::is_same<decltype(varDeclFilePathFk)::source_type, VarDecl>::value);

    auto functionDefFilePathFk = foreign_key(&FunctionDef::file_path).references(&File::path);
    STATIC_REQUIRE(std::is_same<decltype(functionDefFilePathFk)::target_type, File>::value);
    STATIC_REQUIRE(std::is_same<decltype(functionDefFilePathFk)::source_type, FunctionDef>::value);

    auto functionCallParentFunctionName =
        foreign_key(&FunctionCall::parent_function_name).references(&FunctionDef::function_name);
    STATIC_REQUIRE(std::is_same<decltype(functionCallParentFunctionName)::target_type, FunctionDef>::value);
    STATIC_REQUIRE(std::is_same<decltype(functionCallParentFunctionName)::source_type, FunctionCall>::value);

    auto storage = make_storage({},
                                make_table("files", make_column("path", &File::path, primary_key())),
                                make_table("c_inclusions",
                                           make_column("includer_path", &CppInclusion::includer_path),
                                           make_column("includee_path", &CppInclusion::includee_path),
                                           primary_key(&CppInclusion::includer_path, &CppInclusion::includee_path),
                                           cppInclusionIncluderPathFk,
                                           cppInclusionIncludeePathFk),
                                make_table("func_decls",
                                           make_column("name", &FunctionDecl::function_name),
                                           make_column("file_path", &FunctionDecl::file_path),
                                           make_column("return_type", &FunctionDecl::return_type),
                                           functionDeclFilePathFk,
                                           primary_key(&FunctionDecl::function_name, &FunctionDecl::file_path)),
                                make_table("var_decls",
                                           make_column("id", &VarDecl::id, primary_key().autoincrement()),
                                           make_column("name", &VarDecl::name),
                                           make_column("type", &VarDecl::type),
                                           make_column("is_global", &VarDecl::is_global),
                                           make_column("file_path", &VarDecl::file_path),
                                           varDeclFilePathFk),
                                make_table("func_defs",
                                           make_column("name", &FunctionDef::function_name),
                                           make_column("file_path", &FunctionDef::file_path),
                                           make_column("return_type", &FunctionDef::return_type),
                                           functionDefFilePathFk,
                                           primary_key(&FunctionDef::function_name)),
                                make_table("func_calls",
                                           make_column("line_num", &FunctionCall::line_num),
                                           make_column("called_func_name", &FunctionCall::called_function_name),
                                           make_column("parent_func_name", &FunctionCall::parent_function_name),
                                           functionCallParentFunctionName,
                                           primary_key(&FunctionCall::called_function_name,
                                                       &FunctionCall::parent_function_name,
                                                       &FunctionCall::line_num)));
    storage.sync_schema();

    using Storage = decltype(storage);

    using namespace sqlite_orm::internal::storage_traits;
    {
        using FkTuple = storage_fk_references<Storage, FunctionCall>::type;
        using Expected = std::tuple<>;
        STATIC_REQUIRE(std::is_same<FkTuple, Expected>::value);
    }
    {
        using FkTuple = storage_fk_references<Storage, FunctionDef>::type;
        using Expected = std::tuple<FunctionCall>;
        STATIC_REQUIRE(std::is_same<FkTuple, Expected>::value);
    }
    {
        using FkTuple = storage_fk_references<Storage, VarDecl>::type;
        using Expected = std::tuple<>;
        STATIC_REQUIRE(std::is_same<FkTuple, Expected>::value);
    }
    {
        using FkTuple = storage_fk_references<Storage, FunctionDecl>::type;
        using Expected = std::tuple<>;
        STATIC_REQUIRE(std::is_same<FkTuple, Expected>::value);
    }
    {
        using FkTuple = storage_fk_references<Storage, CppInclusion>::type;
        using Expected = std::tuple<>;
        STATIC_REQUIRE(std::is_same<FkTuple, Expected>::value);
    }
    {
        using FkTuple = storage_fk_references<Storage, File>::type;
        using Expected = std::tuple<CppInclusion, CppInclusion, FunctionDecl, VarDecl, FunctionDef>;
        STATIC_REQUIRE(std::is_same<FkTuple, Expected>::value);
    }
}
