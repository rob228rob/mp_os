#include </home/mp_os/common/include/not_implemented.h>
#include <zmq.hpp>
#include "/home/mp_os/logger/server_logger/include/server_logger.h"

server_logger::server_logger(std::string format) : _format(format), _context(1), _socket(_context, ZMQ_PUSH)
{
    _socket.connect("tcp://localhost:5555");
}

server_logger::server_logger(std::string format, std::string &port) : _format(format), _context(1), _socket(_context, ZMQ_PUSH)
{
    _socket.connect("tcp://localhost:" + port);
}

// NEED: =delete copy and op assignment
// server_logger::server_logger(
//     server_logger const &other) : _streams(other._streams),
//      _console_severity(other._console_severity), _format(other._format) 
// {

// }

// server_logger &server_logger::operator=(
//     server_logger const &other)
// {
//     if (this != &other) 
//     {
//         _streams = other._streams;
//         _console_severity = other._console_severity;
//         _format = other._format;
//     }

//     return *this;
// }

server_logger::server_logger(
    server_logger &&other) noexcept : _streams(std::move(other._streams)), 
    _console_severity(std::move(other._console_severity)), 
    _format(std::move(other._format)),
    _context(std::move(other._context)),
    _socket(std::move(other._socket))
{

}

server_logger &server_logger::operator=(
    server_logger &&other) noexcept
{
    if (this != &other) 
    {
        _streams = std::move(other._streams);
        _console_severity = std::move(other._console_severity);
        _format = std::move(other._format);
        _context = std::move(other._context);
        _socket = std::move(other._socket);
    }

    return *this;
}

server_logger::~server_logger() noexcept
{
    _socket.close();
}

void server_logger::add_file_stream_to_map_streams(
        std::string const &file_path_name, 
        logger::severity const &severity) 
{
    _streams[file_path_name].emplace(severity);
}

void server_logger::add_console_severity(
        logger::severity const &severity)
{   
    _console_severity.emplace(severity);
}

void server_logger::set_port(std::string &port) 
{
    if (port.length() == 4) 
    {
        _socket.close();
        _socket.connect("tcp://localhost:" + port);
    }

}

void server_logger::zmq_send_message(const std::string &text, logger::severity severity) const
{
    std::lock_guard<std::mutex> lock(_mtx); 
    std::string str_severity = severity_to_string(severity);
    std::string msg_str = str_severity + " " + text;
  
    const size_t max_message_size = 4096;
    size_t message_length = msg_str.size();
    auto start = std::chrono::high_resolution_clock::now(); 

    if (message_length <= max_message_size)
    {
        zmq::message_t message(msg_str.data(), message_length);
        _socket.send(message, zmq::send_flags::none);
    }
    else
    {
        size_t offset = 0;
        while (offset < message_length)
        {
            if (std::chrono::high_resolution_clock::now() - start > std::chrono::seconds(10))
            {
                throw std::runtime_error("Message sending took more than 10 seconds, operation aborted.");
            }

            size_t chunk_size = std::min(max_message_size, message_length - offset);
            std::string chunk = msg_str.substr(offset, chunk_size);
            zmq::message_t message(chunk.data(), chunk.size());
           
            auto send_flags = (offset + chunk_size < message_length) ? zmq::send_flags::sndmore 
                                                                      : zmq::send_flags::none;
            _socket.send(message, send_flags);
            
            offset += chunk_size;
        }
    }
}

logger const *server_logger::log(
    const std::string &text,
    logger::severity severity) const noexcept 
{
    auto it = _streams.begin();
    while (it != _streams.end())
    {
        auto it_set = (*it).second.find(severity);
        if (it_set != (*it).second.end())
        {
            zmq_send_message((*it).first + " " +  text, severity);
        }
        ++it;
    }

    auto console_it = _console_severity.find(severity);
    if (console_it != _console_severity.end()) 
    {
        zmq_send_message(std::string{"CONSOLE"} + " " + text, severity);
    }

    return this;
}