#include <algorithm>
#include <fstream>
#include <filesystem>

#include "session_manager.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Session& SessionManager::start_session() {
    const std::uint32_t id = next_session_id_++;

    session_list_.push_back({id});
    active_session_id_ = id;
    selected_session_id_ = id;

    loaded_[id] = Session(id);
    save_sessions_index();

    return loaded_.at(id);
}

void SessionManager::close_session() {
    if (active_session_id_ == 0) {
        return;
    }
    Session* session = get_session(active_session_id_);
    if (session != nullptr) {
        save_session(*session);
    }

    if (active_session_id_ != selected_session_id_) {
        unload_session(active_session_id_);
    }
    active_session_id_ = 0;
    save_sessions_index();
}

Session* SessionManager::active_session() {
    if (active_session_id_ == 0) {
        return nullptr;
    }
    return get_session(active_session_id_);
}

Session* SessionManager::selected_session() {
    if (selected_session_id_ == 0) {
        return nullptr;
    }
    return get_session(selected_session_id_);
}

const Session* SessionManager::selected_session() const {
    if (selected_session_id_ == 0) {
        return nullptr;
    }
    auto it = loaded_.find(selected_session_id_);
    if (it == loaded_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::uint32_t SessionManager::selected_session_id() {
    return selected_session_id_;
}

std::uint32_t SessionManager::active_session_id() {
    return active_session_id_;
}

void SessionManager::select_session(std::uint32_t session_id) {
    bool found = false;
    for (const SessionInfo& info : session_list_) {
        if (info.id == session_id) {
            found = true;
            break;
        }
    }
    if (!found) {
        return;
    }

    const std::uint32_t old_selected = selected_session_id_;
    selected_session_id_ = session_id;

    get_session(session_id);

    if (old_selected != 0 && old_selected != active_session_id_) {
        unload_session(old_selected);
    }
    save_sessions_index();
}

bool SessionManager::remove_session(std::uint32_t session_id) {
    if (session_id == 0 || session_id == active_session_id_) {
        return false;
    }

    bool found = false;
    for (const SessionInfo& info : session_list_) {
        if (info.id == session_id) {
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }

    unload_session(session_id);
    std::filesystem::remove(session_path(session_id));

    session_list_.erase(
        std::remove_if(session_list_.begin(), session_list_.end(),
                       [session_id](const SessionInfo& info) { return info.id == session_id; }),
        session_list_.end());
    if (selected_session_id_ == session_id) {
        if (active_session_id_ != 0) {
            selected_session_id_ = active_session_id_;
        } else if (!session_list_.empty()) {
            selected_session_id_ = session_list_.back().id;
        } else {
            selected_session_id_ = 0;
        }
        if (selected_session_id_ != 0) {
            get_session(selected_session_id_);
        }
    }
    save_sessions_index();
    return true;
}

void SessionManager::set_sessions_dir(const std::string& dir) {
    sessions_dir_ = dir;
    if (!sessions_dir_.empty() && sessions_dir_.back() != '/') {
        sessions_dir_ += '/';
    }
}

const std::vector<SessionInfo>& SessionManager::session_list() const {
    return session_list_;
}

std::string SessionManager::session_path(std::uint32_t id) const {
    return sessions_dir_ + "session_" + std::to_string(id) + ".json";
}

void SessionManager::save_session(const Session& session) {
    if (sessions_dir_.empty()) {
        return;
    }
    std::filesystem::create_directories(sessions_dir_);
    json j = session;
    std::ofstream out(session_path(session.id()));
    if (!out) {
        return;
    }
    out << j.dump(2);
}

bool SessionManager::load_session(std::uint32_t id, Session& session) {
    std::ifstream in(session_path(id));
    if (!in) {
        return false;
    }
    json j;
    in >> j;
    session = j.get<Session>();
    return true;
}

Session* SessionManager::get_session(std::uint32_t id) {
    auto it = loaded_.find(id);
    if (it != loaded_.end()) {
        return &it->second;
    }
    for (const SessionInfo& info : session_list_) {
        if (info.id != id) {
            continue;
        }
        Session session(id);
        load_session(id, session);
        auto [inserted, ok] = loaded_.emplace(id, std::move(session));
        return &inserted->second;
    }
    return nullptr;
}

void SessionManager::unload_session(std::uint32_t id) {
    auto it = loaded_.find(id);
    if (it == loaded_.end()) {
        return;
    }
    save_session(it->second);
    loaded_.erase(it);
}

void SessionManager::load_sessions_index() {
    if (sessions_dir_.empty()) {
        return;
    }
    session_list_.clear();
    loaded_.clear();
    active_session_id_ = 0;
    selected_session_id_ = 0;
    next_session_id_ = 1;

    std::ifstream in(sessions_dir_ + "sessions_index.json");
    if (!in) {
        return;
    }

    json j;
    in >> j;

    active_session_id_ = j.at("active_session_id").get<std::uint32_t>();
    selected_session_id_ = j.at("selected_session_id").get<std::uint32_t>();
    next_session_id_ = j.at("next_session_id").get<std::uint32_t>();

    for (const json& id : j.at("sessions")) {
        session_list_.push_back({id.get<std::uint32_t>()});
    }

    if (selected_session_id_ != 0) {
        get_session(selected_session_id_);
    }

    if (active_session_id_ != 0 && active_session_id_ != selected_session_id_) {
        get_session(active_session_id_);
    }
}

void SessionManager::save_loaded_sessions() {
    for (auto& [id, session] : loaded_) {
        save_session(session);
    }
    save_sessions_index();
}

void SessionManager::save_sessions_index() {
    if (sessions_dir_.empty()) {
        return;
    }
    std::filesystem::create_directories(sessions_dir_);

    json ids = json::array();
    for (const SessionInfo& info : session_list_) {
        ids.push_back(info.id);
    }

    json j{{"active_session_id", active_session_id_},
           {"selected_session_id", selected_session_id_},
           {"next_session_id", next_session_id_},
           {"sessions", ids}};
    std::ofstream out(sessions_dir_ + "sessions_index.json");
    if (!out) {
        return;
    }
    out << j.dump(2);
}