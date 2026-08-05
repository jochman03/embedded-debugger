#pragma once

#include <fstream>
#include <cstdint>

class Recorder {
  public:
    void record_sample(uint32_t timestamp_ms, uint32_t signal_id, std::string name, double value);
    bool start_recording(uint32_t plot_number);
    void stop_recording();
    bool is_recording() const;
    void flush();
    bool has_data_pending() const;

  private:
    std::string filename_;
    std::ofstream file_;
    bool data_pending_{false};
};
