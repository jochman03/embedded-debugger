#pragma once
#include <ctime>
#include <nlohmann/json.hpp>

#include "session_manager.hpp"

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
    void update_now();
    std::time_t& created_at();
    std::string& file_path();
    std::uint32_t id() const;

    PlotManager* plot_manager();

    SessionManager* session_manager();

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
};