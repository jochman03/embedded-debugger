#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json_fwd.hpp>

#include "session.hpp"

struct SessionInfo {
    std::uint32_t id;
};

class SessionManager {
  public:
    Session& start_session();
    void close_session();

    Session* active_session();
    const Session* active_session() const;

    Session* selected_session();
    const Session* selected_session() const;

    std::uint32_t selected_session_id();
    std::uint32_t active_session_id();

    void select_session(std::uint32_t session_id);
    bool remove_session(std::uint32_t session_id);

    void set_sessions_dir(const std::string& dir);
    void load_sessions_index();
    void save_loaded_sessions();

    const std::vector<SessionInfo>& session_list() const;

  private:
    std::uint32_t active_session_id_{0};
    std::uint32_t selected_session_id_{0};
    std::uint32_t next_session_id_{1};

    std::vector<SessionInfo> session_list_;
    std::string sessions_dir_;
    std::unordered_map<std::uint32_t, Session> loaded_;

    std::string session_path(std::uint32_t id) const;

    void save_session(const Session& session);
    bool load_session(std::uint32_t id, Session& session);
    Session* get_session(std::uint32_t id);
    void unload_session(std::uint32_t id);
    void save_sessions_index();
};