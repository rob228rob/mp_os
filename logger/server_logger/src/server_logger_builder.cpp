#include </home/mp_os/common/include/not_implemented.h>

#include "/home/mp_os/logger/server_logger/include/server_logger_builder.h"
#include "/home/mp_os/logger/server_logger/include/server_logger.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fstream>

server_logger_builder::server_logger_builder()
{

}

server_logger_builder::server_logger_builder(std::string format) : _format(format)
{

}

server_logger_builder::server_logger_builder(
    server_logger_builder const &other) : _streams(other._streams) ,
     _console_severities(other._console_severities), _format(other._format)
{

}

server_logger_builder &server_logger_builder::operator=(
    server_logger_builder const &other)
{
    if (this != &other) 
    {
        _streams = other._streams;
        _console_severities = other._console_severities;
        _format = other._format;
    }

    return *this;
}

server_logger_builder::server_logger_builder(
    server_logger_builder &&other) noexcept : _streams(std::move(other._streams)),
     _console_severities(std::move(other._console_severities)), _format(std::move(other._format))
{

}


server_logger_builder &server_logger_builder::operator=(
    server_logger_builder &&other) noexcept
{
    if (this != &other) 
    {
        _streams = std::move(other._streams);
        _console_severities = std::move(other._console_severities);
        _format = std::move(other._format);
    }

    return *this;
}

server_logger_builder::~server_logger_builder() noexcept
{

}

logger_builder *server_logger_builder::add_file_stream(
    std::string const &stream_file_path,
    logger::severity severity)
{
    auto absolute_path = std::filesystem::absolute(stream_file_path);
    _streams[absolute_path].emplace(severity);

    return this;
}

logger_builder *server_logger_builder::add_console_stream(
    logger::severity severity)
{  
    _console_severities.emplace(severity);

    return this;
}

logger_builder* server_logger_builder::transform_with_configuration(
    std::string const &configuration_file_path,
    std::string const &configuration_path)
{
    std::ifstream json_file_stream(configuration_file_path);
    nlohmann::json data = nlohmann::json::parse(json_file_stream);

    nlohmann::json element;
    try
    {
        element = data.at(configuration_path);
    }
    catch (nlohmann::json::out_of_range& ex)
    {
        throw(std::string("Configuration path" + configuration_path +  "hasn't been found"));
    }
    std::string file = element["file"].get<std::string>();
    std::vector<logger::severity> severities = element["severity"];
    for(size_t i = 0; i < severities.size(); ++i)
    {
        if (file.empty()) 
            add_console_stream(severities[i]);
        else 
            add_file_stream(file, severities[i]);
    }
    return this;
}

logger_builder *server_logger_builder::clear()
{
    _streams.clear();
    _console_severities.clear();
    
    return this;
}

logger_builder *server_logger_builder::set_port(std::string &port) 
{
    _port = port;

    return this;
}

logger *server_logger_builder::build() const
{    
    std::string to_share = _port;
    server_logger* srvr_logger = new server_logger(_format, to_share);
    auto map_iterator = _streams.begin();
    
    while (map_iterator != _streams.end())
    {
        auto set_iterator = (*map_iterator).second.begin();
        while (set_iterator != (*map_iterator).second.end())
        {
            (*srvr_logger).add_file_stream_to_map_streams((*map_iterator).first, (*set_iterator));
            ++set_iterator;
        }
        ++map_iterator;
    }
    
    auto console_set_it = _console_severities.begin();
    while (console_set_it != _console_severities.end())
    {
        (*srvr_logger).add_console_severity((*console_set_it));
        ++console_set_it;
    }
    
    return srvr_logger;
}