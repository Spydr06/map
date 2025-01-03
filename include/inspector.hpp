#pragma once

#include "way.hpp"

#include <memory>

class Map;

class Inspector {
public:
    Inspector() 
    {}

    void inspect_ui(Map* map, std::shared_ptr<Way> way);
private:

};

