// main.cpp
#include "shedule.h"
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <cassert>

// ---------- Вспомогательные утилиты ----------
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

static std::string getCommand(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    return toUpper(trim(cmd));
}

static void printHelp() {
    std::cout << "\n=== СИСТЕМА УПРАВЛЕНИЯ РАСПИСАНИЕМ ===\n"
        "КОМАНДЫ:\n"
        "  PRINT                  - показать полное расписание (матрица)\n"
        "  PRINT SELECT           - показать текущую выборку\n"
        "  PRINT <поле> <знач>    - быстрый поиск (матрица)\n"
        "  SELECT <условия>       - поиск в выборку\n"
        "  RESELECT <условия>     - уточнить выборку\n"
        "  ADD <день> <пара> <ауд> <предм> <препод> <группа>\n"
        "  REMOVE <условия>       - удалить записи\n"
        "  LOAD <файл> / SAVE <файл>\n"
        "  GENERATE [параметры]\n"
        "  STATUS / CLEAR / HELP / EXIT\n"
        "-----------------------------------------------\n";
}

// ---------- Автоматические тесты ----------
bool runTests() {
    std::cout << "\n========== ЗАПУСК ТЕСТОВ ==========\n";
    bool allPassed = true;
    const std::string testFile = "test_schedule.txt";
    const std::string saveFile  = "test_save.txt";

    // 1. Подготовка тестового файла
    {
        std::ofstream out(testFile);
        out << "Monday 1 Room_1 Subject_1 Teacher_1 1\n";
        out << "Monday 1 Room_2 Subject_2 Teacher_2 2\n";
        out << "Monday 2 Room_1 Subject_1 Teacher_2 1\n";
        out << "Tuesday 1 Room_1 Subject_2 Teacher_1 2\n";
        out << "Tuesday 2 Room_2 Subject_1 Teacher_2 2\n";
        out.close();
    }

    Database db;
    if (!db.loadFromFile(testFile)) { std::cerr << "FAIL: loadFromFile\n"; allPassed = false; }
    else std::cout << "OK: loadFromFile\n";

    UserID uid = 1;

    // 2. PRINT полного расписания
    Command cmdPrintAll = parse("PRINT");
    if (!cmdPrintAll.valid || !cmdPrintAll.fullOutput) {
        std::cerr << "FAIL: parse PRINT full\n"; allPassed = false;
    } else {
        // db.execute(uid, cmdPrintAll); // можно закомментировать для чистоты вывода тестов
        auto all = db.select(SearchConditions{});
        if (all.size() != 5) { std::cerr << "FAIL: полное расписание ожидалось 5, получено " << all.size() << "\n"; allPassed = false; }
        else std::cout << "OK: полное расписание содержит 5 записей\n";
    }

    // 3. SELECT и PRINT SELECT
    Command cmdSelect = parse("SELECT GROUP == 2");
    db.execute(uid, cmdSelect);
    const auto& sel = db.getSessionManager().getSelection(uid);
    if (sel.size() != 3) { std::cerr << "FAIL: SELECT GROUP == 2 ожидалось 3, получено " << sel.size() << "\n"; allPassed = false; }
    else {
        std::cout << "OK: SELECT GROUP == 2 (3 записи)\n";
        Command cmdPrintSel = parse("PRINT SELECT");
        db.execute(uid, cmdPrintSel);
        if (db.getSessionManager().getSelection(uid).size() != 3) { std::cerr << "FAIL: PRINT SELECT изменил выборку\n"; allPassed = false; }
        else std::cout << "OK: PRINT SELECT сохраняет выборку\n";
    }

    // 4. RESELECT
    Command cmdResel = parse("RESELECT TEACHER == Teacher_1");
    db.execute(uid, cmdResel);
    if (db.getSessionManager().getSelection(uid).size() != 1) { std::cerr << "FAIL: RESELECT ожидалась 1 запись\n"; allPassed = false; }
    else std::cout << "OK: RESELECT (1 запись)\n";

    // 5. Быстрый поиск (не меняет выборку)
    size_t beforeQuick = db.getSessionManager().getSelection(uid).size();
    Command cmdQuick = parse("PRINT TEACHER Teacher_2");
    db.execute(uid, cmdQuick);
    if (db.getSessionManager().getSelection(uid).size() != beforeQuick) { std::cerr << "FAIL: быстрый поиск изменил выборку\n"; allPassed = false; }
    else std::cout << "OK: быстрый поиск не меняет выборку\n";
    if (db.select(cmdQuick.conditions).size() != 3) { std::cerr << "FAIL: быстрый поиск Teacher_2 ожидалось 3\n"; allPassed = false; }
    else std::cout << "OK: быстрый поиск Teacher_2 найдено 3\n";

    // 6. ADD без коллизий
    const auto& times = db.getTimes();
    const auto& auds = db.getAuditories();
    int tIdxMon2 = -1, aIdxRoom2 = -1;
    for (size_t i=0; i<times.size(); ++i)
        if (times[i].weekday=="Monday" && times[i].lesson_num==2) tIdxMon2 = static_cast<int>(i);
    for (size_t i=0; i<auds.size(); ++i)
        if (auds[i]=="Room_2") aIdxRoom2 = static_cast<int>(i);
    Schedule_unit newRec("Subject_3","Teacher_3",3, tIdxMon2, aIdxRoom2);
    if (!db.insertRecord(newRec)) { std::cerr << "FAIL: ADD без коллизий\n"; allPassed = false; }
    else {
        std::cout << "OK: ADD без коллизий\n";
        if (db.select(SearchConditions{}).size() != 6) { std::cerr << "FAIL: после ADD ожидалось 6 записей\n"; allPassed = false; }
        else std::cout << "OK: после ADD 6 записей\n";
    }

    // 7. ADD с коллизией
    Schedule_unit conflictRec("Subject_4","Teacher_4",4, tIdxMon2, aIdxRoom2);
    if (db.insertRecord(conflictRec)) { std::cerr << "FAIL: ADD с коллизией должно было отклониться\n"; allPassed = false; }
    else std::cout << "OK: ADD с коллизией отклонено\n";

    // 8. REMOVE группы 3 и проверка количества
    size_t removed = db.removeRecords(parse("REMOVE GROUP == 3").removeConditions);
    if (removed != 1) { std::cerr << "FAIL: REMOVE GROUP == 3 ожидалось 1, удалено " << removed << "\n"; allPassed = false; }
    else std::cout << "OK: REMOVE (удалена 1 запись)\n";
    if (db.select(SearchConditions{}).size() != 5) { std::cerr << "FAIL: после REMOVE ожидалось 5 записей\n"; allPassed = false; }
    else std::cout << "OK: после REMOVE 5 записей\n";

    // 9. SAVE / LOAD roundtrip
    if (!db.saveToFile(saveFile)) { std::cerr << "FAIL: saveToFile\n"; allPassed = false; }
    else {
        Database db2;
        if (!db2.loadFromFile(saveFile)) { std::cerr << "FAIL: load после save\n"; allPassed = false; }
        else {
            if (db2.select(SearchConditions{}).size() != 5) { std::cerr << "FAIL: roundtrip ожидалось 5\n"; allPassed = false; }
            else std::cout << "OK: save/load roundtrip (5 записей)\n";
        }
    }

    // 10. CLEAR и PRINT SELECT с пустой выборкой
    db.getSessionManager().clearSelection(uid);
    Command cmdPrintSelEmpty = parse("PRINT SELECT");
    db.execute(uid, cmdPrintSelEmpty);   // ожидается сообщение "Выборка пуста..."
    if (db.getSessionManager().getSelection(uid).size() != 0) { std::cerr << "FAIL: CLEAR не очистил выборку\n"; allPassed = false; }
    else std::cout << "OK: CLEAR и PRINT SELECT с пустой выборкой\n";

    // 11. Парсер ошибок
    Command err1 = parse("UNKNOWN_CMD");
    if (err1.valid) { std::cerr << "FAIL: парсер пропустил неизвестную команду\n"; allPassed = false; }
    else std::cout << "OK: неизвестная команда -> ошибка\n";

    Command err2 = parse("SELECT BADFIELD == 1");
    if (err2.valid) { std::cerr << "FAIL: парсер пропустил неверное поле\n"; allPassed = false; }
    else std::cout << "OK: неверное поле в SELECT -> ошибка\n";

    Command err3 = parse("PRINT TEACHER");
    if (err3.valid) { std::cerr << "FAIL: парсер пропустил отсутствие значения\n"; allPassed = false; }
    else std::cout << "OK: PRINT без значения -> ошибка\n";

    // 12. getFreeAuditories
    DateTime dtMon2("Monday", 2);
    auto freeAuds = db.getFreeAuditories(dtMon2);
    if (freeAuds.size() != 1 || freeAuds[0] != "Room_2") {
        std::cerr << "FAIL: getFreeAuditories ожидалась только Room_2\n"; allPassed = false;
    } else std::cout << "OK: getFreeAuditories\n";

    // Очистка
    std::remove(testFile.c_str());
    std::remove(saveFile.c_str());

    if (allPassed) std::cout << "========== ВСЕ ТЕСТЫ ПРОЙДЕНЫ ==========\n";
    else           std::cout << "========== НЕКОТОРЫЕ ТЕСТЫ ПРОВАЛЕНЫ ==========\n";
    return allPassed;
}

// ---------- main ----------
int main() {
    printHelp();
    runTests();   // закомментируйте, если не нужны тесты

    Database db;
    UserID uid = 1;

    if (!db.loadFromFile("Data.txt")) {
        std::cout << "Генерация начальных тестовых данных...\n";
        generateTestData(4, 10, 10, 8, 12, 0.6);
        db.loadFromFile("Data.txt");
        std::cout << "Данные загружены.\n";
    } else {
        std::cout << "Загружены сохранённые данные.\n";
    }

    std::string line;
    std::cout << "> ";
    while (std::getline(std::cin, line)) {
        line = trim(line);
        if (line.empty()) { std::cout << "> "; continue; }

        std::string cmd = getCommand(line);

        if (cmd == "HELP") {
            printHelp();
        }
        else if (cmd == "EXIT" || cmd == "QUIT") {
            break;
        }
        else if (cmd == "STATUS") {
            std::cout << "Слотов времени: " << db.getTimes().size()
                      << ", аудиторий: " << db.getAuditories().size() << "\n";
        }
        else if (cmd == "CLEAR") {
            db.getSessionManager().clearSelection(uid);
            std::cout << "Выборка очищена.\n";
        }
        else if (cmd == "LOAD" || cmd == "SAVE") {
            std::istringstream iss(line);
            std::string tmp, fname;
            iss >> tmp >> fname;
            bool ok = (cmd == "LOAD") ? db.loadFromFile(fname) : db.saveToFile(fname);
            std::cout << (ok ? "Успешно.\n" : "Ошибка файла.\n");
        }
        else if (cmd == "GENERATE") {
            std::istringstream iss(line);
            std::string tmp;
            int a = 4, b = 10, c = 10, d = 8, e = 12;
            double f = 0.6;
            iss >> tmp >> a >> b >> c >> d >> e >> f;
            generateTestData(a, b, c, d, e, f);
            if (db.loadFromFile("Data.txt"))
                std::cout << "Сгенерировано в Data.txt и загружено.\n";
            else
                std::cout << "Ошибка загрузки Data.txt\n";
        }
        else if (cmd == "ADD") {
            std::istringstream iss(line);
            std::string c, day, aud, subj, teach;
            int lesson, grp;
            if (!(iss >> c >> day >> lesson >> aud >> subj >> teach >> grp)) {
                std::cout << "Формат: ADD day lesson aud subj teach group\n";
            } else {
                DateTime dt(day, lesson);
                int ti = -1, ai = -1;
                const auto& times = db.getTimes();
                const auto& auds = db.getAuditories();
                for (size_t i = 0; i < times.size(); ++i) if (times[i] == dt) { ti = static_cast<int>(i); break; }
                for (size_t i = 0; i < auds.size(); ++i) if (auds[i] == aud) { ai = static_cast<int>(i); break; }
                if (ti < 0 || ai < 0) {
                    std::cout << "Неизвестное время или аудитория.\n";
                } else {
                    Schedule_unit rec(subj, teach, grp, ti, ai);
                    std::cout << (db.insertRecord(rec) ? "Добавлено.\n" : "Ошибка (коллизия).\n");
                }
            }
        }
        else {
            // Все остальные команды (SELECT, RESELECT, REMOVE, PRINT ...)
            Command parsed = parse(line);
            if (!parsed.valid) {
                std::cout << "Ошибка: " << parsed.errorMsg << "\n";
            } else {
                db.execute(uid, parsed);
            }
        }
        std::cout << "> ";
    }

    std::cout << "Завершение.\n";
    return 0;
}