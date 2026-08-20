#pragma once

#include "profile.hpp"

class ProfileManager {
  public:
    void load_profiles();
    void create_profile();
    bool remove_profile(std::uint32_t profile_id);
    bool activate_profile(std::uint32_t profile_id);
    void save_profile(uint32_t profile_id);
    uint32_t active_profile_id() const;
    Profile* active_profile();
    std::vector<Profile>& profiles();

  private:
    std::vector<Profile> profiles_;
    std::uint32_t active_profile_id_{0};
    std::uint32_t next_profile_id_{1};
};
