#include "way.hpp"
#include "flags.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <string>
#include <unordered_map>

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
    if(railway != tags.end()) {
        m_classification = Metadata::Classification::RAILWAY;
    }
}

void Way::create_buffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    assert(m_vao != 0);
    assert(m_vbo != 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_nodes.size() * sizeof(Node), &m_nodes[0], GL_STATIC_DRAW);

    glBindVertexArray(m_vao);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Node), nullptr);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(Node), (void*) offsetof(Node, m_metadata));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Way::draw_buffers(RenderFlags flags) {
    if(!(flags & RenderFlags::FOOTWAYS) && m_metadata.is_footway())
        return;

    if(!(flags & RenderFlags::BUILDINGS) && m_metadata.is_building())
        return;

    glBindVertexArray(m_vao);
    glLineWidth(m_metadata.m_line_width);
    glDrawArrays(GL_LINE_STRIP, 0, m_nodes.size());
}

