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
    writeInitCode(initCode, tables);
    for (const auto& tableNode : *tables) {
        Table table = boost::get<mstch::map>(tableNode);
        std::string tableName = boost::get<std::string>(table["name"]);
        std::ofstream header(m_outputDir + "/" + tableName + ".cpp", std::ios_base::trunc);
        writeHeader(header, table);
    }
}

void DatabaseConverter::writeInitCode(std::ostream &os, const DatabaseConverter::Tables &tables)
{
    std::string initTmpltLoc = m_templateDir + "/init_code.mustache";
    std::ifstream initTmpltFile(initTmpltLoc);
    if(!(initTmpltFile && initTmpltFile.is_open())) {
        throw std::runtime_error("can not open init code template from " + initTmpltLoc);
    }
    std::string initTmplt((std::istreambuf_iterator<char>(initTmpltFile)),
                     std::istreambuf_iterator<char>());

    os << mstch::render(initTmplt, *tables);
}

void DatabaseConverter::writeHeader(std::ostream &os, const DatabaseConverter::Table &table)
{

}
