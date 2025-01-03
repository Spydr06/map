#pragma once

#include <map.hpp>
#include <memory>

enum TagAttr {
    TAGATTR_KEY,
    TAGATTR_VALUE,
    TAGATTR_ICON_URL,
    TAGATTR_UNKNOWN
};

struct Tag {
    std::string key = "";
    std::string value = "";
    std::string icon_url = "";

    Tag() {}
};

struct TagData {
    TagData(std::shared_ptr<Map> map)
        : m_map(map)
    {}

    std::shared_ptr<Map> m_map;
    
    Tag m_current_tag;
    TagAttr m_current_attr;
};

auto load_taginfo(const char* xml_path, std::shared_ptr<Map> map) -> int;
