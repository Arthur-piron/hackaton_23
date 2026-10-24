#include "core/SaveFile.hpp"

#include "raylib.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace {
constexpr const char* kFileName = "lastlight_save.txt";
constexpr const char* kHeader = "# LAST LIGHT : progression permanente (ossements, talents, records)";

bool isWritable(const std::string& path)
{
    const bool existed = FileExists(path.c_str());
    std::ofstream probe(path, std::ios::app);
    if (!probe.good()) return false;
    probe.close();
    if (!existed) std::remove(path.c_str());
    return true;
}

std::string trim(const std::string& text)
{
    const size_t first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) return "";
    const size_t last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}
}

std::string SaveFile::defaultPath()
{
    // GetApplicationDirectory() se termine déjà par un séparateur.
    const std::string beside = std::string(GetApplicationDirectory()) + kFileName;
    if (isWritable(beside)) return beside;
    for (const char* variable : {"APPDATA", "HOME"}) {
        if (const char* dir = std::getenv(variable)) {
            const std::string candidate = std::string(dir) + "/" + kFileName;
            if (isWritable(candidate)) return candidate;
        }
    }
    return kFileName;
}

bool SaveFile::load()
{
    values_.clear();
    std::ifstream in(path_);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const size_t equals = line.find('=');
        if (equals == std::string::npos) continue;
        values_[trim(line.substr(0, equals))] = trim(line.substr(equals + 1));
    }
    return true;
}

bool SaveFile::save() const
{
    std::ofstream out(path_, std::ios::trunc);
    if (!out) {
        TraceLog(LOG_WARNING, "SAVE: impossible d'écrire %s", path_.c_str());
        return false;
    }
    out << kHeader << '\n';
    for (const auto& [key, value] : values_) out << key << '=' << value << '\n';
    return out.good();
}

int SaveFile::getInt(const std::string& key, int fallback) const
{
    const auto it = values_.find(key);
    if (it == values_.end()) return fallback;
    char* end = nullptr;
    const long value = std::strtol(it->second.c_str(), &end, 10);
    return end != it->second.c_str() ? static_cast<int>(value) : fallback;
}

void SaveFile::setInt(const std::string& key, int value)
{
    values_[key] = std::to_string(value);
}
