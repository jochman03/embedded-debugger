#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <iostream>

#include "profile_manager.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void ProfileManager::load_profiles() {}

void ProfileManager::create_profile() {
    std::time_t now = std::time(nullptr);
    std::tm local_time = *std::localtime(&now);

    profiles_.emplace_back(next_profile_id_);
    auto& profile = profiles_.back();
    std::ostringstream name;
    name << "profile_" << std::put_time(&local_time, "%Y%m%d_%H%M%S");

    profile.set_name(name.str());
    name << ".json";
    profile.file_path() = "profiles/" + name.str();

    profile.created_at() = now;
    profile.updated_at() = now;

    next_profile_id_++;
}

bool ProfileManager::remove_profile(std::uint32_t profile_id) {
    if (profile_id == 0 || profile_id == active_profile_id_) {
        return false;
    }
    const auto it =
        std::find_if(profiles_.begin(), profiles_.end(),
                     [profile_id](const Profile& profile) { return profile.id() == profile_id; });
    if (it != profiles_.end()) {
        profiles_.erase(it);
        return true;
    }
    return false;
}

bool ProfileManager::activate_profile(std::uint32_t profile_id) {
    auto it =
        std::find_if(profiles_.begin(), profiles_.end(),
                     [profile_id](const Profile& profile) { return profile.id() == profile_id; });
    if (it == profiles_.end()) {
        return false;
    }
    active_profile_id_ = profile_id;
    return true;
}

void ProfileManager::save_profile(uint32_t profile_id) {
    auto it = find_if(profiles_.begin(), profiles_.end(),
                      [profile_id](const Profile& profile) { return profile.id() == profile_id; });
    if (it == profiles_.end()) {
        return;
    }

    auto& profile = *it;

    json j = profile;
    std::filesystem::create_directories("profiles"); // albo parent_path z file_path
    std::ofstream out(profile.file_path());
    if (!out) {
        return;
    }
    out << j.dump(2);
};

uint32_t ProfileManager::active_profile_id() const {
    return active_profile_id_;
}

Profile* ProfileManager::active_profile() {
    if (active_profile_id_ == 0) {
        return nullptr;
    }
    const auto it =
        std::find_if(profiles_.begin(), profiles_.end(),
                     [this](const Profile& profile) { return profile.id() == active_profile_id_; });
    if (it != profiles_.end()) {
        return &(*it);
    }
    return nullptr;
}

std::vector<Profile>& ProfileManager::profiles() {
    return profiles_;
}