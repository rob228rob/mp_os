#include <zmq.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <mutex>

#include "/home/mp_os/logger/server_logger/include/server_logger_builder.h"
#include "/home/mp_os/logger/server_logger/include/server_logger.h"
#include "/home/mp_os/logger/logger/include/logger.h"

void zmq_receiver(zmq::context_t& context, const std::string& port, std::mutex& mtx) {
    zmq::socket_t socket(context, ZMQ_PULL);
    socket.bind("tcp://*:" + port);

    std::cout << "Receiver is running on port: " << port << "\n";

    auto start = std::chrono::steady_clock::now();

    while (true) {
        try {
            zmq::message_t message;
            zmq::recv_result_t result = socket.recv(message, zmq::recv_flags::dontwait);

            if (result.has_value()) {

                std::string complete_message = message.to_string() + "\npart_0\n";

                int count = 0;
                while (socket.get(zmq::sockopt::rcvmore)) {
                    ++count;
                    zmq::recv_result_t res = socket.recv(message, zmq::recv_flags::dontwait);
                    complete_message += message.to_string() + "\npart_" + std::to_string(count) + "\n\n";
                }

                std::lock_guard<std::mutex> guard(mtx);
                std::cout << "Received: " << complete_message << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (std::chrono::steady_clock::now() - start >= std::chrono::seconds(10)) {
                break;
            }
        }
        catch (const zmq::error_t& e) {
            std::cerr << "ZMQ error: " << e.what() << std::endl;
            break;
        }
    }

    std::cout << "Receiver on port " << port << " has stopped.\n";
}

int main() {

    server_logger_builder* builder = new server_logger_builder("%s %m %d %t");
    builder->add_file_stream("1.txt", logger::severity::trace);
    builder->add_file_stream("1.txt", logger::severity::debug);
    builder->add_file_stream("1.txt", logger::severity::error);
    std::string port1 = "5554";
    builder->set_port(port1);
    logger* loggr = builder->build();
    std::string msg = "start";
    for (int i = 0; i < 4099; ++i) 
    {
        msg += "x";
        if (i % 100 == 0) msg += std::to_string(i);
    }
    loggr->error("sadfjksdnlcsmdlcmsdl;clmsd;cmsd; dskcsd;mcsd mcsldkmmcdsl cldsnc sdcn;asdas[dma asda;lsd;alsmd downdnokcnokdncd dpwnckwdoncowkdc wakcas");
    loggr->debug(msg);


    zmq::context_t context(1);
    std::mutex mtx;
    const std::string port = "5554";  
    

    std::thread receiver_thread(zmq_receiver, std::ref(context), std::ref(port), std::ref(mtx));

    receiver_thread.join();

    return 0;
}