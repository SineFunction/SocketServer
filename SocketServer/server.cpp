#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <algorithm>

#define PORT 8080

std::mutex cout_mutex;  // 保护 std::cout 的互斥锁
std::vector<std::thread> threads; // 存储线程的 vector
// 处理客户端的函数
void handle_client(int client_fd) {
    {
        std::lock_guard<std::mutex> lock(cout_mutex);//输出锁
        //输出当前线程的id
        //上锁的目的是保证输出的完整性
        std::cout << "客户端连接成功,线程ID: " << std::this_thread::get_id() << std::endl;
    }

    //告知客户端 连接已建立
    const char* msg = "欢迎连接服务器\n";
    send(client_fd, msg, strlen(msg), 0);

    char buffer[1024] = {0};
    while(true)
    {
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "收到客户端消息: " << buffer << std::endl;
                std::string stringBuffer(buffer);
                if(stringBuffer =="exit"){
                    std::cout<<"客户端已断开连接"<<std::endl;
                    break;
                }
            }

            std::string input_str(buffer);
            std::reverse(input_str.begin(), input_str.end());

            // 发送回客户端
            send(client_fd, input_str.c_str(), input_str.size(), 0);
        }
    }

    close(client_fd);
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "客户端断开连接,线程ID: " << std::this_thread::get_id() << " 结束\n";
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket 创建失败\n";
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    //绑定到对应的端口
    //如果不进行bind 服务器不知道监听哪个ip和端口
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "绑定失败\n";
        return -1;
    }

    //从主动模式改为被动模式
    //等待别人主动连接
    if (listen(server_fd, 5) < 0) {
        std::cerr << "监听失败\n";
        return -1;
    }

    std::cout << "服务器启动，等待客户端连接...\n";

    while (true) {
        sockaddr_in client_addr;//用于连接客户端
        socklen_t client_len = sizeof(client_addr);
        //等待建立连接
        //accept(服务器,客户端的地址信息,客户端的大小)
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr,&client_len);

        if (client_fd < 0) {
            std::cerr << "接收连接失败\n";
            continue;
        }

        // 创建线程处理客户端
        //创建一个线程来处理(函数,函数所需参数... ...)
        //在accept之后建立，确保这个线程负责的客户端存在
        threads.emplace_back(handle_client, client_fd);
    }

    close(server_fd);//关闭与客户端的连接

    // 等待所有线程执行完毕
    //std::thread对象是不可拷贝的 只能被移动 如果不使用引用
    //编译器会尝试拷贝线程对象 这会导致编译错误
    for (auto& t : threads) {
        //调用join()前检查joinable(),可以避免在一个不可join的线程上调用join(),
        //从而防止程序出现异常或未定义行为
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}