#pragma once

#include "non_volatile.hpp"
#include "renderutil.hpp"
#include "way.hpp"
#include "map.hpp"

#include <future>

class MapQuery : public RenderElement {
public:
    struct Filter {
        enum Mode {
            FILTER_EXACT = 0,
            FILTER_NOT,
            FILTER_SUBSTRING,
            FILTER_REGEX
        };
    
        void draw_static();
        void draw_editable();
        void draw_mode_dropdown();

        bool match(MapElement& elem) const;

        std::string m_attribute{}, m_filter{};
        Mode m_mode = FILTER_SUBSTRING;
    };

    static constexpr size_t QUERY_LIMIT = 128;

    MapQuery()
        : m_limit{QUERY_LIMIT}
    {}

    ~MapQuery() {
        if(auto& query = m_current_query)
            query->wait();
    }

    virtual void draw_scene(Viewport& viewport, InputState& input) override;
    virtual void draw_ui(InputState& input) override;
    virtual void menu_item() override;

    virtual int get_z_index() const override {
        return 50;
    }

private:
    void collect();
    void query(std::shared_ptr<Map> map);

    Filter m_new_filter{};
    std::vector<Filter> m_filters{};

    std::optional<std::future<std::vector<std::shared_ptr<MapElement>>>> m_current_query{};
    std::optional<std::vector<std::shared_ptr<MapElement>>> m_last_query{};

    non_volatile<size_t, "query.limit"> m_limit;
    bool m_show_query = false;
};

