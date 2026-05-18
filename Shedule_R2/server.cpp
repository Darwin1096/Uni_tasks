// server.cpp
#include "shedule.h"
#include "result.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include <cctype>
#include <sstream>

// Прототипы вспомогательных функций
static std::string trim(const std::string& s);
static std::string toUpper(std::string s);
static void sendResponse(int client_fd, const Result& res);
static Result processCommand(Database& db, UserID uid, const std::string& command);
static bool readFull(int fd, void* buf, size_t n);
static bool writeFull(int fd, const void* buf, size_t n);

// ------------------------------------------------------------
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

static bool readFull(int fd, void* buf, size_t n) {
    size_t total = 0;
    char* ptr = static_cast<char*>(buf);
    while (total < n) {
        ssize_t r = read(fd, ptr + total, n - total);
        if (r <= 0) return false;
        total += r;
    }
    return true;
}

static bool writeFull(int fd, const void* buf, size_t n) {
    size_t total = 0;
    const char* ptr = static_cast<const char*>(buf);
    while (total < n) {
        ssize_t w = write(fd, ptr + total, n - total);
        if (w <= 0) return false;
        total += w;
    }
    return true;
}

static void sendResponse(int client_fd, const Result& res) {
    auto body = res.serialize();
    uint32_t len = htonl(static_cast<uint32_t>(body.size()));
    writeFull(client_fd, &len, sizeof(len));
    writeFull(client_fd, body.data(), body.size());
}

// Преобразование индексов в записи (для сессии и т.д.)
static std::vector<Schedule_unit> indicesToRecords(const std::vector<Indexes>& positions, const Database& db) {
    std::vector<Schedule_unit> recs;
    for (const auto& idx : positions) {
        const Schedule_unit* u = db.getUnit(idx.timeIndex, idx.auditoryIndex);
        if (u) recs.push_back(*u);
    }
    return recs;
}

// Формирование вспомогательных полей times/auditories для Result
static void fillTimeAndAudInfo(Result& res, const Database& db) {
    const auto& times = db.getTimes();
    const auto& auds  = db.getAuditories();
    res.rows = static_cast<int>(times.size());
    res.cols = static_cast<int>(auds.size());
    for (const auto& t : times) res.times.push_back(t.toString());
    res.auditories = auds;
}

// Обработка команды ADD (вручную, т.к. parse не поддерживает)
static Result handleAdd(Database& db, const std::string& args) {
    std::istringstream iss(args);
    std::string day, aud, subj, teach;
    int lesson, grp;
    if (!(iss >> day >> lesson >> aud >> subj >> teach >> grp)) {
        return Result(false, "Формат: ADD день урок аудитория предмет преподаватель группа");
    }

    DateTime dt(day, lesson);
    const auto& times = db.getTimes();
    const auto& auds  = db.getAuditories();
    int ti = -1, ai = -1;
    for (size_t i = 0; i < times.size(); ++i)
        if (times[i] == dt) { ti = static_cast<int>(i); break; }
    for (size_t i = 0; i < auds.size(); ++i)
        if (auds[i] == aud) { ai = static_cast<int>(i); break; }

    if (ti < 0 || ai < 0) {
        return Result(false, "Неизвестное время или аудитория");
    }

    Schedule_unit rec(subj, teach, grp, ti, ai);
    if (db.insertRecord(rec)) {
        Result res(true, "Добавлено");
        fillTimeAndAudInfo(res, db);
        return res;
    } else {
        return Result(false, "Ошибка (коллизия)");
    }
}

// Обработка GENERATE (вручную)
static Result handleGenerate(Database& db, const std::string& args) {
    int a = 4, b = 10, c = 10, d = 8, e = 12;
    double f = 0.6;
    std::istringstream iss(args);
    if (!(iss >> a >> b >> c >> d >> e >> f)) {
        // если параметров нет, используем значения по умолчанию
    }
    generateTestData(a, b, c, d, e, f);
    if (db.loadFromFile("Data.txt")) {
        Result res(true, "Сгенерировано в Data.txt и загружено");
        fillTimeAndAudInfo(res, db);
        return res;
    } else {
        return Result(false, "Ошибка загрузки Data.txt после генерации");
    }
}

// Основная функция обработки команды
static Result processCommand(Database& db, UserID uid, const std::string& command) {
    std::string cmdLine = trim(command);
    if (cmdLine.empty()) return Result(false, "Пустая команда");

    std::istringstream iss(cmdLine);
    std::string cmdToken;
    iss >> cmdToken;
    std::string cmd = toUpper(cmdToken);

    // EXIT обрабатывается на уровне сервера, сюда не попадает
    if (cmd == "HELP") {
        return Result(true, 
            "КОМАНДЫ:\n"
            "  PRINT                  - полное расписание\n"
            "  PRINT SELECT           - текущая выборка\n"
            "  PRINT <поле> <знач>    - быстрый поиск\n"
            "  SELECT <условия>       - поиск в выборку\n"
            "  RESELECT <условия>     - уточнить выборку\n"
            "  ADD <день> <пара> <ауд> <предм> <препод> <группа>\n"
            "  REMOVE <условия>       - удалить записи\n"
            "  SAVE <файл> / LOAD <файл>\n"
            "  GENERATE [параметры]\n"
            "  STATUS / CLEAR / EXIT");
    }
    else if (cmd == "STATUS") {
        Result res(true, "Статистика");
        fillTimeAndAudInfo(res, db);
        return res;
    }
    else if (cmd == "CLEAR") {
        db.getSessionManager().clearSelection(uid);
        Result res(true, "Выборка очищена");
        fillTimeAndAudInfo(res, db);
        return res;
    }
    else if (cmd == "SAVE" || cmd == "LOAD") {
        std::string fname;
        iss >> fname;
        if (fname.empty()) return Result(false, "Укажите имя файла");
        bool ok = (cmd == "LOAD") ? db.loadFromFile(fname) : db.saveToFile(fname);
        Result res(ok, ok ? "Успешно" : "Ошибка файла");
        fillTimeAndAudInfo(res, db);
        return res;
    }
    else if (cmd == "GENERATE") {
        std::string args;
        std::getline(iss, args);
        return handleGenerate(db, trim(args));
    }
    else if (cmd == "ADD") {
        std::string args;
        std::getline(iss, args);
        return handleAdd(db, args);
    }
    else {
        // Для SELECT, RESELECT, REMOVE, PRINT – используем parse
        Command parsed = parse(cmdLine);
        if (!parsed.valid) {
            return Result(false, parsed.errorMsg);
        }

        Result res(true, "");
        // Выполняем команду без использования db.execute, чтобы контролировать вывод
        switch (parsed.cmd) {
            case SELECT: {
                auto results = db.select(parsed.conditions);
                std::vector<Indexes> positions;
                for (const auto& u : results)
                    positions.emplace_back(u.timeIndex, u.auditoryIndex);
                db.getSessionManager().setSelection(uid, positions);
                res.records = std::move(results);
                res.message = "Выборка сохранена (" + std::to_string(res.records.size()) + " записей)";
                break;
            }
            case RESELECT: {
                const auto& current = db.getSessionManager().getSelection(uid);
                std::vector<Indexes> filtered;
                for (const auto& idx : current.selected_positions) {
                    if (idx.timeIndex >= 0 && idx.timeIndex < static_cast<int>(db.getTimes().size()) &&
                        idx.auditoryIndex >= 0 && idx.auditoryIndex < static_cast<int>(db.getAuditories().size()) &&
                        db.getUnit(idx.timeIndex, idx.auditoryIndex) != nullptr) {
                        if (match(*db.getUnit(idx.timeIndex, idx.auditoryIndex), db, parsed.conditions))
                            filtered.push_back(idx);
                    }
                }
                db.getSessionManager().setSelection(uid, filtered);
                res.records = indicesToRecords(filtered, db);
                res.message = "Перевыборка (" + std::to_string(res.records.size()) + " записей)";
                break;
            }
            case REMOVE: {
                size_t removed = db.removeRecords(parsed.removeConditions);
                res.affectedCount = static_cast<int>(removed);
                res.message = "Удалено записей: " + std::to_string(removed);
                break;
            }
            case PRINT: {
                std::vector<Schedule_unit> recs;
                if (parsed.fullOutput) {
                    recs = db.select(SearchConditions{});
                } else if (parsed.conditions.empty()) {
                    // PRINT SELECT
                    const auto& sel = db.getSessionManager().getSelection(uid);
                    if (sel.isEmpty()) {
                        res.success = true;
                        res.message = "Выборка пуста";
                        fillTimeAndAudInfo(res, db);
                        return res;
                    }
                    recs = indicesToRecords(sel.selected_positions, db);
                } else {
                    recs = db.select(parsed.conditions);
                }
                if (recs.empty() && !parsed.fullOutput) {
                    res.message = "Ничего не найдено";
                } else {
                    // Формируем матрицу или список? Для простоты возвращаем записи, клиент отобразит
                    res.records = std::move(recs);
                    res.message = "Найдено записей: " + std::to_string(res.records.size());
                }
                break;
            }
            default:
                return Result(false, "Команда не поддерживается сервером");
        }
        fillTimeAndAudInfo(res, db);
        return res;
    }
}

int main() {
    Database db;
    if (!db.loadFromFile("Data.txt")) {
        std::cout << "Data.txt не найден, начинаем с пустой базы.\n";
    } else {
        std::cout << "База загружена из Data.txt\n";
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(1234);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Ошибка bind\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Ошибка listen\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Сервер запущен на порту 1234\n";

    bool stopServer = false;

    while (!stopServer) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Ошибка accept\n";
            continue;
        }

        std::cout << "Подключился клиент\n";

        // Цикл обработки команд от данного клиента
        while (true) {
            // 1. Читаем длину сообщения
            uint32_t msg_len_net;
            if (!readFull(client_fd, &msg_len_net, sizeof(msg_len_net))) {
                std::cout << "Клиент отключился или ошибка чтения длины\n";
                break;  // выходим из цикла команд
            }
            uint32_t msg_len = ntohl(msg_len_net);
            if (msg_len > 1024 * 1024) {
                std::cerr << "Слишком длинное сообщение\n";
                break;
            }

            // 2. Читаем тело запроса
            std::vector<uint8_t> body(msg_len);
            if (!readFull(client_fd, body.data(), msg_len)) {
                std::cerr << "Ошибка чтения тела запроса\n";
                break;
            }

            // 3. Разбираем UserID и команду
            const uint8_t* ptr = body.data();
            size_t offset = 0;
            uint32_t uid_net;
            std::memcpy(&uid_net, ptr + offset, sizeof(uid_net)); offset += 4;
            UserID uid = ntohl(uid_net);

            uint32_t cmd_len_net;
            std::memcpy(&cmd_len_net, ptr + offset, sizeof(cmd_len_net)); offset += 4;
            uint32_t cmd_len = ntohl(cmd_len_net);
            std::string command(reinterpret_cast<const char*>(ptr + offset), cmd_len);

            std::cout << "Получена команда от пользователя " << uid << ": " << command << std::endl;

            // 4. Обработка EXIT
            std::string upperCmd = toUpper(trim(command));
            if (upperCmd == "EXIT") {
                Result exitRes(true, "Сервер завершает работу");
                sendResponse(client_fd, exitRes);
                stopServer = true;
                break;  // выходим из цикла команд
            }

            // 5. Выполняем команду
            Result res = processCommand(db, uid, command);
            sendResponse(client_fd, res);
        }

        close(client_fd);
        std::cout << "Соединение с клиентом закрыто\n";
    }

    close(server_fd);
    std::cout << "Сервер остановлен.\n";
    return 0;
}