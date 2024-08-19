#include "renderutil.hpp"
#include "log.hpp"

#include <cmath>

Texture::Texture(GLuint width, GLuint height, GLenum format, GLenum data_type) 
    : m_width(width), m_height(height)
{
    glGenTextures(1, &m_id);

    bind();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, data_type, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_id);
}

Framebuffer::Framebuffer(GLuint width, GLuint height) 
    : m_texture(std::make_unique<Texture>(width, height)) {
    glGenFramebuffers(1, &m_id);

    bind();
    m_texture->bind();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture->id(), 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        mlog::logln(mlog::ERROR, "Framebuffer error");
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &m_id);
}

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


