#include "database_converter.h"
#include <streambuf>

DatabaseConverter::DatabaseConverter(const std::string &templateDir, const std::string &outputDir)
    :m_templateDir(templateDir)
    ,m_outputDir(outputDir)
{
}

void DatabaseConverter::writeAll(DatabaseConverter::Tables tables)
{
    std::ofstream initCode(m_outputDir + "/initcode.cpp", std::ios_base::trunc);
    writeInitCode(initCode,*tables);
    for (const auto& table : *tables) {
        std::string tableName = table["name"];
        std::ofstream header(m_outputDir + "/" + tableName + ".hpp", std::ios_base::trunc);
        writeHeader(header, table);
        std::ofstream impl(m_outputDir + "/" + tableName + ".cpp", std::ios_base::trunc);
        writeImplementation(impl, table);
    }
}

void DatabaseConverter::writeInitCode(std::ostream &os, const nlohmann::json &tables)
{
    executeTemplate(os, tables, "init_code.mustache");
}

void DatabaseConverter::writeHeader(std::ostream &os, const nlohmann::json &table)
{
    executeTemplate(os, table, "header.mustache");
}

void DatabaseConverter::writeImplementation(std::ostream &os, const nlohmann::json &table)
{
    executeTemplate(os, table, "implementation.mustache");
}

void DatabaseConverter::executeTemplate(std::ostream &os, const nlohmann::json &data, const std::string &templateName)
{
    std::string initTmpltLoc = m_templateDir + "/" + templateName;
    std::ifstream initTmpltFile(initTmpltLoc);
    if(!(initTmpltFile && initTmpltFile.is_open())) {
        throw std::runtime_error("can not open init code template from " + initTmpltLoc);
    }
    std::string initTmplt((std::istreambuf_iterator<char>(initTmpltFile)),
                     std::istreambuf_iterator<char>());

    try {
        inja::render_to(os, initTmplt, data);
    } catch(const std::exception& ex) {
        throw std::runtime_error("error while parsing " + templateName + " " + ex.what());
    }
}
