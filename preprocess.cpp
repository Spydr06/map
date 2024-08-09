#include "preprocess.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>

#include <expat.h>
#include <memory>
#include <sstream>
#include <string>

struct context {
    std::ofstream output;
    std::shared_ptr<Map> map;
};

static void XMLCALL enter_element(void* user_data, const XML_Char* name, const XML_Char** atts) {
    auto* context = static_cast<struct context*>(user_data);

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
        context->map->add_node(std::stoull(id), Node{{std::stof(lat), std::stof(lon)}});
        context->output << id << ' ' << lat << ' ' << lon << std::endl;
    }
}

auto preprocess_data(const char* xml_path, const char* data_path, std::shared_ptr<Map> map) -> int {
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

    struct context context{
        std::ofstream(data_path),
        map
    };
    XML_SetUserData(parser, static_cast<void*>(&context));
    XML_SetElementHandler(parser, enter_element, nullptr);

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
    context.output.flush();
    context.output.close();

    return ret;
}

void load_preprocessed_data(std::ifstream& input, std::shared_ptr<Map> map) {
    std::string line;
    uint64_t line_num = 0;
    while(std::getline(input, line)) {
        line_num++;
        std::istringstream line_stream(line);
        Node::Id id;
        float lat, lon;
        if(!(line_stream >> id >> lat >> lon)) {
            std::cerr << "line " << line_num << ": parse error" << std::endl;
        }

        map->add_node(id, Node{{lat, lon}});
    }
}
