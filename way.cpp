#include "way.hpp"
#include "log.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <numeric>
#include <string>
#include <unordered_map>
#include <fstream>
#include <meta>

const DrawPriority classification_draw_priorities[] {
    DrawPriority::BUILDING, // UNKNOWN
    DrawPriority::MOTORWAY, // HIGHWAY_MOTORWAY
    DrawPriority::MAJOR_HIGHWAY, // HIGHWAY_TRUNK
    DrawPriority::MAJOR_HIGHWAY, // HIGHWAY_PRIMARY
    DrawPriority::MINOR_HIGHWAY, // HIGHWAY_SECONDARY
    DrawPriority::MINOR_HIGHWAY, // HIGHWAY_TERTIARY
    DrawPriority::LOCAL_STREET, // HIGHWAY_UNCLASSIFIED
    DrawPriority::RESIDENTIAL_STREET, // HIGHWAY_RESIDENTIAL
    DrawPriority::RESIDENTIAL_STREET, // HIGHWAY_LIVING_STREET
    DrawPriority::LOCAL_STREET, // HIGHWAY_SERVICE
    DrawPriority::RESIDENTIAL_STREET, // HIGHWAY_PEDESTIRAN
    DrawPriority::PATHWAY, // HIGHWAY_TRACK
    DrawPriority::LOCAL_STREET, // HIGHWAY_BUSWAY
    DrawPriority::FOOTWAY, // FOOTWAY
    DrawPriority::CYCLEWAY, // CYCLEWAY
    DrawPriority::FOOTWAY, // FOOTWAY_SIDEWALK
    DrawPriority::FOOTWAY, // FOOTWAY_CROSSING
    DrawPriority::RAILWAY, // RAILWAY
    DrawPriority::RIVER, // WATERWAY
    DrawPriority::RIVER, // WATER
    DrawPriority::AGRICULTURAL, // LANDUSE_AGRICULTURAL
    DrawPriority::AGRICULTURAL, // LANDUSE_FOREST
    DrawPriority::INDUSTRIAL, // LANDUSE_INDUSTRIAL
    DrawPriority::RECREATIONAL, // LANDUSE_RECREATIONAL
    DrawPriority::INDUSTRIAL, // LANUSE_TRANSPORT
    DrawPriority::COMMERCIAL, // LANDUSE_COMMERCIAL
    DrawPriority::RECREATIONAL, // LANDUSE_RESIDENTIAL
    DrawPriority::AERIALWAY, // AERIALWAY_GONDOLA
    DrawPriority::POWER_LINE, // POWER_LINE
    DrawPriority::BUILDING, // POWER_DISTRIBUTION
};

static_assert(sizeof(classification_draw_priorities) / sizeof(DrawPriority) == Metadata::__CLASSIFICATION_LAST);

static std::unordered_map<std::string, Metadata::Classification> highway_classifications({
    {"motorway", Metadata::Classification::HIGHWAY_MOTORWAY},
    {"motorway_link", Metadata::Classification::HIGHWAY_MOTORWAY},
    {"motorway_junction", Metadata::Classification::HIGHWAY_MOTORWAY},
    {"trunk", Metadata::Classification::HIGHWAY_TRUNK},
    {"trunk_link", Metadata::Classification::HIGHWAY_TRUNK},
    {"primary", Metadata::Classification::HIGHWAY_PRIMARY},
    {"primary_link", Metadata::Classification::HIGHWAY_PRIMARY},
    {"secondary", Metadata::Classification::HIGHWAY_SECONDARY},
    {"secondary_link", Metadata::Classification::HIGHWAY_SECONDARY},
    {"tertiary", Metadata::Classification::HIGHWAY_TERTIARY},
    {"tertiary_link", Metadata::Classification::HIGHWAY_TERTIARY},
    {"unclassified", Metadata::Classification::HIGHWAY_UNCLASSIFIED},
    {"residential", Metadata::Classification::HIGHWAY_RESIDENTIAL},
    {"living_street", Metadata::Classification::HIGHWAY_LIVING_STREET},
    {"service", Metadata::Classification::HIGHWAY_SERVICE},
    {"pedestrian", Metadata::Classification::HIGHWAY_PEDESTRIAN},
    {"track", Metadata::Classification::HIGHWAY_TRACK},
    {"bus_guideway", Metadata::Classification::HIGHWAY_BUSWAY},
    {"busway", Metadata::Classification::HIGHWAY_BUSWAY},
    {"footway", Metadata::Classification::HIGHWAY_FOOTWAY},
    {"cycleway", Metadata::Classification::HIGHWAY_CYCLEWAY},
    {"crossing", Metadata::Classification::FOOTWAY_CROSSING},
});

static std::unordered_map<Metadata::Classification, GLbyte> highway_widths({
    {Metadata::Classification::HIGHWAY_MOTORWAY, 3},
    {Metadata::Classification::HIGHWAY_TRUNK, 3},
    {Metadata::Classification::HIGHWAY_PRIMARY, 2},
    {Metadata::Classification::HIGHWAY_SECONDARY, 2},
    {Metadata::Classification::HIGHWAY_TERTIARY, 2},
});

static std::unordered_map<std::string, Metadata::Classification> footway_classification({
    {"sidewalk", Metadata::Classification::FOOTWAY_SIDEWALK},
    {"crossing", Metadata::Classification::FOOTWAY_CROSSING}
});

static std::unordered_map<std::string, Metadata::Classification> landuse_classification({
    {"farmland", Metadata::Classification::LANDUSE_AGRICULTURAL},
    {"meadow", Metadata::Classification::LANDUSE_AGRICULTURAL},
    {"orchard", Metadata::Classification::LANDUSE_AGRICULTURAL},
    {"vineyard", Metadata::Classification::LANDUSE_AGRICULTURAL},
    {"greenhouse_horticulture", Metadata::Classification::LANDUSE_AGRICULTURAL},
    {"farmyard", Metadata::Classification::LANDUSE_AGRICULTURAL},
    {"aquaculture", Metadata::Classification::WATER},
    {"forest", Metadata::Classification::LANDUSE_FOREST},
    {"wood", Metadata::Classification::LANDUSE_FOREST},
    {"scrub", Metadata::Classification::LANDUSE_FOREST},
    {"quarry", Metadata::Classification::LANDUSE_INDUSTRIAL},
    {"park", Metadata::Classification::LANDUSE_RECREATIONAL},
    {"garden", Metadata::Classification::LANDUSE_RECREATIONAL},
    {"grass", Metadata::Classification::LANDUSE_RECREATIONAL},
    {"recreation_ground", Metadata::Classification::LANDUSE_RECREATIONAL},
    {"industrial", Metadata::Classification::LANDUSE_INDUSTRIAL},
    {"railway", Metadata::Classification::LANDUSE_TRANSPORT},
    {"port", Metadata::Classification::LANDUSE_INDUSTRIAL},
    {"depot", Metadata::Classification::LANDUSE_TRANSPORT},
    {"reservoir", Metadata::Classification::WATER},
    {"commercial", Metadata::Classification::LANDUSE_COMMERCIAL},
    {"residential", Metadata::Classification::LANDUSE_RESIDENTIAL},
    {"retail", Metadata::Classification::LANDUSE_COMMERCIAL},
});

static std::unordered_map<std::string, Metadata::Classification> power_classification({
    {"line", Metadata::Classification::POWER_LINE},
    {"minor_line", Metadata::Classification::POWER_LINE},
    {"cable", Metadata::Classification::POWER_LINE},
    {"tower", Metadata::Classification::POWER_DISTRIBUTION},
    {"transformer", Metadata::Classification::POWER_DISTRIBUTION},
    {"substation", Metadata::Classification::POWER_DISTRIBUTION}
});

Metadata::Metadata(std::unordered_map<std::string, std::string>& tags) {
    m_classification = Metadata::UNKNOWN;

    auto highway = tags.find("highway");
    if(highway != tags.end()) {
        auto classification = highway_classifications.find(highway->second);
        if(classification == highway_classifications.end())
            m_classification = Metadata::Classification::HIGHWAY_UNCLASSIFIED;
        else
            m_classification = classification->second;

        m_line_width = std::max(highway_widths[m_classification], GLbyte(1));
    }

    auto footway = tags.find("footway");
    if(footway != tags.end()) {
        auto classification = footway_classification.find(footway->second);
        if(classification != footway_classification.end())
            m_classification = classification->second;
    }

    auto railway = tags.find("railway");
    if(railway != tags.end())
        m_classification = Metadata::Classification::RAILWAY;

    auto landuse = tags.find("landuse");
    if(landuse != tags.end()) {
        auto classification = landuse_classification.find(landuse->second);
        if(classification != landuse_classification.end())
            m_classification = classification->second;
    }

    auto waterway = tags.find("waterway");
    if(waterway != tags.end())
        m_classification = Metadata::Classification::WATERWAY;

    auto water = tags.find("water");
    if(water != tags.end())
        m_classification = Metadata::Classification::WATER;

    auto power = tags.find("power");
    if(power != tags.end()) {
        auto classification = power_classification.find(power->second);
        if(classification != power_classification.end())
            m_classification = classification->second;
        else
            m_classification = Metadata::Classification::POWER_DISTRIBUTION;
    }

    auto aerialway = tags.find("aerialway");
    if(aerialway != tags.end())
        m_classification = Metadata::Classification::AERIALWAY_GONDOLA;
}

void Way::create_buffers() {
    glGenBuffers(1, &m_vbo);

    assert(m_vbo != 0);

    if((m_indices = triangulate_polygon())) {
    //if(false) {
        glGenBuffers(1, &m_ebo);
        assert(m_ebo != 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices->size() * sizeof(GLint), &(*m_indices)[0], GL_STATIC_DRAW);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_nodes.size() * sizeof(Node), &m_nodes[0], GL_STATIC_DRAW);
}

void Way::rebuild_vaos() {
    if(m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    glGenVertexArrays(1, &m_vao);
    assert(m_vao != 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    glBindVertexArray(m_vao);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Node), nullptr);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(Node), (void*) offsetof(Node, m_metadata));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void Way::draw_buffers(float scale) {
    glBindVertexArray(m_vao);
    glLineWidth(m_metadata.m_line_width * 2.0f * scale);
    
    if(m_ebo)
        glDrawElements(GL_TRIANGLES, m_indices->size(), GL_UNSIGNED_INT, m_indices->data());
    else
        glDrawArrays(GL_LINE_STRIP, 0, m_nodes.size());
}

void Way::draw_highlighted_buffers(float scale) {
    glBindVertexArray(m_vao);
    glLineWidth(4.0f * scale);
    glDrawArrays(GL_LINE_STRIP, 0, m_nodes.size());
}

bool Way::is_area() const {
    return (
        // m_tags.find("area") != m_tags.end() ||
        m_metadata.m_classification == Metadata::Classification::LANDUSE_FOREST ||
        m_metadata.m_classification == Metadata::Classification::LANDUSE_AGRICULTURAL ||
        m_metadata.m_classification == Metadata::Classification::LANDUSE_RECREATIONAL ||
        m_metadata.m_classification == Metadata::Classification::WATER
    ) && m_nodes.front() == m_nodes.back();
}

static inline double cross_product_z(glm::dvec2 a, glm::dvec2 b) {
    return a.x * b.y - a.y * b.x;
}

static inline bool is_point_in_triangle(glm::dvec2 p, glm::dvec2 a, glm::dvec2 b, glm::dvec2 c) {
    glm::dvec2 ab = b - a, bc = c - b, ca = a - c;
    glm::dvec2 ap = p - a, bp = p - b, cp = p - c;

    return cross_product_z(ab, ap) <= 0.0f && cross_product_z(bc, bp) <= 0.0f && cross_product_z(ca, cp) <= 0.0f;
}

static inline GLuint get_index(const std::vector<GLuint>& indices, std::int64_t i) {
    return indices[(i + indices.size()) % indices.size()];
}

std::optional<std::vector<GLuint>> Way::triangulate_polygon() {
    if(!is_area() || relevant_vertices_count() < 2 || triangle_count() < 3)
        return std::nullopt;

    if(get_winding_order() != WindingOrder::CLOCKWISE)
        std::reverse(m_nodes.begin(), m_nodes.end());

    std::vector<GLuint> remaining_indices(relevant_vertices_count()); // TODO: maybe set<int>?
    std::iota(remaining_indices.begin(), remaining_indices.end(), 0);

    std::vector<GLuint> indices(triangle_count() * 3);

    glm::dvec2 origin = m_nodes[0].m_coord;
    
    while(remaining_indices.size() > 3) {
        bool ear_found = false;

        for(std::int64_t i = remaining_indices.size() - 1; i >= 0; i--) {
            GLuint a = get_index(remaining_indices, i);
            GLuint b = get_index(remaining_indices, i - 1);
            GLuint c = get_index(remaining_indices, i + 1);

            glm::dvec2 va = glm::dvec2(m_nodes[a].m_coord) - origin;
            glm::dvec2 vb = glm::dvec2(m_nodes[b].m_coord) - origin;
            glm::dvec2 vc = glm::dvec2(m_nodes[c].m_coord) - origin;

            if(cross_product_z(vb - va, vc - va) < 0.0)
                continue;

            bool is_ear = true;

            for(GLuint j : remaining_indices) {
                if(j == a || j == b || j == c)
                    continue;

                glm::dvec2 p = glm::dvec2(m_nodes[j].m_coord) - origin;

                if(is_point_in_triangle(p, vb, va, vc)) {
                    is_ear = false;
                    break;
                }
            }

            if(is_ear) {
                indices.push_back(b);
                indices.push_back(a);
                indices.push_back(c);

                remaining_indices.erase(remaining_indices.begin() + i);
                ear_found = true;
                break;
            }
        }

        if(!ear_found) {
            mlog::logln(mlog::WARN, "Way %lu: no suitable polygon", m_id);
            return std::nullopt;
        }
    }

    indices.push_back(remaining_indices[0]);
    indices.push_back(remaining_indices[1]);
    indices.push_back(remaining_indices[2]);

    return indices;
}

WindingOrder Way::get_winding_order() const {
    double sum = 0.0;
    for(size_t i = 0; i < m_nodes.size(); i++) {
        glm::dvec2 cur = m_nodes[i].m_coord;
        glm::dvec2 next = m_nodes[(i + 1) % m_nodes.size()].m_coord;

        sum += (next.x - cur.x) * (next.y + cur.y);
    }

    return sum > 0.0 ? WindingOrder::CLOCKWISE : WindingOrder::COUNTER_CLOCKWISE;
}


auto Metadata::classification_name(Metadata::Classification classification) -> std::optional<std::string> {
    template for(constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^Metadata::Classification))) {
        if(classification == [: e :]) 
            return std::string(std::meta::identifier_of(e));
    }

    return std::nullopt;
}

