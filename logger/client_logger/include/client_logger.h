#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H

#include </home/mp_os/logger/logger/include/logger.h>
#include "client_logger_builder.h"

#include <map>
#include <unordered_map>
#include <set>

class client_logger final:
    public logger
{

private:

    std::string _format;

private:

    std::map<std::string, std::pair<std::ofstream *, std::set<logger::severity>>> _streams;

    std::set<logger::severity> _console_severity;

private:

    static std::unordered_map<std::string, std::pair<std::ofstream *, size_t>> _all_streams;

private:

    void print_with_flags_to_file_stream(
        std::ofstream * stream, 
        std::string const &format, 
        std::string const &message, 
        logger::severity const &severity) const;

    void print_with_flags_to_console_stream( 
        std::string const &format, 
        std::string const &message, 
        logger::severity const &severity) const;

public:

    void add_file_stream_to_log_map_streams(
        std::string const &file_path_name, 
        logger::severity const &severity);

    void add_console_stream_to_log_map_streams(
        logger::severity const &severity);

    // THINK ABOUT THIS FUNC
    void add_file_stream_to_map_links_counter(
        std::string file_path_name);

public:

    client_logger();

    client_logger(std::string format);

    client_logger(
        client_logger const &other);

    client_logger &operator=(
        client_logger const &other);

    client_logger(
        client_logger &&other) noexcept;

    client_logger &operator=(
        client_logger &&other) noexcept;

    ~client_logger() noexcept final;

public:

    [[nodiscard]] logger const *log(
        const std::string &message,
        logger::severity severity) const noexcept override;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H