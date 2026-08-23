#pragma once

#include <optional>

#include "profile.hpp"

struct ProfileInfo {
    std::uint32_t id;
    std::string name;
    std::string file_path;
};

class ProfileManager {
  public:
    void create_profile();
    bool remove_profile(std::uint32_t profile_id);
    bool activate_profile(std::uint32_t profile_id);
    void save_profile(uint32_t profile_id);
    void save_profiles_index();
    void load_profiles();
    void load_profile(Profile& profile);

    uint32_t active_profile_id() const;
    Profile* active_profile();
    std::string sessions_dir();

    const std::vector<ProfileInfo>& profile_list() const;

  private:
    std::optional<Profile> active_;

    std::vector<ProfileInfo> profile_list_;
    std::uint32_t active_profile_id_{0};
    std::uint32_t next_profile_id_{1};
};
