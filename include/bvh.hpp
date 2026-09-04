#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

#include "bbox.hpp"
#include "way.hpp"

class BVH : public BBox {
public:
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::shared_ptr<Way>;
        using difference_type = std::ptrdiff_t;
        using pointer = std::shared_ptr<Way>*;
        using reference = std::shared_ptr<Way>&;

        iterator() = default;

        explicit iterator(BVH* bvh) {
            if(bvh) {
                m_stack.push_back({bvh, 0, 0});
                advance();
            }
        }

        reference operator*() const {
            auto& frame = m_stack.back();
            return frame.bvh->m_ways[frame.priority][frame.index];
        }

        pointer operator->() const {
            auto& frame = m_stack.back();
            return &frame.bvh->m_ways[frame.priority][frame.index];
        }

        iterator& operator++() {
            if(m_stack.empty())
                return *this;

            auto& frame = m_stack.back();
            frame.index++;
            advance();
            return *this;
        }

        bool operator==(const iterator& other) const {
            return m_stack.empty() && other.m_stack.empty();
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        struct frame {
            BVH* bvh;
            size_t priority;
            size_t index;
        };

        void advance();

        std::vector<frame> m_stack{};
    };

    BVH(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth, size_t depth);

    void add_way(std::shared_ptr<Way> way);
    void rebuild_vaos();

    void draw(BBox& viewport, DrawPriority priority, size_t max_depth, size_t depth, float scale);

    std::pair<float, std::shared_ptr<Way>> get_nearest_way(glm::vec2 coords, DrawPriority priority) const;

    iterator begin() {
        return iterator(this);
    }

    iterator end() {
        return iterator();
    }

private:
    std::pair<std::unique_ptr<BVH>, std::unique_ptr<BVH>> m_children;
    std::vector<std::shared_ptr<Way>> m_ways[__DRAW_PRIO_LAST];
};

