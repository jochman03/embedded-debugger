#include <fstream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <iostream>

#include "profile_manager.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void ProfileManager::load_profiles() {
    active_.reset();
    profile_list_.clear();

    active_profile_id_ = 0;
    next_profile_id_ = 1;

    std::ifstream in("profiles/index.json");
    if (!in) {
        return;
    }

    json j;
    in >> j;

    active_profile_id_ = j.at("active_profile_id").get<std::uint32_t>();
    next_profile_id_ = j.at("next_profile_id").get<std::uint32_t>();

    for (const json& entry : j.at("profiles")) {
        ProfileInfo info;
        info.id = entry.at("id").get<std::uint32_t>();
        info.name = entry.at("name").get<std::string>();
        info.file_path = entry.at("file_path").get<std::string>();
        profile_list_.push_back(info);
    }
}

void ProfileManager::load_profile(Profile& profile) {
    std::ifstream in(profile.file_path());
    if (!in) {
        return;
    }
    json j;
    try {
        in >> j;
        from_json(j, profile);
    } catch (const json::exception& e) {
        std::cerr << "Failed to load profiels " << profile.file_path() << ": " << e.what()
                  << std::endl;
    }
}

void ProfileManager::create_profile() {
    std::time_t now = std::time(nullptr);
    std::tm local_time = *std::localtime(&now);

    std::ostringstream name;
    name << "profile_" << std::put_time(&local_time, "%Y%m%d_%H%M%S");

    const std::string name_str = name.str();
    const std::string path = "profiles/" + name_str + ".json";

    profile_list_.push_back({next_profile_id_, name_str, path});
    next_profile_id_++;

    save_profiles_index();
}

bool ProfileManager::remove_profile(std::uint32_t profile_id) {
    if (profile_id == 0 || profile_id == active_profile_id_) {
        return false;
    }
    const auto it =
        std::find_if(profile_list_.begin(), profile_list_.end(),
                     [profile_id](const ProfileInfo& info) { return info.id == profile_id; });
    if (it == profile_list_.end()) {
        return false;
    }
    profile_list_.erase(it);
    save_profiles_index();
    return true;
}

bool ProfileManager::activate_profile(std::uint32_t profile_id) {
    auto it = std::find_if(profile_list_.begin(), profile_list_.end(),
                           [profile_id](const ProfileInfo& info) { return info.id == profile_id; });
    if (it == profile_list_.end()) {
        return false;
    }
    if (active_profile_id_ != 0 && active_profile_id_ != profile_id) {
        save_profile(active_profile_id_);
    }
    active_.emplace(profile_id);
    active_->set_name(it->name);
    active_->file_path() = it->file_path;

    active_profile_id_ = profile_id;
    save_profiles_index();

    load_profile(*active_);
    active_->session_manager()->set_sessions_dir(sessions_dir());
    active_->session_manager()->load_sessions_index();

    return true;
}

void ProfileManager::save_profile(uint32_t profile_id) {
    if (!active_ || profile_id != active_profile_id_) {
        return;
    }

    Profile& profile = *active_;
    profile.session_manager()->set_sessions_dir(sessions_dir());
    profile.session_manager()->save_loaded_sessions();

    for (ProfileInfo& info : profile_list_) {
        if (info.id == profile_id) {
            info.name = profile.name();
            break;
        }
    }

    json j = profile;
    std::filesystem::create_directories("profiles");
    std::ofstream out(profile.file_path());
    if (!out) {
        return;
    }
    out << j.dump(2);

    save_profiles_index();
}

void ProfileManager::save_profiles_index() {
    std::filesystem::create_directories("profiles");
    json list = json::array();
    for (const ProfileInfo& info : profile_list_) {
        list.push_back({
            {"id", info.id},
            {"name", info.name},
            {"file_path", info.file_path},
        });
    }

    json j{{"active_profile_id", active_profile_id_},
           {"next_profile_id", next_profile_id_},
           {"profiles", list}};

    std::ofstream out{"profiles/index.json"};
    if (!out) {
        return;
    }
    out << j.dump(2);
}

uint32_t ProfileManager::active_profile_id() const {
    return active_profile_id_;
}

Profile* ProfileManager::active_profile() {
    if (!active_ || active_profile_id_ == 0) {
        return nullptr;
    }
    return &*active_;
}

std::string ProfileManager::sessions_dir() {
    Profile* profile = active_profile();
    if (profile == nullptr) {
        return {};
    }

    std::string dir = profile->file_path();
    if (dir.size() >= 5 && dir.substr(dir.size() - 5) == ".json") {
        dir.resize(dir.size() - 5);
    }
    return dir + "/";
}

const std::vector<ProfileInfo>& ProfileManager::profile_list() const {
    return profile_list_;
}
