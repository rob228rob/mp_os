#include <zmq.hpp>
#include <thread>
#include <iostream>
#include <string>
#include <chrono>
#include <mutex>

void zmq_sender(zmq::context_t& context, int from, const std::string &port, std::mutex &mtx) {
    
    std::lock_guard<std::mutex> guard(mtx);
    zmq::socket_t socket(context, ZMQ_PUSH);
    socket.connect("tcp://localhost:" + port);

    for (int i = from; i < from+10; ++i) {
        std::string msg_str = "Hello " + std::to_string(i);
        zmq::message_t message(msg_str.data(), msg_str.size());
        socket.send(message, zmq::send_flags::none);
        std::cout << "Sent: " << msg_str << std::endl;
    }

    socket.close(); 

}

void zmq_receiver(zmq::context_t& context, const std::string& port, std::mutex &mtx) {
        std::lock_guard<std::mutex> guard(mtx);
    zmq::socket_t socket(context, ZMQ_PULL);
    socket.bind("tcp://" + port);

    auto start = std::chrono::steady_clock::now();

    while (true) {
        zmq::message_t message;
        if (socket.recv(message, zmq::recv_flags::dontwait)) {
            std::cout << "Received on " << port << ": " << message.to_string() << std::endl;
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= 10) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::cout << "Finished receiving messages on " << port << "." << std::endl;
    socket.close(); // Закрыть сокет после получения всех сообщений
}

int main() {
    std::mutex mtx;
    zmq::context_t context;

    // Создаем поток для отправки сообщений
    std::thread sender_thread(zmq_sender, std::ref(context), 10, "5555", std::ref(mtx));
    
    // Создаем поток для приема сообщений
    std::thread receiver_thread_one(zmq_receiver, std::ref(context), "*:5555", std::ref(mtx) );
    
    // Создаем еще один поток для приема сообщений
    std::thread receiver_thread_two(zmq_receiver, std::ref(context), "*:5556", std::ref(mtx));

    std::thread sender_second(zmq_sender, std::ref(context), 21, "5556", std::ref(mtx));

    sender_thread.join();
    sender_second.join();
    receiver_thread_one.join();
    receiver_thread_two.join();
    
    

    context.close(); // Очистить контекст после завершения работы всех сокетов

    return 0;
}
