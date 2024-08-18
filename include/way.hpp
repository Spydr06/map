#pragma once

#include "viewport.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <GL/glew.h>

// draw priority, in descending order
enum DrawPriority {
    MOTORWAY,
    RAILWAY = MOTORWAY,
    RIVER = MOTORWAY,
    MAJOR_HIGHWAY,
    MINOR_HIGHWAY,
    AGRICULTURAL,
    INDUSTRIAL = AGRICULTURAL,
    LOCAL_STREET,
    POWER_LINE = LOCAL_STREET,
    COMMERCIAL = LOCAL_STREET,
    RESIDENTIAL_STREET, 
    RECREATIONAL = RESIDENTIAL_STREET,
    PATHWAY,
    CYCLEWAY,
    FOOTWAY,
    BUILDING,

    __DRAW_PRIO_LAST,
};

extern const DrawPriority classification_draw_priorities[];

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
        LAKE,

        LANDUSE_AGRICULTURAL,
        LANDUSE_FOREST,
        LANDUSE_INDUSTRIAL,
        LANDUSE_RECREATIONAL,
        LANDUSE_TRANSPORT,
        LANDUSE_COMMERCIAL,
        LANDUSE_RESIDENTIAL,

        POWER_LINE,
        POWER_DISTRIBUTION,

        __CLASSIFICATION_LAST
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

    inline auto draw_priority() const {
        return classification_draw_priorities[m_classification];
    }

    inline bool operator==(const Metadata& other) const {
        return m_classification == other.m_classification;
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

    inline bool operator==(const Node& other) const {
        return m_coord == other.m_coord && m_metadata == other.m_metadata;
    }

/*    void swap(const Node& other) {
        glm::vec2 aux_coord = other.m_coord;
        Metadata aux_metadata = other.m_metadata;
        other.m_coord = m_coord;
        other.m_metadata = m_metadata;
        m_coord = aux_coord;
        m_metadata = aux_metadata;
    }*/
    
    glm::vec2 m_coord;
    Metadata m_metadata;
};

enum WindingOrder {
    CLOCKWISE,
    COUNTER_CLOCKWISE,
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

    void draw_buffers();
    void draw_highlighted_buffers();

    inline void add_node(Node node) {
        increase_bbox(node.m_coord);
        m_nodes.push_back(node);
    }

    inline auto& get_nodes() {
        return m_nodes;
    }

    inline auto get_id() const -> Id {
        return m_id;
    }

    inline void add_tag(std::string key, std::string value) {
        m_tags.insert({key, value});
    }

    inline auto& get_tags() {
        return m_tags;
    }

    auto parse_metadata() -> Metadata {
        return m_metadata = Metadata(m_tags);
    }

    inline auto& get_metadata() const {
        return m_metadata;
    }
    
private:
    bool is_area() const;
    std::optional<std::vector<GLuint>> triangulate_polygon();

    inline size_t relevant_vertices_count() const {
        return m_nodes.size() > 1 && m_nodes.front() == m_nodes.back() ? m_nodes.size() - 1 : m_nodes.size();
    }

    inline size_t triangle_count() const {
        return relevant_vertices_count() - 2;
    }

    WindingOrder get_winding_order() const;

    std::vector<Node> m_nodes;
    Metadata m_metadata;

    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;

    Id m_id;

    std::unordered_map<std::string, std::string> m_tags;
    std::optional<std::vector<GLuint>> m_indices = std::nullopt;
};

