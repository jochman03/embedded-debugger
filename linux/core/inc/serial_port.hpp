#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class SerialPort {
  public:
    ~SerialPort();

    bool open(const std::string& path, int baud_rate);
    void close();

    bool is_open() const;
    std::vector<std::uint8_t> read_available();

    const std::string& error() const;

    bool write_bytes(const std::uint8_t* data, std::size_t size);

  private:
    int descriptor_ = -1;
    std::string error_;
};