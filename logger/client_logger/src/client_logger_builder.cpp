#include </home/mp_os/common/include/not_implemented.h>
#include <vector>
#include "../include/client_logger_builder.h"
#include </home/mp_os/logger/client_logger/include/client_logger_builder.h>
#include </home/mp_os/logger/client_logger/include/client_logger.h>
#include <nlohmann/json.hpp>
#include <fstream>

client_logger_builder::client_logger_builder()
{
    _format = "%s %m %t %d";
}

client_logger_builder::client_logger_builder(std::string const &format) : _format(format)
{

}

client_logger_builder::client_logger_builder(
    client_logger_builder const &other) : streams(other.streams), _format(other._format)
{

}

client_logger_builder &client_logger_builder::operator=(
    client_logger_builder const &other)
{
    if (this != &other) {
        streams = other.streams;
        _format = other._format;
    }

    return *this;
}

client_logger_builder::client_logger_builder(
    client_logger_builder &&other) noexcept : streams(std::move(other.streams)), _format(std::move(other._format))
{

}

client_logger_builder &client_logger_builder::operator=(
    client_logger_builder &&other) noexcept
{
    if (this != &other) {
        streams = std::move(other.streams);
        _format = std::move(other._format);
    }

    return *this;
}

client_logger_builder::~client_logger_builder() noexcept
{

}

logger_builder *client_logger_builder::add_file_stream(
    std::string const &stream_file_path,
    logger::severity severity)
{
    auto absolute_path = std::filesystem::absolute(stream_file_path);
    streams[absolute_path].emplace(severity);

    return this;
}

logger_builder *client_logger_builder::add_console_stream(
    logger::severity severity)
{
    std::string fake_file_path = "cout";
    streams[fake_file_path].emplace(severity);
     
    return this;
}

logger_builder* client_logger_builder::transform_with_configuration(
        std::string const &configuration_file_path,
        std::string const &configuration_path)
{
    std::ifstream jsonFileStream(configuration_file_path);
    nlohmann::json data = nlohmann::json::parse(jsonFileStream);

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


logger_builder *client_logger_builder::clear()
{
    streams.clear();

    return this;
}

logger *client_logger_builder::build() const
{
    client_logger* client_log = new client_logger(_format);
    
    auto map_iterator = streams.begin();
    while (map_iterator != streams.end())
    {
        if ((*map_iterator).first == "cout") {
            auto set_iterator = (*map_iterator).second.begin();
            while (set_iterator != (*map_iterator).second.end())
            {
                (*client_log).add_console_stream_to_log_map_streams(*set_iterator); 
                ++set_iterator;
            }

            ++map_iterator;
            continue;
        }
        auto set_iterator = (*map_iterator).second.begin();
        while (set_iterator != (*map_iterator).second.end())
        {
            (*client_log).add_file_stream_to_log_map_streams((*map_iterator).first, (*set_iterator));
            ++set_iterator;
        }
        ++map_iterator;
    }
    
    return client_log;
}