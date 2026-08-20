#pragma once

#include <vector>
#include <nlohmann/json_fwd.hpp>

#include "session.hpp"

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

    const std::vector<Session>& sessions() const;

    friend void to_json(nlohmann::json& j, const SessionManager& session_manager);
    friend void from_json(const nlohmann::json& j, SessionManager& session_manager);

  private:
    std::vector<Session> sessions_;
    std::uint32_t active_session_id_{0};
    std::uint32_t selected_session_id_{0};
    std::uint32_t next_session_id_{1};
};