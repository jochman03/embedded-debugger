#pragma once

enum class LogType { Info, Warning, Error, Reset, Unknown };

LogType prefix_to_log_type(const std::string& prefix);