#include <expat.h>
#include <filesystem>
#include <taginfo.hpp>

#include <map.hpp>
#include <log.hpp>

#include <memory>
#include <fstream>
#include <cstring>

#include <curl/curl.h>
#include <curl/easy.h>
#include <librsvg/rsvg.h>
#include <cairo.h>

static void XMLCALL enter_element(void* user_data, const XML_Char* name, const XML_Char**) {
    auto data = static_cast<TagData*>(user_data);

    data->m_current_attr = TAGATTR_UNKNOWN;

    if(std::memcmp(name, "tags", 4) == 0) {
        data->m_current_tag = Tag();
    }
    else if(std::memcmp(name, "key", 3) == 0)
        data->m_current_attr = TAGATTR_KEY;
    else if(std::memcmp(name, "value", 5) == 0)
        data->m_current_attr = TAGATTR_VALUE;
    else if(std::memcmp(name, "icon_url", 8) == 0)
        data->m_current_attr = TAGATTR_ICON_URL;
}

static void XMLCALL leave_element(void* user_data, const XML_Char* name) {
    auto data = static_cast<TagData*>(user_data);

    if(std::memcmp(name, "tags", 4) == 0)
        data->m_map->register_taginfo(data->m_current_tag.key, data->m_current_tag.value, data->m_current_tag.icon_url);
    data->m_current_attr = TAGATTR_UNKNOWN;
}

static void XMLCALL element_contents(void* user_data, const XML_Char* contents, int contents_len) {
    auto data = static_cast<TagData*>(user_data);

    if(!contents)
        return;

    switch(data->m_current_attr) {
        case TAGATTR_KEY:
            data->m_current_tag.key = std::string(contents, contents_len); 
            break;
        case TAGATTR_VALUE:
            data->m_current_tag.value = std::string(contents, contents_len); 
            break;
        case TAGATTR_ICON_URL:
            data->m_current_tag.icon_url = std::string(contents, contents_len);
            break;
        default:
            break;
    }
}

auto load_taginfo(const char* xml_path, std::shared_ptr<Map> map) -> int {
    mlog::logln(mlog::INFO, "Preprocessing tag info...");

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

    TagData data(map);

    XML_SetUserData(parser, static_cast<void*>(&data));
    XML_SetElementHandler(parser, enter_element, leave_element);
    XML_SetCharacterDataHandler(parser, element_contents);

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

static const std::string CACHE_DIR = "cache";

void Map::register_taginfo(std::string& key, std::string& value, std::string& icon_url) {
    m_taginfo[key][value].m_icon_url = icon_url;
}

static size_t write_to_dest(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* file = static_cast<std::ofstream*>(userdata);
    size_t total_size = size * nmemb;

    mlog::logln(mlog::INFO, "writing %zu bytes..", total_size);

    if(file->is_open()) {
        file->write(static_cast<char*>(ptr), total_size);
        return total_size;
    }
    return 0;
}

static int downloadSVG(std::string& url, std::string& dest_path) {
    auto* curl = curl_easy_init();
    if(!curl) {
        mlog::logln(mlog::ERROR, "Faild to initialize libcurl.");
        return EINVAL;
    }

    std::ofstream output_file(dest_path, std::ios::binary);
    if(!output_file) {
        mlog::logln(mlog::ERROR, "Failed to open file for writing: %s", dest_path.c_str());
        curl_easy_cleanup(curl);
        return ENOENT;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_dest);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_file);

    if(auto res = curl_easy_perform(curl); res != CURLE_OK) {
        mlog::logln(mlog::ERROR, "Failed downloading %s: %s", url.c_str(), curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        output_file.close();
        return ENOENT;
    }

    curl_easy_cleanup(curl);
    output_file.close();
    return 0;
}

static int renderSVG(std::string& svg_path, glm::vec2 dimensions, cairo_surface_t** surface) {
    *surface = NULL;

    GError* error = NULL;
    RsvgHandle* handle = rsvg_handle_new_from_file(svg_path.c_str(), &error);
    if(error) {
        mlog::logln(mlog::ERROR, "Failed opening svg file `%s`: %s", svg_path.c_str(), error->message);
        return EINVAL;
    }

    RsvgRectangle viewport = {0, 0, dimensions.x, dimensions.y};

    cairo_surface_t* cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, viewport.width, viewport.height);    
    cairo_t* cr = cairo_create(cairo_surface);

    rsvg_handle_render_document(handle, cr, &viewport, &error);
    if(error) {
        mlog::logln(mlog::ERROR, "Failed rendering svg image `%s`: %s", svg_path.c_str(), error->message);
        cairo_destroy(cr);
        cairo_surface_destroy(cairo_surface);
        return EINVAL;
    }

     
    *surface = cairo_surface;
    return 0;
}

static int loadSVG(cairo_surface_t* surface, GLuint* texture_id) {
    glGenTextures(1, texture_id);
    glBindTexture(GL_TEXTURE_2D, *texture_id);

    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    uint8_t* data = cairo_image_surface_get_data(surface);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
}

int CachedTag::load_image() {
    if(m_state == TAG_STATE_LOADED)
        return 0;

    auto last_slash_pos = m_icon_url.find_last_of('/');
    assert(last_slash_pos != std::string::npos);
    m_svg_path = CACHE_DIR + "/" + m_icon_url.substr(last_slash_pos + 1);

    std::filesystem::create_directory(CACHE_DIR); 

    if(std::filesystem::exists(m_svg_path))
        m_state = TAG_STATE_FETCHED;

    switch(m_state) {
        case TAG_STATE_UNLOADED:
            mlog::logln(mlog::INFO, "Fetching svg icon `%s`...", m_icon_url.c_str());
            if(int err = downloadSVG(m_icon_url, m_svg_path))
                return err;
            m_state = TAG_STATE_FETCHED;
            
        case TAG_STATE_FETCHED:
            mlog::logln(mlog::INFO, "Loading svg icon `%s`...", m_svg_path.c_str());
            if(int err = renderSVG(m_svg_path, {256, 256}, &m_cairo_surface))
                return err;
            m_state = TAG_STATE_RENDERED;
    
        case TAG_STATE_RENDERED:
            if(int err = loadSVG(m_cairo_surface, &m_texture_id))
                return err;

            m_state = TAG_STATE_LOADED;
            cairo_surface_destroy(m_cairo_surface);
            m_cairo_surface = nullptr;

        case TAG_STATE_LOADED:
            break;
    }
    
    return 0;
}

