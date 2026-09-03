#include <algorithm>
#include <fstream>
#include <filesystem>

#include "session_manager.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Session& SessionManager::start_session() {
    const bool create_new = auto_save_ || save_current_session_;
    if (!create_new) {
        std::uint32_t id = active_session_id_;
        if (id == 0) {
            id = selected_session_id_;
        }
        if (id == 0 && !session_list_.empty()) {
            id = session_list_.back().id;
        }
        if (id != 0) {
            active_session_id_ = id;
            selected_session_id_ = id;
            last_clicked_id_ = id;
            Session* session = get_session(id);
            session->clear();
            clear_marked_sessions();
            return *session;
        }
    }
    save_current_session_ = false;
    const std::uint32_t id = next_session_id_++;

    session_list_.push_back({id});
    active_session_id_ = id;
    selected_session_id_ = id;

    loaded_[id] = Session(id);
    last_clicked_id_ = id;
    if (create_new) {
        save_sessions_index();
    }

    clear_marked_sessions();

    return loaded_.at(id);
}

void SessionManager::close_session() {
    if (active_session_id_ == 0) {
        return;
    }
    Session* session = get_session(active_session_id_);
    if (session != nullptr && (auto_save_ || save_current_session_)) {
        save_session(*session, save_current_session_);
    }

    if (active_session_id_ != selected_session_id_) {
        unload_session(active_session_id_);
    }
    active_session_id_ = 0;
    if (auto_save_ || save_current_session_) {
        save_sessions_index();
    }
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
    last_clicked_id_ = session_id;
    get_session(session_id);

    if (old_selected != 0 && old_selected != active_session_id_) {
        unload_session(old_selected);
    }
    if (auto_save_) {
        save_sessions_index();
    }
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
    if (last_clicked_id_ == session_id) {
        last_clicked_id_ = selected_session_id_;
    }
    save_sessions_index();

    marked_session_ids_.erase(session_id);
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

void SessionManager::toggle_marked_session(std::uint32_t session_id) {
    if (session_id == 0 || session_id == active_session_id_) {
        return;
    }
    last_clicked_id_ = session_id;
    if (!marked_session_ids_.erase(session_id)) {
        marked_session_ids_.insert(session_id);
    }
}

void SessionManager::clear_marked_sessions() {
    marked_session_ids_.clear();
}

bool SessionManager::is_session_marked(std::uint32_t session_id) const {
    return marked_session_ids_.count(session_id) > 0;
}

const std::unordered_set<std::uint32_t>& SessionManager::marked_sessions() const {
    return marked_session_ids_;
}

void SessionManager::remove_marked_sessions() {
    const auto ids = marked_session_ids_;
    for (std::uint32_t id : ids) {
        remove_session(id);
    }
    clear_marked_sessions();
}

void SessionManager::mark_all_sessions() {
    for (const SessionInfo& info : session_list_) {
        if (info.id != active_session_id_ && info.id != 0) {
            marked_session_ids_.insert(info.id);
        }
    }
}

void SessionManager::mark_session_neighbor(bool up) {
    if (session_list_.empty()) {
        return;
    }

    const int delta = up ? -1 : 1;
    int i = -1;
    for (int k = 0; k < static_cast<int>(session_list_.size()); ++k) {
        if (session_list_[k].id == last_clicked_id_) {
            i = k;
            break;
        }
    }

    if (i < 0) {
        for (int k = 0; k < static_cast<int>(session_list_.size()); ++k) {
            if (session_list_[k].id == selected_session_id_) {
                i = k;
                break;
            }
        }
    }
    if (i < 0) {
        for (int k = 0; k < static_cast<int>(session_list_.size()); ++k) {
            if (session_list_[k].id == active_session_id_) {
                i = k;
                break;
            }
        }
    }
    if (i < 0) {
        i = 0;
    }

    for (int j = i + delta; j >= 0 && j < static_cast<int>(session_list_.size()); j += delta) {
        const std::uint32_t id = session_list_[j].id;
        last_clicked_id_ = id;
        if (id != active_session_id_) {
            marked_session_ids_.insert(id);
            return;
        }
    }
}

bool SessionManager::auto_save() const {
    return auto_save_;
}

void SessionManager::set_auto_save(bool enabled) {
    auto_save_ = enabled;
    if (auto_save_) {
        save_loaded_sessions();
    } else {
        save_sessions_index();
    }
}

void SessionManager::mark_session_for_save() {
    if (active_session_id_ == 0) {
        return;
    }
    save_current_session_ = true;
}

std::string SessionManager::session_path(std::uint32_t id) const {
    return sessions_dir_ + "session_" + std::to_string(id) + ".json";
}

void SessionManager::save_session(const Session& session, bool force) {
    if (sessions_dir_.empty() || (!auto_save_ && !force)) {
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
    clear_marked_sessions();
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
    auto_save_ = j.value("auto_save", false);

    for (const json& id : j.at("sessions")) {
        session_list_.push_back({id.get<std::uint32_t>()});
    }

    if (selected_session_id_ != 0) {
        get_session(selected_session_id_);
    }

    if (active_session_id_ != 0 && active_session_id_ != selected_session_id_) {
        get_session(active_session_id_);
    }
    last_clicked_id_ = selected_session_id_;
}

void SessionManager::save_loaded_sessions() {
    if (!auto_save_) {
        return;
    }
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
           {"sessions", ids},
           {"auto_save", auto_save_}};
    std::ofstream out(sessions_dir_ + "sessions_index.json");
    if (!out) {
        return;
    }
    out << j.dump(2);
}
