#pragma once

#include "way.hpp"
#include <set>

class Relation : public MapElement {
public:
    Relation(Id id)
        : MapElement(id), m_ways{}
    {}

    virtual void inspect(ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None) const override;

    virtual std::string element_type() const override {
        return "relation";
    }

    inline void add_way(Way::Id way) {
        m_ways.insert(way);
    }

    inline const std::set<Way::Id>& get_ways() const {
        return m_ways;
    }

private:
    std::set<Way::Id> m_ways;
};

