#include "client_lib.h"
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <iomanip>

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}


static void printResult(const Result& res) {
    // Выводим сообщение
    if (!res.message.empty()) {
        std::cout << res.message << std::endl;
    }

    // Если нет метаданных для матрицы — показываем записи списком
    if (res.times.empty() || res.auditories.empty() || res.rows == 0 || res.cols == 0) {
        if (!res.records.empty()) {
            std::cout << "Записи:\n";
            for (const auto& rec : res.records) {
                std::cout << "  " << rec.subject << " " << rec.teacher
                          << " (группа " << rec.group << ")\n";
            }
        }
        return;
    }

    // ---------- Естественная сортировка аудиторий по номеру ----------
    std::vector<std::pair<std::string, int>> audPairs;
    for (int i = 0; i < res.cols; ++i) {
        audPairs.emplace_back(res.auditories[i], i);
    }

    auto extractNumber = [](const std::string& s) -> int {
        auto pos = s.rfind('_');
        if (pos != std::string::npos) {
            try { return std::stoi(s.substr(pos + 1)); }
            catch (...) { return 0; }
        }
        return 0;
    };

    std::sort(audPairs.begin(), audPairs.end(),
        [&extractNumber](const auto& a, const auto& b) {
            return extractNumber(a.first) < extractNumber(b.first);
        });

    std::vector<int> oldToNew(res.cols);
    for (int newIdx = 0; newIdx < res.cols; ++newIdx) {
        oldToNew[audPairs[newIdx].second] = newIdx;
    }

    // ---------- Компактные ширины ----------
    const int timeWidth = 10;   // ширина колонки времени
    const int cellWidth = 16;   // ширина ячейки с данными (уменьшена!)

    // ---------- Вывод шапки ----------
    std::cout << std::left << std::setw(timeWidth) << "Время";
    for (const auto& p : audPairs) {
        std::string shortAud = p.first;
        if (shortAud.size() > cellWidth) {
            shortAud = shortAud.substr(0, cellWidth);
        }
        std::cout << std::left << std::setw(cellWidth) << shortAud;
    }
    std::cout << "\n";
    // Разделительная линия
    std::cout << std::string(timeWidth + cellWidth * res.cols, '-') << "\n";

    // ---------- Вывод строк ----------
    for (int t = 0; t < res.rows; ++t) {
        // Метка времени
        std::string timeLabel = "??? 0";
        if (t < static_cast<int>(res.times.size())) {
            const std::string& ts = res.times[t];
            auto spacePos = ts.find(' ');
            if (spacePos != std::string::npos) {
                std::string day = ts.substr(0, 3);
                std::string num = ts.substr(spacePos + 1);
                timeLabel = day + " " + num;
            } else {
                timeLabel = ts.substr(0, 3) + " ?";
            }
        }
        std::cout << std::left << std::setw(timeWidth) << timeLabel;

        for (int newA = 0; newA < res.cols; ++newA) {
            int origA = audPairs[newA].second;
            std::string cellContent = "---";

            for (const auto& rec : res.records) {
                if (rec.timeIndex == t && rec.auditoryIndex == origA) {
                    std::stringstream ss;
                    ss << rec.subject << " " << rec.teacher << "(" << rec.group << ")";
                    cellContent = ss.str();
                    break;
                }
            }

            if (cellContent.size() > cellWidth) {
                cellContent = cellContent.substr(0, cellWidth - 1) + ">";
            }
            std::cout << std::left << std::setw(cellWidth) << cellContent;
        }
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string ip = "127.0.0.1";
    int port = 1234;
    if (argc >= 2) ip = argv[1];
    if (argc >= 3) port = std::stoi(argv[2]);

    Server server;
    if (!server.connect(ip, port)) {
        std::cerr << "Не удалось подключиться к серверу\n";
        return 1;
    }
    std::cout << "Подключено к " << ip << ":" << port << "\n";
    std::cout << "Введите команды (EXIT для выхода).\n";

    UserID uid = 1; // фиксированный ID для простоты
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        line = trim(line);
        if (line.empty()) continue;

        std::string cmd = toUpper(line);
        Result res = server.sendCommand(line, uid);
        printResult(res);

        if (cmd == "EXIT") {
            std::cout << "Завершение работы клиента.\n";
            break;
        }
    }
    return 0;
}