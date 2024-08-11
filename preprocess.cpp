#include "preprocess.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
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
        data->m_node_cache->add_node(std::stoull(id), Node{{std::stof(lon), std::stof(lat)}});
    }
    else if(std::memcmp(name, "way", 3) == 0) {
        const XML_Char* id = nullptr;
        for(int i = 0; atts[i]; i += 2) {
            if(std::memcmp(atts[i], "id", 2) == 0)
                id = atts[i + 1];
        }

        assert(id);
        assert(!data->m_current_way.has_current());

        data->m_current_way.make_current(std::stoull(id));
    }
    else if(std::memcmp(name, "nd", 2) == 0) {
        assert(atts[2] == nullptr);

        Node::Id node_ref = std::stoull(atts[1]);
        auto& node = data->m_node_cache->lookup(node_ref);

        data->m_current_way.get_current()->add_node(node);
    }
    else if(data->m_current_way.has_current() && std::memcmp(name, "tag", 3) == 0) {
        assert(atts[4] == nullptr && atts[0][0] == 'k' && atts[2][0] == 'v');
        data->m_current_way.add_tag(atts[1], atts[3]);
    }
}

static void XMLCALL leave_element(void* user_data, const XML_Char* name) {
    auto data = static_cast<PreData*>(user_data);

    if(std::memcmp(name, "way", 3) == 0) {
        assert(data->m_current_way.has_current());

        // only handle streets for now
        if(data->m_current_way.has_tag("highway")) {
            auto [way_id, way] = data->m_current_way.reset();

            data->m_map->add_way(way_id, std::move(way));
        }
        else {
            data->m_current_way.discard();
        }
    }
}

auto preprocess_data(const char* xml_path, std::shared_ptr<Map> map) -> int {
    auto input = std::ifstream(xml_path);
    if(!input.good()) {
        std::cerr << "Could not open `" << xml_path << "`" << std::endl;
        return 1;
    }
    
    auto parser = XML_ParserCreate(nullptr);
    if(!parser) {
        std::cerr << "Could not create XML parser" << std::endl;
        return 1;
    }

    PreData data(map);

    XML_SetUserData(parser, static_cast<void*>(&data));
    XML_SetElementHandler(parser, enter_element, leave_element);

    int ret = 0;
    const auto buffer_size = BUFSIZ * 10;

    while(!input.eof()) {
        void* const buf = XML_GetBuffer(parser, 1024 * 1024);
        if(!buf) {
            std::cerr << "Could not allocate buffer of size " << buffer_size << std::endl;
            ret = 1;
            goto cleanup;
        }
    
        std::cout << '\r' << input.tellg() / 1024 / 1024 << " MiB parsed";
        std::cout.flush();

        const auto bytes_read = input.readsome((char*) buf, buffer_size);
        if(!bytes_read)
            break;

        if(XML_ParseBuffer(parser, bytes_read, input.eof()) == XML_STATUS_ERROR) {
            std::cerr << "Parse error at line " << XML_GetCurrentLineNumber(parser) << ":" << std::endl
                << XML_ErrorString(XML_GetErrorCode(parser)) << std::endl;
            ret = 1;
            goto cleanup;
        }
    }

    std::cout << std::endl << "done." << std::endl;

cleanup:
    XML_ParserFree(parser);
    input.close();

    map->set_minmax_coord(data.m_node_cache->get_minmax_coord());

    return ret;
}
