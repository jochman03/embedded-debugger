#include <ctime>
#include <iomanip>

#include "profile.hpp"

void Profile::set_name(const char* buf) {
    if (buf == nullptr) {
        return;
    }
    std::string new_name = buf;
    if (new_name.size() < 1 || new_name == name_) {
        return;
    }
    name_ = std::move(new_name);
}

void Profile::set_name(const std::string& new_name) {
    if (new_name.size() < 1 || new_name == name_) {
        return;
    }
    name_ = std::move(new_name);
}

const std::string& Profile::name() const {
    return name_;
}

std::time_t& Profile::updated_at() {
    return updated_at_;
}

const std::time_t& Profile::updated_at() const {
    return updated_at_;
}

void Profile::update_now() {
    updated_at_ = std::time(nullptr);
}

std::time_t& Profile::created_at() {
    return created_at_;
}

const std::time_t& Profile::created_at() const {
    return created_at_;
}

std::string& Profile::file_path() {
    return file_path_;
}

const std::string& Profile::file_path() const {
    return file_path_;
}

std::uint32_t Profile::id() const {
    return id_;
}

PlotManager* Profile::plot_manager() {
    return &plot_manager_;
}

SessionManager* Profile::session_manager() {
    return &session_manager_;
}

void Profile::register_signal(std::uint8_t signal_id, const std::string& name) {
    signal_registry_[signal_id] = name;
}

const std::map<std::uint8_t, std::string>& Profile::signal_registry() const {
    return signal_registry_;
}

void to_json(json& j, const Profile& profile) {
    j = json{{"name", profile.name_},
             {"file_path", profile.file_path_},
             {"created_at", profile.created_at_},
             {"updated_at", profile.updated_at_},
             {"plot_manager", profile.plot_manager_},
             {"signals", profile.signal_registry_}};
}

void from_json(const json& j, Profile& profile) {
    profile.name_ = j.at("name").get<std::string>();
    profile.file_path_ = j.at("file_path").get<std::string>();
    profile.created_at_ = j.at("created_at").get<time_t>();
    profile.updated_at_ = j.at("updated_at").get<time_t>();
    profile.plot_manager_ = j.at("plot_manager").get<PlotManager>();
    if (j.contains("signals")) {
        profile.signal_registry_ = j.at("signals").get<std::map<std::uint8_t, std::string>>();
    }
}
