#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include "server/server.hpp"
#include "msgpack.hpp"
#include <sstream>
#include <iostream>

#include "thread_pool/thread_pool.hpp"

void task(int a) {
    std::cout << std::this_thread::get_id() << " is working on: " << a << std::endl;
}


int main() {
    //Server server("127.0.0.1", 54000);
    //server.createTopic("topic1", 10);
    //server.createTopic("topic2", 2);
    //server.createTopic("topic3", 1);
    //std::thread(&Server::processNonEmptyTopicThreadCreator, std::ref(server)).detach();
    //server.serverLoop();

    ThreadPool pool(4);

    for (int i = 0; i < 10; ++i) {
        pool.addTask(task, i);
    }

    // Wait for some time to let tasks complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Properly terminate the thread pool
    pool.terminate();

    std::cout << "Thread pool terminated." << std::endl;

    return 0;
}