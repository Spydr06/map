#include "renderutil.hpp"
#include <cmath>

static const double EARTH_R = 6378.137;

double measure_latlon_dist(glm::vec2 from, glm::vec2 to) {
    auto d = deg_to_rad(to) - deg_to_rad(from);
    
    double a = std::sin(d.y / 2.0) * std::sin(d.y / 2.0) +
        std::cos(deg_to_rad(from.y)) * std::cos(deg_to_rad(to.y)) *
        std::sin(deg_to_rad(from.x)) * std::sin(deg_to_rad(to.x));

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    double dd = EARTH_R * c;
    return dd * 1000; // meters
}

