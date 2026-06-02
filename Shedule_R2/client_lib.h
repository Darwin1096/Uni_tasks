#ifndef CLIENT_LIB_H
#define CLIENT_LIB_H

#include "result.h"
#include <string>
#include <cstdint>

class Server {
public:
    Server();
    ~Server();

    // Подключение к серверу
    bool connect(const std::string& ip, int port);

    // Отправка команды и получение результата
    Result sendCommand(const std::string& command, UserID uid);

private:
    int sockfd;
    bool connected;
};

#endif