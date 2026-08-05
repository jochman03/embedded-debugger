#include "session_manager.hpp"

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
    return &sessions_[active_session_id_ - 1];
}

const Session* SessionManager::active_session() const {
    if (active_session_id_ == 0) {
        return nullptr;
    }
    return &sessions_[active_session_id_ - 1];
}

Session* SessionManager::selected_session() {
    if (selected_session_id_ == 0) {
        return nullptr;
    }
    return &sessions_[selected_session_id_ - 1];
}

const Session* SessionManager::selected_session() const {
    if (selected_session_id_ == 0) {
        return nullptr;
    }
    return &sessions_[selected_session_id_ - 1];
}

std::uint32_t SessionManager::selected_session_id() {
    return selected_session_id_;
}

std::uint32_t SessionManager::active_session_id() {
    return active_session_id_;
}

void SessionManager::select_session(std::uint32_t session_id) {
    selected_session_id_ = session_id;
}

bool SessionManager::remove_session(std::uint32_t session_id) {
    if (session_id == 0) {
        return false;
    }
    if (session_id == active_session_id_) {
        return false;
    }
    std::size_t index_to_remove = sessions_.size();

    for (std::size_t i = 0; i < sessions_.size(); ++i) {
        if (sessions_[i].id() == session_id) {
            index_to_remove = i;
            break;
        }
    }
    if (index_to_remove == sessions_.size()) {
        return false;
    }
    sessions_.erase(sessions_.begin() + index_to_remove);

    if (selected_session_id_ == session_id) {
        if (active_session_id_ != 0) {
            session_id = active_session_id_;
        } else if (selected_session_id_ == 0 && sessions_.size() != 0) {
            selected_session_id_ = sessions_.size();
        } else {
            selected_session_id_ = 0;
        }
    }
    return true;
}

const std::vector<Session>& SessionManager::sessions() const {
    return sessions_;
}
