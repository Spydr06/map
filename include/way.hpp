#pragma once

#include "viewport.hpp"
#include "flags.hpp"

#include <vector>
#include <unordered_map>
#include <string>

#include <GL/glew.h>

struct Metadata {
    enum Classification : GLbyte {
        UNKNOWN = 0,
        HIGHWAY_MOTORWAY,
        HIGHWAY_TRUNK,
        HIGHWAY_PRIMARY,
        HIGHWAY_SECONDARY,
        HIGHWAY_TERTIARY,
        HIGHWAY_UNCLASSIFIED,
        HIGHWAY_RESIDENTIAL,
        HIGHWAY_LIVING_STREET,
        HIGHWAY_SERVICE,
        HIGHWAY_PEDESTRIAN,
        HIGHWAY_TRACK,
        HIGHWAY_BUSWAY,
        HIGHWAY_FOOTWAY,
        HIGHWAY_CYCLEWAY,

        FOOTWAY_SIDEWALK,
        FOOTWAY_CROSSING,

        RAILWAY,
        WATERWAY,

        POWER_LINE,
        POWER_DISTRIBUTION
    };

    Metadata(std::unordered_map<std::string, std::string>& tags);
    Metadata()
        : m_classification(Classification::UNKNOWN)
    {}

    inline bool is_footway() const {
        return m_classification == FOOTWAY_SIDEWALK || m_classification == FOOTWAY_CROSSING || m_classification == HIGHWAY_FOOTWAY;
    }

    inline bool is_building() const {
        return m_classification == UNKNOWN || m_classification == POWER_DISTRIBUTION; // TODO
    }

    inline bool is_track() const {
        return m_classification == HIGHWAY_TRACK || m_classification == HIGHWAY_UNCLASSIFIED;
    }

    Classification m_classification;
    GLbyte m_line_width = 1;

    GLshort __padding;
};

static_assert(sizeof(Metadata) == sizeof(GLuint));

struct Node {
    typedef uint64_t Id;

    Node(glm::vec2 coord)
        : m_coord(coord), m_metadata()
    {}
    
    Node(glm::vec2 coord, std::unordered_map<std::string, std::string>& tags)
        : m_coord(coord), m_metadata(tags)
    {}

    glm::vec2 m_coord;
    Metadata m_metadata;
};

class Way : public BBox {
public:
    typedef uint64_t Id;

    Way(Id id) : m_nodes(), m_metadata(), m_id(id)
    {}

    Way(const Way &) = delete;

    ~Way() {
        if(m_vao)
            glDeleteVertexArrays(1, &m_vao);
        if(m_vbo)
            glDeleteVertexArrays(1, &m_vbo);
    }

    void create_buffers();

    void draw_buffers(RenderFlags flags);

    inline void add_node(Node node) {
        increase_bbox(node.m_coord);
        m_nodes.push_back(node);
    }

    inline auto& get_nodes() {
        return m_nodes;
    }

    inline void set_metadata(Metadata metadata) {
        m_metadata = metadata;
    }

    inline auto get_id() const -> Id {
        return m_id;
    }
    
private:
    std::vector<Node> m_nodes;
    Metadata m_metadata;

    GLuint m_vao = 0, m_vbo = 0;

    Id m_id;
};

