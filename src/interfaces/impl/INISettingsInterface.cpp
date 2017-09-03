#include "interfaces/impl/INISettingsInterface.h"

using namespace bkengine;

void INISettingsInterface::init()
{
}

void INISettingsInterface::loadFromFile(const std::string &fileName)
{
    std::string logPrefix = "INISettingsInterface::loadFromFile(const std::string &=" + fileName + "): ";
    std::ifstream file(fileName);
    withoutSections.clear();
    withSections.clear();

    if (file.good()) {
        std::string line;
        bool hasSection = false;
        std::string currentSection;

        while (std::getline(file, line)) {
            uint64_t openBracket = line.find('[');
            uint64_t closeBracket = line.find(']');
            uint64_t equalSign = line.find('=');

            if (openBracket != std::string::npos && closeBracket != std::string::npos && closeBracket > openBracket) {
                currentSection = line.substr(openBracket + 1, closeBracket - openBracket - 1);
                hasSection = true;
                Logger::debug << logPrefix << "Section \"" << currentSection << "\" found";
            } else if (equalSign != std::string::npos) {
                std::string key = line.substr(0, equalSign);
                std::string value = line.substr(equalSign + 1);

                if (hasSection) {
                    withSections[currentSection][key] = value;
                    Logger::debug << logPrefix << "Value \"" << value << "\" added to \"" << currentSection << "."
                                  << key << "\"";
                } else {
                    withoutSections[key] = value;
                    Logger::debug << logPrefix << "Value \"" << value << "\" added to \"" << key << "\"";
                }
            } else if (line.size() > 0 && line[0] == ';') {
                Logger::debug << logPrefix << "Comment \"" << line << "\" found";
            } else {
                Logger::warning << logPrefix << "Line \"" << line << "\" is invalid and will be ignored";
            }
        }

        file.close();
        Logger::info << logPrefix << "Successfully read " << count() << " values from file";
    } else {
        Logger::error << logPrefix << "Failed to open file";
    }
}

void INISettingsInterface::saveToFile(const std::string &fileName) const
{
    std::ofstream file(fileName);

    if (file.good()) {
        for (auto &pair : withoutSections) {
            file << pair.first << "=" << pair.second << std::endl;
        }

        file << std::endl;

        for (auto &section : withSections) {
            file << "[" << section.first << "]" << std::endl;

            for (auto &pair : section.second) {
                file << pair.first << "=" << pair.second << std::endl;
            }

            file << std::endl;
        }

        file.close();
    } else {
        Logger::error << "INISettingsInterface::saveToFile(const std::string &=" << fileName
                      << "): Failed to open file";
    }
}

std::string INISettingsInterface::get(const std::string &key) const
{
    if (!hasValue(key)) {
        Logger::error << "INISettingsInterface::get(const std::string &=" << key << "): Key not found";
        return "";
    }

    uint64_t dot = key.find('.');

    if (dot == std::string::npos) {
        return withoutSections.at(key);
    } else {
        std::string section = key.substr(0, dot);

        if (section.empty()) {
            Logger::error << "INISettingsInterface::get(const std::string &=" << key
                          << "): Section is empty (parameter key has to have a "
                             "format of \"section.key\")";
            return "";
        }

        std::string newKey = key.substr(dot + 1);

        if (newKey.empty()) {
            Logger::error << "INISettingsInterface::get(const std::string &=" << key
                          << "): Key is empty (parameter key has to have a format of "
                             "\"section.key\")";
            return "";
        }

        return withSections.at(section).at(newKey);
    }
}

std::string INISettingsInterface::remove(const std::string &key)
{
    if (!hasValue(key)) {
        Logger::error << "INISettingsInterface::remove(const std::string &=" << key << "): Key not found";
        return "";
    }

    uint64_t dot = key.find('.');
    std::string entry;

    if (dot == std::string::npos) {
        entry = withoutSections[key];
        withoutSections.erase(key);
    } else {
        std::string section = key.substr(0, dot);

        if (section.empty()) {
            Logger::error << "INISettingsInterface::remove(const std::string &=" << key
                          << "): Section is empty (parameter key has to have "
                             "a format of \"section.key\")";
            return "";
        }

        std::string newKey = key.substr(dot + 1);

        if (newKey.empty()) {
            Logger::error << "INISettingsInterface::remove(const std::string &=" << key
                          << "): Key is empty (parameter key has to have a "
                             "format of \"section.key\")";
            return "";
        }

        entry = withSections[section][newKey];
        withSections[section].erase(newKey);

        if (withSections[section].size() == 0) {
            withSections.erase(section);
        }
    }

    return entry;
}

bool INISettingsInterface::hasValue(const std::string &key) const
{
    if (key.empty()) {
        Logger::error << "INISettingsInterface::hasValue(const std::string &=" << key
                      << "): Parameter key is empty (it has to have a "
                         "format of \"section.key\")";
        return false;
    }

    uint64_t dot = key.find('.');

    if (dot == std::string::npos) {
        return withoutSections.find(key) != withoutSections.end();
    } else {
        std::string section = key.substr(0, dot);

        if (section.empty()) {
            Logger::error << "INISettingsInterface::hasValue(const std::string &=" << key
                          << "): Section is empty (parameter key has to have "
                             "a format of \"section.key\")";
            return false;
        }

        std::string newKey = key.substr(dot + 1);

        if (newKey.empty()) {
            Logger::error << "INISettingsInterface::hasValue(const std::string &=" << key
                          << "): Key is empty (parameter key has to have a "
                             "format of \"section.key\")";
            return false;
        }

        if (withSections.find(section) == withSections.end()) {
            return false;
        } else {
            return withSections.at(section).find(newKey) != withSections.at(section).end();
        }
    }
}

void INISettingsInterface::create(const std::string &key, const std::string &value)
{
    if (key.empty()) {
        Logger::error << "INISettingsInterface::create(const std::string &=" << key << ", const std::string &=" << value
                      << "): Parameter key is empty (it has to have a format of "
                         "\"section.key\")";
        return;
    }

    if (hasValue(key)) {
        Logger::error << "INISettingsInterface::create(const std::string &=" << key << ", const std::string &=" << value
                      << "): The key already exists!";
        return;
    }

    uint64_t dot = key.find('.');

    if (dot == std::string::npos) {
        withoutSections[key] = value;
    } else {
        std::string section = key.substr(0, dot);

        if (section.empty()) {
            Logger::error << "INISettingsInterface::create(const std::string &=" << key
                          << ", const std::string &=" << value
                          << "):Section is empty (parameter key has to have a format "
                             "of \"section.key\")";
            return;
        }

        std::string newKey = key.substr(dot + 1);

        if (newKey.empty()) {
            Logger::error << "INISettingsInterface::create(const std::string &=" << key
                          << ", const std::string &=" << value
                          << "): Key is empty (parameter key has to have a format of "
                             "\"section.key\")";
            return;
        }

        withSections[section][newKey] = value;
    }
}

void INISettingsInterface::change(const std::string &key, const std::string &newValue)
{
    if (!hasValue(key)) {
        Logger::error << "INISettingsInterface::change(const std::string &=" << key
                      << ", const std::string &=" << newValue << "): Key not found";
        return;
    }

    remove(key);
    create(key, newValue);
}

uint32_t INISettingsInterface::count() const
{
    uint32_t values = 0;

    for (auto &pairs : withSections) {
        values += pairs.second.size();
    }

    values += withoutSections.size();
    return values;
}
