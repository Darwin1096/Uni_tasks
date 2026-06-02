#include "client_lib.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

Server::Server() : sockfd(-1), connected(false) {}

Server::~Server() {
    if (sockfd != -1) {
        close(sockfd);
    }
}

bool Server::connect(const std::string& ip, int port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return false;
    }

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Неверный адрес\n";
        close(sockfd);
        sockfd = -1;
        return false;
    }

    if (::connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Ошибка подключения\n";
        close(sockfd);
        sockfd = -1;
        return false;
    }

    connected = true;
    return true;
}

Result Server::sendCommand(const std::string& command, UserID uid) {
    if (!connected) {
        return Result(false, "Нет соединения с сервером");
    }

    // Формируем тело запроса: UserID (4 байта) + длина команды (4 байта) + сама команда
    std::vector<uint8_t> request;
    uint32_t uid_net = htonl(uid);
    request.insert(request.end(), reinterpret_cast<const uint8_t*>(&uid_net),
                   reinterpret_cast<const uint8_t*>(&uid_net) + 4);

    uint32_t cmd_len = htonl(static_cast<uint32_t>(command.size()));
    request.insert(request.end(), reinterpret_cast<const uint8_t*>(&cmd_len),
                   reinterpret_cast<const uint8_t*>(&cmd_len) + 4);
    request.insert(request.end(), command.begin(), command.end());

    // Отправляем длину + тело
    uint32_t total_len = htonl(static_cast<uint32_t>(request.size()));
    if (write(sockfd, &total_len, sizeof(total_len)) != sizeof(total_len)) {
        return Result(false, "Ошибка отправки длины запроса");
    }
    if (write(sockfd, request.data(), request.size()) != static_cast<ssize_t>(request.size())) {
        return Result(false, "Ошибка отправки запроса");
    }

    // Читаем длину ответа
    uint32_t resp_len_net;
    if (read(sockfd, &resp_len_net, sizeof(resp_len_net)) != sizeof(resp_len_net)) {
        return Result(false, "Ошибка чтения длины ответа");
    }
    uint32_t resp_len = ntohl(resp_len_net);
    if (resp_len > 10 * 1024 * 1024) { // 10 MB limit
        return Result(false, "Слишком большой ответ");
    }
    std::vector<uint8_t> resp_body(resp_len);
    size_t total_read = 0;
    while (total_read < resp_len) {
        ssize_t r = read(sockfd, resp_body.data() + total_read, resp_len - total_read);
        if (r <= 0) {
            return Result(false, "Ошибка чтения тела ответа");
        }
        total_read += r;
    }

    try {
        return Result::deserialize(resp_body);
    } catch (const std::exception& e) {
        return Result(false, std::string("Ошибка десериализации: ") + e.what());
    }
}