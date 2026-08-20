#include "session_manager.hpp"
#include <algorithm>

Session& SessionManager::start_session() {
    sessions_.emplace_back(next_session_id_);
    selected_session_id_ = next_session_id_;
    active_session_id_ = next_session_id_;

    next_session_id_++;
    return sessions_.back();
}

void SessionManager::close_session() {
    active_session_id_ = 0;
}

Session* SessionManager::active_session() {
    if (active_session_id_ == 0) {
        return nullptr;
    }
    auto it = std::find_if(sessions_.begin(), sessions_.end(), [this](const Session& session) {
        return session.id() == active_session_id_;
    });
    if (it == sessions_.end()) {
        return nullptr;
    }
    return &(*it);
}

const Session* SessionManager::active_session() const {
    if (active_session_id_ == 0) {
        return nullptr;
    }
    auto it = std::find_if(sessions_.begin(), sessions_.end(), [this](const Session& session) {
        return session.id() == active_session_id_;
    });
    if (it == sessions_.end()) {
        return nullptr;
    }
    return &(*it);
}

Session* SessionManager::selected_session() {
    if (selected_session_id_ == 0) {
        return nullptr;
    }
    auto it = std::find_if(sessions_.begin(), sessions_.end(), [this](const Session& session) {
        return session.id() == selected_session_id_;
    });
    if (it == sessions_.end()) {
        return nullptr;
    }
    return &(*it);
}

const Session* SessionManager::selected_session() const {
    if (selected_session_id_ == 0) {
        return nullptr;
    }
    auto it = std::find_if(sessions_.begin(), sessions_.end(), [this](const Session& session) {
        return session.id() == selected_session_id_;
    });
    if (it == sessions_.end()) {
        return nullptr;
    }
    return &(*it);
}

std::uint32_t SessionManager::selected_session_id() {
    return selected_session_id_;
}

std::uint32_t SessionManager::active_session_id() {
    return active_session_id_;
}

void SessionManager::select_session(std::uint32_t session_id) {
    auto it =
        std::find_if(sessions_.begin(), sessions_.end(),
                     [session_id](const Session& session) { return session.id() == session_id; });
    if (it == sessions_.end()) {
        return;
    }
    selected_session_id_ = session_id;
}

bool SessionManager::remove_session(std::uint32_t session_id) {
    if (session_id == 0 || session_id == active_session_id_) {
        return false;
    }

    auto it =
        std::find_if(sessions_.begin(), sessions_.end(),
                     [session_id](const Session& session) { return session.id() == session_id; });
    if (it == sessions_.end()) {
        return false;
    }
    sessions_.erase(it);

    if (selected_session_id_ == session_id) {
        if (active_session_id_ != 0) {
            selected_session_id_ = active_session_id_;
        } else if (!sessions_.empty()) {
            selected_session_id_ = sessions_.back().id();
        } else {
            selected_session_id_ = 0;
        }
    }
    return true;
}

const std::vector<Session>& SessionManager::sessions() const {
    return sessions_;
}

void to_json(nlohmann::json& j, const SessionManager& session_manager) {
    j = json{{"sessions", session_manager.sessions_},
             {"active_session_id", session_manager.active_session_id_},
             {"selected_session_id", session_manager.selected_session_id_},
             {"next_session_id", session_manager.next_session_id_}};
}

void from_json(const nlohmann::json& j, SessionManager& session_manager) {
    session_manager.sessions_ = j.at("sessions").get<std::vector<Session>>();
    session_manager.active_session_id_ = j.at("active_session_id").get<std::uint32_t>();
    session_manager.selected_session_id_ = j.at("selected_session_id").get<std::uint32_t>();
    session_manager.next_session_id_ = j.at("next_session_id").get<std::uint32_t>();
}
