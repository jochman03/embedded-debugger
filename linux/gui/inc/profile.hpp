#pragma once
#include <ctime>
#include <nlohmann/json.hpp>

#include "session_manager.hpp"
#include "plot_manager.hpp"

#include <map>
#include <cstdint>

using json = nlohmann::json;

class Profile {
  public:
    Profile(std::uint32_t id) {
        id_ = id;
    }
    void set_name(const char* buf);
    void set_name(const std::string& new_name);

    const std::string& name() const;

    std::time_t& updated_at();
    const std::time_t& updated_at() const;

    void update_now();
    std::time_t& created_at();
    const std::time_t& created_at() const;

    std::string& file_path();
    const std::string& file_path() const;

    std::uint32_t id() const;

    PlotManager* plot_manager();

    SessionManager* session_manager();

    void register_signal(std::uint8_t signal_id, const std::string& name);
    const std::map<std::uint8_t, std::string>& signal_registry() const;

    friend void to_json(json& j, const Profile& profile);
    friend void from_json(const json& j, Profile& profile);

  private:
    std::uint32_t id_;
    std::string name_;
    std::string file_path_;
    std::time_t created_at_;
    std::time_t updated_at_;

    PlotManager plot_manager_;
    SessionManager session_manager_;

    std::map<std::uint8_t, std::string> signal_registry_;
};