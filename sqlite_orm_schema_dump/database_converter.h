#ifndef DATABASE_CONVERTER_H
#define DATABASE_CONVERTER_H

#include <mstch/mstch.hpp>
#include <memory>
#include <fstream>

class DatabaseConverter
{
public:
    typedef std::shared_ptr<mstch::array> Tables;
    typedef mstch::map Table;

    DatabaseConverter(const std::string& templateDir, const std::string& outputDir);

    void writeAll(Tables tables);
    void writeInitCode(std::ostream& os, const Tables& tables);
    void writeHeader(std::ostream& os, const Table& table);
    void writeImplementation(std::ostream& os, const Table& table);

private:
    std::string m_templateDir;
    std::string m_outputDir;
};

#endif // DATABASE_CONVERTER_H
