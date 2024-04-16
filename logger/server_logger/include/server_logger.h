#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H

#include </home/mp_os/logger/logger/include/logger.h>
#include "/home/mp_os/logger/server_logger/include/server_logger_builder.h"
#include <mutex>
#include <zmq.hpp>

class server_logger final:
    public logger
{

    friend class server_logger_builder;

public:

    server_logger() = default;

    server_logger(
        std::string format, 
        std::string &port);

    server_logger(
        std::string format);

    server_logger(
        server_logger const &other) = delete;

    server_logger &operator=(
        server_logger const &other) = delete;

    server_logger(
        server_logger &&other) noexcept;

    server_logger &operator=(
        server_logger &&other) noexcept;

    ~server_logger() noexcept final;

private:
    zmq::context_t _context;

    mutable zmq::socket_t _socket;

    mutable std::mutex _mtx;

    std::string _format;

    std::map<std::string, std::set<logger::severity>> _streams;

    std::set<logger::severity> _console_severity;

private:

    void set_port(std::string &port);

private:

    void add_file_stream_to_map_streams(
        std::string const &file_path_name, 
        logger::severity const &severity);

    void add_console_severity(
        logger::severity const &severity);

    void zmq_send_message(
        const std::string &text, 
        logger::severity severity) const;

public:

    [[nodiscard]] logger const *log(
        const std::string &message,
        logger::severity severity) const noexcept override; //add const

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H