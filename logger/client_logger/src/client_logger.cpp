#include </home/mp_os/common/include/not_implemented.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <filesystem>
#include "/home/mp_os/logger/client_logger/include/client_logger.h"
#include "/home/mp_os/logger/logger/include/logger.h"

std::unordered_map<std::string, std::pair<std::ofstream *, size_t>> client_logger::_all_streams;

client_logger::client_logger() 
{
    _format = "%s   %m %t %d";
}

client_logger::client_logger(std::string format) : _format(format)
{

}

client_logger::client_logger(
    client_logger const &other) : _streams(other._streams), _console_severity(other._console_severity)
{   
    
}

client_logger &client_logger::operator=(
    client_logger const &other)
{
    if (this != &other) {
        _streams = other._streams;
        _console_severity = other._console_severity;
    }
    
    return *this;
}

client_logger::client_logger(
    client_logger &&other) noexcept : _streams(std::move(other._streams)), _console_severity(std::move(other._console_severity))
{
    
}

client_logger &client_logger::operator=(
    client_logger &&other) noexcept
{
    if (this != &other) {
        _streams = std::move(other._streams);
        _console_severity = std::move(other._console_severity);
    }

    return *this;
}

client_logger::~client_logger() noexcept
{
    
    auto it = _streams.begin();
    while (it != _streams.end())
    {
        _all_streams[(*it).first].second -= 1;
        if (_all_streams[(*it).first].second == 0) {
            (*it).second.first->close();
        }

        ++it;
    }
}

void client_logger::add_file_stream_to_map_links_counter(
        std::string file_path_name) 
{   

    auto iterator = _all_streams.find(file_path_name);
    
    if (iterator == _all_streams.end()) 
    {
        std::ofstream * ostream = new std::ofstream;
        
        ostream->open(file_path_name);
        _all_streams[file_path_name] = std::make_pair(ostream, 1);
    }
    else 
    {
        _all_streams[file_path_name].second += 1;
    }

    return;
}

void client_logger::add_console_stream_to_log_map_streams(
        logger::severity const &severity) 
{
    _console_severity.emplace(severity);
}

void client_logger::add_file_stream_to_log_map_streams(
        std::string const &file_path_name, 
        logger::severity const &severity)
{   
    add_file_stream_to_map_links_counter(file_path_name);

    auto iterator = _streams.find(file_path_name);
 
    if (iterator == _streams.end()) 
    {
        std::ofstream * ostream = new std::ofstream; 
        ostream->open(file_path_name);
        _streams[file_path_name] = std::make_pair(ostream, std::set{severity});
    }
    else 
    {
        _streams[file_path_name].second.insert(severity);
    }

    return;
}



logger const *client_logger::log(
    const std::string &text,
    logger::severity severity) const noexcept
{
    auto it_map = _streams.begin();
    std::string format = client_logger::_format;
    while (it_map != _streams.end())
    {
        auto it_set = (*it_map).second.second.find(severity);

        // find current severity and write to file if it was found
        if (it_set != (*it_map).second.second.end()) {
            std::ofstream * out_stream = (*it_map).second.first;
            
            client_logger::print_with_flags_to_file_stream(out_stream, format, text, severity);
        }
        ++it_map;
    }
    
    auto finded = _console_severity.find(severity);
    if (finded != _console_severity.end()) {
        print_with_flags_to_console_stream(format, text, severity);
    }

    return this;
}

void client_logger::print_with_flags_to_file_stream(
    std::ofstream * stream, 
    std::string const &format, 
    std::string const &message, 
    logger::severity const &severity) const
{

    size_t size = format.length();
    std::string date = logger::current_date_to_string();
    std::string time = logger::current_time_to_string();
    std::string sev = logger::severity_to_string(severity);

    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '%') {
            if (i + 1 == format.size()) {
                *stream << '%';
                continue;
            }

            ++i;
            switch (format[i]) {
                case 'd': 
                    *stream << date;
                    break;
                case 't': 
                    *stream << time;
                    break;
                case 'm': 
                    *stream << message;
                    break;
                case 's':
                    *stream << sev;
                    break;
                default:
                    *stream << '%' << format[i];
                    break;
            }
        } else {
            *stream << format[i];
        }
    }

    *stream << std::endl;
}

void client_logger::print_with_flags_to_console_stream(
    std::string const &format, 
    std::string const &message, 
    logger::severity const &severity) const
{

    size_t size = format.length();
    std::string date = logger::current_date_to_string();
    std::string time = logger::current_time_to_string();
    std::string sev = logger::severity_to_string(severity);

    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '%') {
            if (i + 1 == format.size()) {
                std::cout << '%';
                continue;
            }

            ++i;
            switch (format[i]) {
                case 'd': 
                    std::cout << date;
                    break;
                case 't': 
                    std::cout << time;
                    break;
                case 'm': 
                    std::cout << message;
                    break;
                case 's':
                    std::cout << sev;
                    break;
                default:
                    std::cout << '%' << format[i];
                    break;
            }
        } else {
            std::cout << format[i];
        }
    }

    std::cout << std::endl;
}