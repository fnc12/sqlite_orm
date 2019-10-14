#ifndef DATABASE_CONVERTER_H
#define DATABASE_CONVERTER_H

#include <nlohmann/json.hpp>
#include <inja.hpp>
#include <memory>
#include <fstream>

class DatabaseConverter
{
public:
    typedef std::shared_ptr<nlohmann::json> Tables;

    DatabaseConverter(const std::string& templateDir, const std::string& outputDir);

    void writeAll(Tables tables);
    void writeInitCode(std::ostream& os, const nlohmann::json& tables);
    void writeHeader(std::ostream& os, const nlohmann::json& table);
    void writeImplementation(std::ostream& os, const nlohmann::json& table);

private:
    std::string m_templateDir;
    std::string m_outputDir;
    void executeTemplate(std::ostream &os, const nlohmann::json &data, const std::string& templateName);
};

#endif // DATABASE_CONVERTER_H
