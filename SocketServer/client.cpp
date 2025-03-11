#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"  // 服务器 IP
#define PORT 8080              // 服务器端口号

int main() {
    // 1. 创建客户端 Socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Socket 创建失败\n";
        return -1;
    }

    // 2. 服务器地址设置
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // 3. 将 IP 地址转换为网络字节序
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "无效的地址\n";
        return -1;
    }

    // 4. 连接服务器
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "连接失败\n";
        return -1;
    }

    std::cout << "成功连接服务器\n";

    char buffer[1024] = {0}; // 用于接收服务器返回的消息
    std::string input;
    int bytes = recv(sock,buffer,sizeof(buffer)-1,0);
    buffer[bytes] = '\0';
    std::cout<<"服务器:"<<buffer<<std::endl;
    // 5. 不断发送和接收数据
    while (true) {
        std::cout << "你:";
        std::getline(std::cin, input);

        if (input == "exit") {
            send(sock, input.c_str(), input.size(), 0);
            std::cout << "断开连接\n";
            break;
        }

        // 发送数据到服务器
        send(sock, input.c_str(), input.size(), 0);

        // 接收服务器返回的消息
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cerr << "服务器断开连接\n";
            break;
        }

        buffer[bytes_received] = '\0';  // 确保字符串正确终止
        std::cout << "服务器: " << buffer << std::endl;
    }

    // 6. 关闭 Socket
    close(sock);
    return 0;
}
