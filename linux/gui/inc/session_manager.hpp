#pragma once

#include <vector>

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

  private:
    std::vector<Session> sessions_;
    std::uint32_t active_session_id_{0};
    std::uint32_t selected_session_id_{0};
    std::uint32_t next_session_id_{1};
};