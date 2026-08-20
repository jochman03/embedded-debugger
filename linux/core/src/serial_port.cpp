#include "serial_port.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace {
speed_t baud_value(int baud_rate) {
    switch (baud_rate) {
        case 9600:
            return B9600;

        case 115200:
            return B115200;

        default:
            return 0;
    }
}
} // namespace

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open(const std::string& path, int baud_rate) {
    close();
    error_.clear();

    const speed_t speed = baud_value(baud_rate);

    if (speed == 0) {
        error_ = "Unsupported baud rate";
        return false;
    }

    descriptor_ = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (descriptor_ < 0) {
        error_ = std::strerror(errno);
        return false;
    }

    termios settings{};

    if (tcgetattr(descriptor_, &settings) != 0) {
        error_ = std::strerror(errno);
        close();
        return false;
    }

    cfmakeraw(&settings);

    settings.c_cflag &= ~CSTOPB;
    settings.c_cflag &= ~CRTSCTS;
    settings.c_cflag |= CLOCAL | CREAD;

    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;

    cfsetispeed(&settings, speed);
    cfsetospeed(&settings, speed);

    if (tcsetattr(descriptor_, TCSANOW, &settings) != 0) {
        error_ = std::strerror(errno);
        close();
        return false;
    }

    tcflush(descriptor_, TCIFLUSH);

    return true;
}

void SerialPort::close() {
    if (descriptor_ >= 0) {
        ::close(descriptor_);
        descriptor_ = -1;
    }
}

bool SerialPort::is_open() const {
    return descriptor_ >= 0;
}

std::vector<std::uint8_t> SerialPort::read_available() {
    std::vector<std::uint8_t> result;

    if (!is_open()) {
        return result;
    }

    std::uint8_t buffer[512];

    while (true) {
        const ssize_t received = ::read(descriptor_, buffer, sizeof(buffer));

        if (received > 0) {
            result.insert(result.end(), buffer, buffer + received);

            continue;
        }

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            error_ = std::strerror(errno);
            close();
        }

        break;
    }

    return result;
}

const std::string& SerialPort::error() const {
    return error_;
}

bool SerialPort::write_bytes(const std::uint8_t* data, std::size_t size) {
    error_.clear();

    if (!is_open()) {
        error_ = "Serial port is not open";
        return false;
    }
    if (data == nullptr && size > 0) {
        error_ = "Invalid data buffer";
        return false;
    }

    std::size_t offset = 0;

    while (offset < size) {
        const ssize_t written = ::write(descriptor_, data + offset, size - offset);
        if (written > 0) {
            offset += static_cast<std::size_t>(written);
            continue;
        }

        if (written < 0 && errno == EINTR) {
            continue;
        }

        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            error_ = "Serial port write would block";
            return false;
        }

        if (written < 0) {
            error_ = std::strerror(errno);
            close();
        } else {
            error_ = "Serial port write returned zero";
        }

        return false;
    }
    return true;
}
