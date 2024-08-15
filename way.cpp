#include "way.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>

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
    DrawPriority::RAILWAY, // RAILWAY,
    DrawPriority::RIVER, // WATERWAY
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

    auto waterway = tags.find("waterway");
    if(waterway != tags.end())
        m_classification = Metadata::Classification::WATERWAY;

    auto power = tags.find("power");
    if(power != tags.end()) {
        auto classification = power_classification.find(power->second);
        if(classification != power_classification.end())
            m_classification = classification->second;
        else
            m_classification = Metadata::Classification::POWER_DISTRIBUTION;
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

/*    if(m_id == 270084823) {
        std::cout << "here" << std::endl;

        std::ofstream output("extract.txt");
        
        for(auto& node : m_nodes) {
            output << std::setprecision(9) << node.m_coord.x << "," << node.m_coord.y << std::endl;
        }

        output.close();
    } */
}

void Way::draw_buffers() {
    glBindVertexArray(m_vao);
    glLineWidth(m_metadata.m_line_width);
    glDrawArrays(GL_LINE_STRIP, 0, m_nodes.size());
}

void Way::draw_highlighted_buffers() {
    glBindVertexArray(m_vao);
    glLineWidth(4);
    glDrawArrays(GL_LINE_STRIP, 0, m_nodes.size());
}

