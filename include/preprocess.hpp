#pragma once

#include "map.hpp"

#include <limits>
#include <memory>
#include <string>
#include <utility>

class NodeCache {
public:
    NodeCache() 
        : m_nodes({}), 
        m_min_coord(std::numeric_limits<float>::infinity()), m_max_coord(-std::numeric_limits<float>::infinity())
    {}

    inline void add_node(Node::Id id, Node node) {
        increase_bbox(node.coord);
        this->m_nodes.insert({id, node});
    }

    inline auto lookup(Node::Id id) -> Node& {
        return m_nodes[id];
    }

    inline auto& get_nodes() {
        return m_nodes;
    }

    inline auto get_minmax_coord() {
        return std::make_pair(m_min_coord, m_max_coord);
    }

private:
    inline void increase_bbox(glm::vec2& coord) {
        m_min_coord.x = std::min(m_min_coord.x, coord.x);
        m_min_coord.y = std::min(m_min_coord.y, coord.y);
        m_max_coord.x = std::max(m_max_coord.x, coord.x);
        m_max_coord.y = std::max(m_max_coord.y, coord.y);
    }

    std::unordered_map<Node::Id, Node> m_nodes;
    glm::vec2 m_min_coord, m_max_coord;
};

class WayCache {
public:
    WayCache()
        : m_tags({})
    {}

    inline void make_current(Way::Id id) {

        m_way = std::make_unique<Way>();
        m_way_id = id;
    }

    inline auto& get_current() {
        return m_way;
    }

    inline auto get_current_id() {
        return m_way_id;
    }

    inline bool has_current() {
        return m_way_id != 0;
    }

    inline void discard() {
        m_way_id = 0;
        m_way = nullptr;
        m_tags.clear();
    }
    
    inline auto reset() {
        auto way_id = m_way_id;
        auto way = std::move(m_way);
        discard();
        return std::make_pair(way_id, std::move(way));
    }

    inline void add_tag(std::string tag_key, std::string tag_value) {
        m_tags.insert({tag_key, tag_value});
    }

    inline bool has_tag(std::string tag_key) {
        return m_tags.find(tag_key) == m_tags.end();
    }

private:
    Way::Id m_way_id = 0;
    std::unique_ptr<Way> m_way = nullptr;

    std::unordered_map<std::string, std::string> m_tags;
};

struct PreData {
    PreData(std::shared_ptr<Map> map)
        : m_map(map), m_node_cache(std::make_unique<NodeCache>()), m_current_way()
    {}

    std::shared_ptr<Map> m_map;
    std::unique_ptr<NodeCache> m_node_cache;

    WayCache m_current_way;
};

auto preprocess_data(const char* xml_path, std::shared_ptr<Map> map) -> int;
