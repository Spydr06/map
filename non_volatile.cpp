#include "non_volatile.hpp"

#include "log.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <cctype>

#include "inipp/inipp.h"

std::shared_ptr<non_volatile_store> settings_store = nullptr;

template<>
std::string non_volatile_store::serialize(const bool& value) {
    return value ? "true" : "false";
}

bool case_insensitive_eq(char a, char b)
{
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

template<>
std::optional<bool> non_volatile_store::deserialize(const std::string& v) {
    if(std::ranges::equal(v, std::string_view{"true"}, case_insensitive_eq))
        return true;
    if(std::ranges::equal(v, std::string_view{"false"}, case_insensitive_eq))
        return false;

    mlog::logln(mlog::ERROR, "parser error: value \"%s\" is not a bool.", v.c_str());
    return std::nullopt;
}

template<>
std::string non_volatile_store::serialize(const std::string& value) {
    return std::format("\"{}\"", value);
}

template<>
std::optional<std::string> non_volatile_store::deserialize(const std::string& v) {
    if(v.size() < 2 || v.front() != '"' || v.back() != '"') {
        mlog::logln(mlog::ERROR, "parser error: value \"%s\" is not a string.", v.c_str());
        return std::nullopt;
    }

    return v.substr(1, v.size() - 2);
}

template<>
std::string non_volatile_store::serialize(const int& value) {
    return std::format("{}", value);
}

template<>
std::optional<int> non_volatile_store::deserialize(const std::string& v) {
    int value{};

    const char *end = v.data() + v.size();
    auto [ptr, err] = std::from_chars(v.data(), end, value);
    if(err != std::errc{} || ptr != end) {
        mlog::logln(mlog::ERROR, "parser error: value \"%s\" is not an integer.", v.c_str());
        return std::nullopt;
    }

    return value;
}

template<>
std::string non_volatile_store::serialize(const size_t& value) {
    return std::format("{}", value);
}

template<>
std::optional<size_t> non_volatile_store::deserialize(const std::string& v) {
    size_t value{};

    const char *end = v.data() + v.size();
    auto [ptr, err] = std::from_chars(v.data(), end, value);
    if(err != std::errc{} || ptr != end) {
        mlog::logln(mlog::ERROR, "parser error: value \"%s\" is not an unsigned integer.", v.c_str());
        return std::nullopt;
    }

    return value;
}

template<>
std::string non_volatile_store::serialize(const float& value) {
    return std::format("{}", value);
}

template<>
std::optional<float> non_volatile_store::deserialize(const std::string& v) {
    const char* end = v.data() + v.size();
    float value{};

    auto [ptr, err] = std::from_chars(v.data(), end, value);
    if (err != std::errc{} || ptr != end) {
        mlog::logln(mlog::ERROR, "parser error: value \"%s\" is not a float.", v.c_str());
        return std::nullopt;
    }

    return value;
}

template<>
std::string non_volatile_store::serialize(const glm::vec4& value) {
    auto to_u8 = [&](float x) -> uint8_t {
        return static_cast<uint8_t>(std::lround(std::clamp(x, 0.0f, 1.0f) * 255.0f));
    };

    uint8_t r = to_u8(value.r);
    uint8_t g = to_u8(value.g);
    uint8_t b = to_u8(value.b);
    uint8_t a = to_u8(value.a);

    return std::format("#{:02X}{:02X}{:02X}{:02X}", r, g, b, a);
}

template<>
std::optional<glm::vec4> non_volatile_store::deserialize(const std::string& v) {
    if(v.size() != 9) {
        mlog::logln(mlog::ERROR, "parser error: value \"%s\" is not a color (#rrggbbaa).", v.c_str());
        return std::nullopt;
    }

    auto hex_u8 = [&](size_t pos) -> std::optional<uint8_t> {
        unsigned x{};
        auto first = v.data() + pos;
        auto [ptr, err] = std::from_chars(first, first + 2, x, 16);
        if (err != std::errc{} || ptr != first + 2 || x > 255)
            return std::nullopt;
        return static_cast<uint8_t>(x);
    };

    auto r = hex_u8(1);
    auto g = hex_u8(3);
    auto b = hex_u8(5);
    auto a = hex_u8(7);

    if(!r || !g || !b || !a) {
        mlog::logln(mlog::ERROR, "parser error: value \"%s\" is not a color (#rrggbbaa).", v.c_str());
        return std::nullopt;
    }

    return glm::vec4(*r / 255.0f, *g / 255.0f, *b / 255.0f, *a / 255.0f);
}

ini_store::ini_store(const std::filesystem::path& path)
    : m_path(path)
{
    if(!std::filesystem::exists(path))
        return;

    mlog::logln(mlog::INFO, "loading non-volatile store from \"%s\"...", path.c_str());

    std::ifstream is(path);
    if(!is.is_open()) {
        mlog::logln(mlog::ERROR, "could not read non-volatile store from \"%s\".", path.c_str());
        return;
    }

    inipp::Ini<char> ini;
    ini.parse(is);

    for(const auto& [section_name, section] : ini.sections) {
        for(const auto& [key, value] : section) {
            if(section_name.empty())
                m_fields[key] = value;
            else
                m_fields[std::format("{}.{}", section_name, key)] = value;
        }
    }
}

static std::pair<std::string, std::string> split_key(const std::string& full_key) {
    if(const auto pos = full_key.find('.'); pos != std::string::npos)
        return {full_key.substr(0, pos), full_key.substr(pos + 1)};
    return {"", full_key};
}

ini_store::~ini_store() {
    std::cout << "saving non-volatile store to " << m_path << "..." << std::endl;

    std::ofstream os(m_path);
    if(!os.is_open())
        return;

    inipp::Ini<char> ini;

    for(const auto& [full_key, value] : m_fields) {
        auto [section, key] = split_key(full_key);
        ini.sections[section][key] = value;
    }

    ini.generate(os);
}
