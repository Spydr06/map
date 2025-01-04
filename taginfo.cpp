#include <expat.h>
#include <filesystem>
#include <taginfo.hpp>

#include <map.hpp>
#include <log.hpp>

#include <regex>
#include <memory>
#include <fstream>
#include <cstring>
#include <iostream>

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

void parse_value(const std::string& value, std::string& tag, std::vector<std::string>& params) {
    auto start = value.begin(), end = value.end();
    auto tag_end = std::find(start, end, '[');
    tag.assign(start, tag_end);

    while(tag_end != end) {
        auto param_start = tag_end + 1;
        auto param_end = std::find(param_start, end, ']');

        if(param_end != end) {
            params.push_back(std::string(param_start, param_end));
            tag_end = param_end + 1;
        }
    }
}

CachedTag* Map::get_taginfo(const std::string& key, const std::string& value) {
    std::string tag;
    std::vector<std::string> params;
    parse_value(value, tag, params);

    for(auto& param : params) {
        tag += "-" + param;
    }

    if(key == "traffic_sign") {
        mlog::logln(mlog::DEBUG, "sign: %s", tag.c_str());
    }

    auto it = m_taginfo[key].find(tag);
    return it == m_taginfo[key].end() ? nullptr : &it->second;
}

std::vector<CachedTag*> Map::get_taginfos(const std::string& key, const std::string& value) {
    std::regex delimiter_regex("[,;]"); 

    std::sregex_token_iterator begin(value.begin(), value.end(), delimiter_regex, -1);
    std::sregex_token_iterator end;

    std::vector<CachedTag*> result;
    std::transform(begin, end, std::back_inserter(result), [this, key](const std::string& part) {
        return this->get_taginfo(key, part);
    });

    for(auto it = result.begin(); it != result.end();) {
        if(*it)
            it++;
        else
            it = result.erase(it);
    }

    return result;
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

int CachedTag::fetch() {
    auto* curl = curl_easy_init();
    if(!curl) {
        mlog::logln(mlog::ERROR, "Faild to initialize libcurl.");
        return EINVAL;
    }

    std::ofstream output_file(m_icon_path, std::ios::binary);
    if(!output_file) {
        mlog::logln(mlog::ERROR, "Failed to open file for writing: %s", m_icon_path.c_str());
        curl_easy_cleanup(curl);
        return ENOENT;
    }

    curl_easy_setopt(curl, CURLOPT_URL, m_icon_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_dest);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_file);

    if(auto res = curl_easy_perform(curl); res != CURLE_OK) {
        mlog::logln(mlog::ERROR, "Failed downloading %s: %s", m_icon_url.c_str(), curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        output_file.close();
        return ENOENT;
    }

    curl_easy_cleanup(curl);
    output_file.close();
    return 0;
}

int CachedTag::read_svg() {
    GError* error = NULL;
    RsvgHandle* handle = rsvg_handle_new_from_file(m_icon_path.c_str(), &error);
    if(error) {
        mlog::logln(mlog::ERROR, "Failed opening svg file `%s`: %s", m_icon_path.c_str(), error->message);
        return EINVAL;
    }

    RsvgRectangle viewport = {0, 0, m_dimensions.x, m_dimensions.y};

    m_cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, viewport.width, viewport.height);    
    cairo_t* cr = cairo_create(m_cairo_surface);

    rsvg_handle_render_document(handle, cr, &viewport, &error);
    if(error) {
        mlog::logln(mlog::ERROR, "Failed rendering svg image `%s`: %s", m_icon_path.c_str(), error->message);
        cairo_destroy(cr);
        cairo_surface_destroy(m_cairo_surface);
        return EINVAL;
    }

    cairo_destroy(cr);
    g_object_unref(handle);
    return 0;
}

int CachedTag::read_png() {
    m_cairo_surface = cairo_image_surface_create_from_png(m_icon_path.c_str());
    if(auto err = cairo_surface_status(m_cairo_surface)) {
        mlog::logln(mlog::ERROR, "Failed rendering png image `%s`: %s", m_icon_path.c_str(), cairo_status_to_string(err));
        return EINVAL;
    }

    m_dimensions.x = cairo_image_surface_get_width(m_cairo_surface);
    m_dimensions.y = cairo_image_surface_get_height(m_cairo_surface);
    return 0;
}

int CachedTag::create_texture() {
    if(!m_cairo_surface)
        return EINVAL;

    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    int width = cairo_image_surface_get_width(m_cairo_surface);
    int height = cairo_image_surface_get_height(m_cairo_surface);
    uint8_t* data = cairo_image_surface_get_data(m_cairo_surface);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    cairo_surface_destroy(m_cairo_surface);
    m_cairo_surface = nullptr;

    return 0;
}

int CachedTag::load_image() {
    if(m_state == TAG_STATE_PRESENT)
        return 0;

    auto last_slash_pos = m_icon_url.find_last_of('/');
    assert(last_slash_pos != std::string::npos);
    m_icon_path = CACHE_DIR + "/" + m_icon_url.substr(last_slash_pos + 1);

    std::filesystem::create_directory(CACHE_DIR); 

    if(std::filesystem::exists(m_icon_path))
        m_state = TAG_STATE_FETCHED;

    switch(m_state) {
        case TAG_STATE_UNLOADED:
            mlog::logln(mlog::INFO, "Fetching icon `%s`...", m_icon_url.c_str());
            if(int err = fetch())
                return err;
            m_state = TAG_STATE_FETCHED;
            
        // fallthrough
        case TAG_STATE_FETCHED:
            switch(image_format()) {
                case TAG_FORMAT_SVG:
                    mlog::logln(mlog::INFO, "Loading svg icon `%s`...", m_icon_path.c_str());
                    if(int err = read_svg())
                        return err;
                    break;
                case TAG_FORMAT_PNG:
                    mlog::logln(mlog::INFO, "Loading png icon `%s`...", m_icon_path.c_str());
                    if(int err = read_png())
                        return err;
                    break;
                default:
                    mlog::logln(mlog::ERROR, "Unknown image format `%s`", m_icon_path.c_str());
                    return EINVAL;
            }
            m_state = TAG_STATE_LOADED;

        // fallthrough
        case TAG_STATE_LOADED:
            if(int err = create_texture())
                return err;
            m_state = TAG_STATE_PRESENT;

        // fallthrough
        case TAG_STATE_PRESENT:
            break;
    }
    
    return 0;
}

