#include "preprocess.hpp"
#include "renderutil.hpp"
#include "way.hpp"
#include "log.hpp"

#include <cassert>
#include <cmath>
#include <fstream>
#include <cstring>

#include <expat.h>
#include <memory>
#include <string>

static void XMLCALL enter_element(void* user_data, const XML_Char* name, const XML_Char** atts) {
    auto data = static_cast<PreData*>(user_data);

    if(std::memcmp(name, "node", 4) == 0) {
        const XML_Char* id = nullptr, *lat = nullptr, *lon = nullptr;
        for(int i = 0; atts[i]; i += 2) {
            if(std::memcmp(atts[i], "id", 2) == 0)
                id = atts[i + 1];
            else if(std::memcmp(atts[i], "lat", 3) == 0)
                lat = atts[i + 1];
            else if(std::memcmp(atts[i], "lon", 3) == 0)
                lon = atts[i + 1];
        }
        
        assert(id && lat && lon);
        data->m_node_cache->add_node(std::stoull(id), Node(map_project(glm::vec2(std::stof(lon), std::stof(lat)))));
    }
    else if(std::memcmp(name, "way", 3) == 0) {
        const XML_Char* id = nullptr;
        for(int i = 0; atts[i]; i += 2) {
            if(std::memcmp(atts[i], "id", 2) == 0)
                id = atts[i + 1];
        }

        assert(id);
        assert(data->m_current_way == nullptr);

        data->m_current_way = std::make_shared<Way>(std::stoull(id));
    }
    else if(std::memcmp(name, "nd", 2) == 0) {
        assert(atts[2] == nullptr);

        Node::Id node_ref = std::stoull(atts[1]);
        auto& node = data->m_node_cache->lookup(node_ref);

        data->m_current_way->add_node(node);
    }
    else if(data->m_current_way != nullptr && std::memcmp(name, "tag", 3) == 0) {
        assert(atts[4] == nullptr && atts[0][0] == 'k' && atts[2][0] == 'v');
        data->m_current_way->add_tag(atts[1], atts[3]);
    }
    else if(std::memcmp(name, "bounds", 5) == 0) {
        const XML_Char *min_lon = nullptr, *max_lon = nullptr, *min_lat = nullptr, *max_lat = nullptr;
        for(int i = 0; atts[i]; i += 2) {
            if(std::memcmp(atts[i], "minlon", 6) == 0)
                min_lon = atts[i + 1];
            else if(std::memcmp(atts[i], "maxlon", 6) == 0)
                max_lon = atts[i + 1];
            else if(std::memcmp(atts[i], "minlat", 6) == 0)
                min_lat = atts[i + 1];
            else if(std::memcmp(atts[i], "maxlat", 6) == 0)
                max_lat = atts[i + 1];
        }

        assert(min_lon && max_lon && min_lat && max_lat);
        auto min_a = map_project(glm::vec2(std::stof(min_lon), std::stof(min_lat)));
        auto min_b = map_project(glm::vec2(std::stof(min_lon), std::stof(max_lat)));
        glm::vec2 min(std::min(min_a.x, min_b.x), min_a.y);

        auto max_a = map_project(glm::vec2(std::stof(max_lon), std::stof(max_lat)));
        auto max_b = map_project(glm::vec2(std::stof(max_lon), std::stof(min_lat)));
        glm::vec2 max(std::max(max_a.x, max_b.x), max_a.y);

        data->m_map->init_bvh(std::make_pair(min, max), 16);
    }
}

static void XMLCALL leave_element(void* user_data, const XML_Char* name) {
    auto data = static_cast<PreData*>(user_data);

    if(std::memcmp(name, "way", 3) == 0) {
        assert(data->m_current_way != nullptr);

        auto metadata = data->m_current_way->parse_metadata();
/*        if(metadata.m_classification == Metadata::UNKNOWN) {
            data->m_current_way = nullptr;
            return;
        } */

        for(auto& node : data->m_current_way->get_nodes()) {
            node.m_metadata = metadata;
        }

        data->m_current_way->create_buffers();

        data->m_map->add_way(std::move(data->m_current_way));
        data->m_current_way = nullptr;
    }
}

auto preprocess_data(const char* xml_path, std::shared_ptr<Map> map) -> int {
    auto input = std::ifstream(xml_path);
    if(!input.good()) {
        mlog::logln(mlog::ERROR, "Could not open `%s`", xml_path);
        return 1;
    }
    
    auto parser = XML_ParserCreate(nullptr);
    if(!parser) {
        mlog::logln(mlog::ERROR, "Could not create XML parser");
        return 1;
    }

    PreData data(map);

    XML_SetUserData(parser, static_cast<void*>(&data));
    XML_SetElementHandler(parser, enter_element, leave_element);

    int ret = 0;
    const auto buffer_size = 1024 * 1024;

    while(!input.eof()) {
        void* const buf = XML_GetBuffer(parser, buffer_size);
        if(!buf) {
            mlog::logln(mlog::ERROR, "Could not allocate buffer of size %d", buffer_size);
            ret = 1;
            goto cleanup;
        }
    
        mlog::log(mlog::INFO, "\r%zu MiB parsed", input.tellg() / 1024 / 1024);

        const auto bytes_read = input.readsome((char*) buf, buffer_size);
        if(!bytes_read)
            break;

        if(XML_ParseBuffer(parser, bytes_read, input.eof()) == XML_STATUS_ERROR) {
            mlog::logln(mlog::ERROR, "Parse error at line %lu:\n%s", XML_GetCurrentLineNumber(parser),
                XML_ErrorString(XML_GetErrorCode(parser)));
            ret = 1;
            goto cleanup;
        }
    }

    mlog::logln(mlog::INFO, "done.");

cleanup:
    XML_ParserFree(parser);
    input.close();

    return ret;
}
