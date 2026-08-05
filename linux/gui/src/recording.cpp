#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <iostream>
#include "recording.hpp"

void Recorder::record_sample(uint32_t timestamp_ms, uint32_t signal_id, std::string name,
                             double value) {
    if (!file_.is_open()) {
        return;
    }
    data_pending_ = true;
    file_ << timestamp_ms << "," << signal_id << "," << name << "," << value << "\n";
}

bool Recorder::start_recording(uint32_t plot_number) {
    std::time_t now = std::time(nullptr);
    std::tm local_time = *std::localtime(&now);

    std::ostringstream name;
    name << "recordings/record_plot" << plot_number << "_"
         << std::put_time(&local_time, "%Y%m%d_%H%M%S") << ".csv";
    filename_ = name.str();

    if (file_.is_open()) {
        file_.close();
    }
    file_.open(filename_, std::ios::trunc);
    if (!file_.is_open()) {
        return false;
    }

    file_ << "timestamp_ms,signal_id,name,value\n";
    return true;
}

void Recorder::stop_recording() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool Recorder::is_recording() const {
    return file_.is_open();
}

void Recorder::flush() {
    if (file_.is_open()) {
        file_.flush();
        data_pending_ = false;
    }
}

bool Recorder::has_data_pending() const {
    return data_pending_;
}
