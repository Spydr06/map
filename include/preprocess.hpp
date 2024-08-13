#pragma once

#include "bbox.hpp"
#include "map.hpp"

#include <memory>
#include <utility>
#include <unordered_map>

class NodeCache : public BBox {
public:
    NodeCache() 
        : m_nodes({})
    {}

    inline void add_node(Node::Id id, Node node) {
        increase_bbox(node.m_coord);
        this->m_nodes.insert({id, node});
    }

    inline auto lookup(Node::Id id) -> Node& {
        auto found = m_nodes.find(id);
        if(found == m_nodes.end())
            assert(false);
        else
            return found->second;
    }

    inline auto& get_nodes() {
        return m_nodes;
    }

private:
    inline void increase_bbox(glm::vec2& coord) {
        m_min_coord.x = std::min(m_min_coord.x, coord.x);
        m_min_coord.y = std::min(m_min_coord.y, coord.y);
        m_max_coord.x = std::max(m_max_coord.x, coord.x);
        m_max_coord.y = std::max(m_max_coord.y, coord.y);
    }

    std::unordered_map<Node::Id, Node> m_nodes;
};

struct PreData {
    PreData(std::shared_ptr<Map> map)
        : m_map(map), m_node_cache(std::make_unique<NodeCache>()), m_current_way()
    {}

    std::shared_ptr<Map> m_map;
    std::unique_ptr<NodeCache> m_node_cache;

    std::shared_ptr<Way> m_current_way;
};

auto preprocess_data(const char* xml_path, std::shared_ptr<Map> map) -> int;

