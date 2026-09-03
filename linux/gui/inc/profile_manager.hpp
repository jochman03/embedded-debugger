#pragma once

#include <optional>
#include <unordered_set>

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

    void toggle_marked_profile(std::uint32_t profile_id);
    void clear_marked_profiles();
    bool is_profile_marked(std::uint32_t profile_id) const;
    const std::unordered_set<std::uint32_t>& marked_profiles() const;
    void remove_marked_profiles();
    void mark_all_profiles();
    void mark_profile_neighbor(bool up);

  private:
    std::optional<Profile> active_;

    std::vector<ProfileInfo> profile_list_;
    std::uint32_t active_profile_id_{0};
    std::uint32_t next_profile_id_{1};
    std::uint32_t last_clicked_id_{0};

    std::unordered_set<std::uint32_t> marked_profile_ids_;
};
