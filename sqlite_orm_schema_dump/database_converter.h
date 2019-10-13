#ifndef DATABASE_CONVERTER_H
#define DATABASE_CONVERTER_H

#include <mstch/mstch.hpp>
#include <memory>
#include <fstream>

class DatabaseConverter
{
public:
    typedef std::shared_ptr<mstch::array> Tables;

    DatabaseConverter(const std::string& templateDir, const std::string& outputDir);

    void writeAll(Tables tables);
    void writeInitCode(std::ostream& os, const mstch::node& tables);
    void writeHeader(std::ostream& os, const mstch::node &table);
    void writeImplementation(std::ostream& os, const mstch::node &table);

private:
    std::string m_templateDir;
    std::string m_outputDir;
    void executeTemplate(std::ostream &os, const mstch::node &data, const std::string& templateName);
};

#endif // DATABASE_CONVERTER_H
