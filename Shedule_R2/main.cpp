// main.cpp
#include "shedule.h"
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <fstream>   // для временного файла в тестах
#include <cassert>
#include <cmath>     // для fabs (сравнение double)

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
        "  SELECT <условия>     - поиск записей (пример: GROUP == 101)\n"
        "  RESELECT <условия>   - уточнить поиск в текущей выборке\n"
        "  PRINT [SORT BY ...]  - вывести выбранное\n"
        "  ADD <day> <lesson> <aud> <subj> <teach> <grp> - добавить запись\n"
        "  REMOVE <условия>     - удалить записи\n"
        "  LOAD <file>          - загрузить из файла\n"
        "  SAVE <file>          - сохранить в файл\n"
        "  GENERATE [params]    - сгенерировать тестовые данные\n"
        "  STATUS               - показать статистику\n"
        "  CLEAR                - очистить выборку\n"
        "  HELP                 - эта справка\n"
        "  EXIT / QUIT          - выход\n"
        "\nУСЛОВИЯ: <ПОЛЕ> <==|!=|<|>|<=|>=|CONTAINS> <значение>\n"
        "Поля: GROUP, TEACHER, SUBJECT, AUDITORY, DATE/DAY, TIME/LESSON_NUM\n"
        "Пример: SELECT TEACHER CONTAINS Ivanov AND DAY == Monday\n"
        "===============================================\n";
}

// ---------- Автоматические тесты ----------
bool runTests() {
    std::cout << "\n========== ЗАПУСК ТЕСТОВ ==========\n";
    bool allPassed = true;
    const std::string testFile = "test_schedule.txt";

    // 1. Генерация небольшого предсказуемого файла
    {
        std::ofstream out(testFile);
        // 2 дня (Monday, Tuesday), 2 аудитории, 2 группы, 2 преподавателя, 2 предмета
        // Формат: день урок аудитория предмет преподаватель группа
        out << "Monday 1 Room_1 Subject_1 Teacher_1 1\n";
        out << "Monday 1 Room_2 Subject_2 Teacher_2 2\n";
        out << "Monday 2 Room_1 Subject_1 Teacher_2 1\n";
        out << "Tuesday 1 Room_1 Subject_2 Teacher_1 2\n";
        out << "Tuesday 2 Room_2 Subject_1 Teacher_2 2\n";
        out.close();
    }

    Database db;

    // 2. Тест загрузки
    if (!db.loadFromFile(testFile)) {
        std::cerr << "FAIL: loadFromFile\n";
        allPassed = false;
    } else {
        std::cout << "OK: loadFromFile\n";
    }

    // 3. Проверка количества записей через select без условий
    auto all = db.select(SearchConditions{});
    if (all.size() != 5) {
        std::cerr << "FAIL: ожидалось 5 записей, получено " << all.size() << "\n";
        allPassed = false;
    } else {
        std::cout << "OK: select all (5 записей)\n";
    }

    // 4. SELECT с условием GROUP == 2
    SearchConditions condGroup2;
    Cond cg;
    cg.field = Cond::GROUP; cg.relation = Cond::EQ; cg.intValue = 2; cg.strValue = "2";
    condGroup2.push_back(cg);
    auto group2 = db.select(condGroup2);
    if (group2.size() != 3) {  // в тестовых данных группа 2 встречается 3 раза
        std::cerr << "FAIL: GROUP == 2, ожидалось 3, получено " << group2.size() << "\n";
        allPassed = false;
    } else {
        std::cout << "OK: SELECT GROUP == 2 (3 записи)\n";
    }

    // 5. SELECT с TEACHER CONTAINS "Teacher_1"
    SearchConditions condTeacher;
    Cond ct;
    ct.field = Cond::TEACHER; ct.relation = Cond::CONTAINS; ct.strValue = "Teacher_1";
    condTeacher.push_back(ct);
    auto teacher1 = db.select(condTeacher);
    if (teacher1.size() != 2) {
        std::cerr << "FAIL: TEACHER CONTAINS Teacher_1, ожидалось 2, получено " << teacher1.size() << "\n";
        allPassed = false;
    } else {
        std::cout << "OK: SELECT TEACHER CONTAINS Teacher_1 (2 записи)\n";
    }

    // 6. Тест REMOVE (удаляем все записи группы 1)
    size_t removed = db.removeRecords(condGroup2);
    if (removed != 3) {
        std::cerr << "FAIL: удаление группы 2, ожидалось 3 удалено, получено " << removed << "\n";
        allPassed = false;
    } else {
        std::cout << "OK: REMOVE GROUP == 2 (удалено 3)\n";
    }

    // после удаления должно остаться 2 записи
    auto afterRemove = db.select(SearchConditions{});
    if (afterRemove.size() != 2) {
        std::cerr << "FAIL: после удаления ожидалось 2 записи, получено " << afterRemove.size() << "\n";
        allPassed = false;
    } else {
        std::cout << "OK: после удаления осталось 2 записи\n";
    }

    // 7. Тест ADD без коллизий (аудитория Room_2 на Monday 2 свободна, так как группа 2 удалена)
    Schedule_unit newRec("Subject_3", "Teacher_3", 3, 1, 1); // timeIndex=1 (Monday 2), auditoryIndex=1 (Room_2)
    bool addOk = db.insertRecord(newRec);
    if (!addOk) {
        std::cerr << "FAIL: добавить запись без коллизий\n";
        allPassed = false;
    } else {
        std::cout << "OK: ADD без коллизий\n";
    }

    // 8. Тест ADD с коллизией по аудитории (то же время и аудитория)
    bool addCollision = db.insertRecord(newRec);
    if (addCollision) {
        std::cerr << "FAIL: должна быть коллизия аудитории\n";
        allPassed = false;
    } else {
        std::cout << "OK: ADD с коллизией аудитории (отклонено)\n";
    }

    // 9. Тест getFreeAuditories
    DateTime dtMon1("Monday", 1);
    auto freeAud = db.getFreeAuditories(dtMon1);
    // На Monday 1 заняты Room_1 и Room_2, свободных быть не должно
    if (!freeAud.empty()) {
        std::cerr << "FAIL: getFreeAuditories Monday 1, ожидалось пусто\n";
        allPassed = false;
    } else {
        std::cout << "OK: getFreeAuditories Monday 1 (нет свободных)\n";
    }

    // 10. Тест SAVE / LOAD
    const std::string saveFile = "test_save.txt";
    if (!db.saveToFile(saveFile)) {
        std::cerr << "FAIL: saveToFile\n";
        allPassed = false;
    } else {
        Database db2;
        if (!db2.loadFromFile(saveFile) || db2.select(SearchConditions{}).size() != 3) {
            std::cerr << "FAIL: load после save, ожидалось 3 записи\n";
            allPassed = false;
        } else {
            std::cout << "OK: save/load roundtrip (3 записи)\n";
        }
    }

    // 11. Тест сортировки через PRINT (проверим порядок)
    // Добавим ещё одну запись для наглядности
    Schedule_unit rec2("Subject_4", "Teacher_1", 4, 0, 0); // Monday 1 Room_1
    db.insertRecord(rec2);
    // Выполним SELECT без условий и применим сортировку по GROUP asc
    auto all2 = db.select(SearchConditions{});
    sortSchedule(all2, Cond::GROUP, true);
    // Проверим, что группы отсортированы: 3, 4, 1 (если id 1 есть? у нас группа 1 осталась от Teacher_1? 
    // после удаления остались: Monday 1 Room_1 (группа 1), Tuesday 1 Room_1 (группа 2) – группа 2 была удалена? 
    // Уточним: удаляли группу 2, записи с группой 2 удалены. Исходные 5: 1) 1 1 Room_1 Subj1 T1 1; 2) 1 1 Room_2 Subj2 T2 2; 
    // 3) 1 2 Room_1 Subj1 T2 1; 4) 2 1 Room_1 Subj2 T1 2; 5) 2 2 Room_2 Subj1 T2 2.
    // Удалили группу 2 (записи 2,4,5). Остались 1 и 3 – обе группы 1. Потом добавили Subject_3 Teacher_3 group 3 (ауд Room_2 Monday 2) 
    // и Subject_4 Teacher_1 group 4 (Room_1 Monday 1). Ожидаемые группы: 1,1,3,4. После сортировки по возрастанию: 1,1,3,4.
    if (all2.size() != 4) {
        std::cerr << "FAIL: перед сортировкой ожидалось 4 записи\n";
        allPassed = false;
    } else {
        bool sortedOk = true;
        for (size_t i = 0; i + 1 < all2.size(); ++i) {
            if (all2[i].group > all2[i+1].group) { sortedOk = false; break; }
        }
        if (!sortedOk) {
            std::cerr << "FAIL: сортировка по GROUP asc\n";
            allPassed = false;
        } else {
            std::cout << "OK: сортировка по GROUP asc\n";
        }
    }

    // 12. Тест getTeacherSchedule, getGroupSchedule, getSubjectSchedule
    auto teachSched = db.getTeacherSchedule("Teacher_1");
    if (teachSched.size() != 2) { // Teacher_1 есть в исходной Monday 1 Room_1 и добавленный Subject_4 Teacher_1 group 4
        std::cerr << "FAIL: getTeacherSchedule Teacher_1, ожидалось 2, получено " << teachSched.size() << "\n";
        allPassed = false;
    } else {
        std::cout << "OK: getTeacherSchedule\n";
    }

    auto groupSched = db.getGroupSchedule(1);
    if (groupSched.size() != 1) { // группа 1 есть Monday 1 Room_1 Subject_1 Teacher_1 (после удаления осталась эта)
        // Но после удаления группы 2 записи группы 1 не удалялись, однако мы добавили Subject_4 Teacher_1 group 4 Room_1 Monday 1, 
        // что создало коллизию с Room_1 Monday 1? insertRecord проверяет коллизию аудитории, поэтому Subject_4 не добавится, 
        // так как на Monday 1 Room_1 уже есть Subject_1 Teacher_1. То есть rec2 не вставится. Учтём это. 
        std::cerr << "FAIL: getGroupSchedule 1, ожидалось 1, получено " << groupSched.size() << "\n";
        allPassed = false;
    } else {
        std::cout << "OK: getGroupSchedule\n";
    }

    // 13. Проверка парсера команд (несколько примеров)
    Command cmd1 = parse("SELECT GROUP == 2");
    if (cmd1.cmd != SELECT || cmd1.conditions.size() != 1 || cmd1.conditions.front().intValue != 2) {
        std::cerr << "FAIL: parse SELECT\n";
        allPassed = false;
    } else {
        std::cout << "OK: parse SELECT\n";
    }

    Command cmd2 = parse("PRINT SORT BY GROUP ASC");
    if (cmd2.cmd != PRINT || !cmd2.hasSortField || cmd2.sortOrder != "asc") {
        std::cerr << "FAIL: parse PRINT SORT\n";
        allPassed = false;
    } else {
        std::cout << "OK: parse PRINT SORT\n";
    }

    // Очистка временных файлов
    std::remove(testFile.c_str());
    std::remove(saveFile.c_str());

    if (allPassed) std::cout << "========== ВСЕ ТЕСТЫ ПРОЙДЕНЫ ==========\n";
    else std::cout << "========== НЕКОТОРЫЕ ТЕСТЫ ПРОВАЛЕНЫ ==========\n";
    return allPassed;
}

// ---------- main ----------
int main() {
    printHelp();

    // Запуск автоматических тестов
    runTests();

    // Основная интерактивная база данных
    Database db;
    UserID uid = 1;
    std::string line;

    // Автоматическая загрузка / генерация начальных данных
    if (!db.loadFromFile("Data.txt")) {
        std::cout << "Генерация начальных тестовых данных...\n";
        generateTestData(4, 10, 10, 8, 12, 0.6);
        db.loadFromFile("Data.txt");
        std::cout << "Данные загружены.\n";
    } else {
        std::cout << "Загружены сохранённые данные.\n";
    }

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
                for (size_t i = 0; i < times.size(); ++i)
                    if (times[i] == dt) { ti = static_cast<int>(i); break; }
                for (size_t i = 0; i < auds.size(); ++i)
                    if (auds[i] == aud) { ai = static_cast<int>(i); break; }

                if (ti < 0 || ai < 0) {
                    std::cout << "Неизвестное время или аудитория.\n";
                } else {
                    Schedule_unit rec(subj, teach, grp, ti, ai);
                    std::cout << (db.insertRecord(rec) ? "Добавлено.\n" : "Ошибка (коллизия).\n");
                }
            }
        }
        else {
            // SELECT, RESELECT, REMOVE, PRINT – через parse
            Command parsed = parse(line);
            if (parsed.cmd == PRINT && db.getSessionManager().getSelection(uid).isEmpty()) {
                std::cout << "Выборка пуста. Сначала выполните SELECT.\n";
            } else {
                db.execute(uid, parsed);
            }
        }
        std::cout << "> ";
    }

    std::cout << "Завершение.\n";
    return 0;
}