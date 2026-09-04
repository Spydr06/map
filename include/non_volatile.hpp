#pragma once

#include <algorithm>
#include <filesystem>

#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

class non_volatile_store {
public:
    template<std::size_t N>
    struct fixed_string {
        char value[N];

        constexpr fixed_string(const char (&str)[N]) {
            std::copy_n(str, N, value);
        }
    };

    template<typename T, fixed_string Name>
    void store_field(const T& value) {
        std::unique_lock lock(m_mutex);
        m_fields[std::string(Name.value)] = serialize(value);
    }

    template<typename T, fixed_string Name>
    std::optional<T> load_field() {
        std::shared_lock lock(m_mutex);
        if(m_fields.find(Name.value) == m_fields.end())
            return std::nullopt;
        return deserialize<T>(m_fields[std::string(Name.value)]);
    }
        
protected:
    template<typename T>
    static std::string serialize(const T& value);

    template<typename T>
    static std::optional<T> deserialize(const std::string& value);

    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::string> m_fields;
};

class ini_store : public non_volatile_store {
public:
    ini_store(const std::filesystem::path& path);
    ~ini_store();

private:
    std::filesystem::path m_path;
};

extern std::shared_ptr<non_volatile_store> settings_store;

template<typename T, non_volatile_store::fixed_string Name>
struct non_volatile {
public:
    non_volatile() 
        : non_volatile(settings_store) 
    {}

    non_volatile(std::shared_ptr<non_volatile_store> store)
        : m_store(store)
    {
        load();
    }

    template<typename... Args>
        requires std::constructible_from<T, Args...>
    explicit non_volatile(Args&&... args)
        : non_volatile(settings_store, std::forward<Args>(args)...)
    {}

    template<typename... Args>
        requires std::constructible_from<T, Args...>
    explicit non_volatile(std::shared_ptr<non_volatile_store> store, Args&&... args)
        : m_store(store)
    {
        if(!load())
            m_value = T(std::forward<Args>(args)...);
    }

    non_volatile(const non_volatile&) = delete;
    non_volatile(non_volatile&&) = delete;

    ~non_volatile() = default;

    non_volatile& operator=(const T& other) 
        requires std::assignable_from<T&, const T&>
    {
        m_value = other;
        save();
        return *this;
    }

    non_volatile& operator=(T&& other)
        requires std::assignable_from<T&, T>
    {
        m_value = std::move(other);
        save();
        return *this;
    }

    T operator*() const {
        return m_value;
    }

    operator const T&() const {
        return m_value;
    }

    T* operator->() {
        return &m_value;
    }

    const T* operator->() const {
        return &m_value;
    }

    non_volatile& operator=(const non_volatile&) = delete;
    non_volatile& operator=(non_volatile&&) = delete;

    bool operator==(const T& other) {
        return m_value == other;
    }

    bool operator==(const non_volatile& other) {
        return m_value == other.m_value;
    }

    bool load() {
        if(!m_store)
            return false;

        // std::cout << "load " << Name.value << std::endl;
        if(auto value = m_store->load_field<T, Name>()) {
            m_value = std::move(*value);
            return true;
        }
        return false;
    }

    void save() const {
        if(!m_store)
            return;

        // std::cout << "save " << Name.value << " = " << m_value << std::endl;
        m_store->store_field<T, Name>(m_value);
    }

private:
    T m_value{};
    std::shared_ptr<non_volatile_store> m_store = nullptr;
};

