#pragma once

#include "way.hpp"

#include <memory>

class Inspector {
public:
    Inspector() 
    {}

    void inspect_ui(std::shared_ptr<Way> way);
private:

};

