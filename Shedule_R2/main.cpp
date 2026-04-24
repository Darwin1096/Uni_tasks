#include "shedule.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

std::string toLowerStr(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return str;
}

void printMenu() {
    std::cout << "\n=== Доступные команды ===\n";
    std::cout << "ADD <group> <teacher> <subject> <auditory> <weekday> <lesson_num>\n";
    std::cout << "   Пример: ADD 101 Ivanov Math A101 Monday 1\n";
    std::cout << "DELETE where <field> <op> <value> [and ...]\n";
    std::cout << "   Поля: group, teacher, subject, auditory, date(weekday), time(lesson_num)\n";
    std::cout << "   Операторы: <, >, =\n";
    std::cout << "SELECT [fields] where <field> <op> <value> [and ...]\n";
    std::cout << "PRINT - Вывести все записи или выбранные (после SELECT)\n";
    std::cout << "SAVE <filename>\n";
    std::cout << "LOAD <filename>\n";
    std::cout << "EXIT\n";
    std::cout << "=========================\n> ";
}

void printUnit(const Schedule_unit& unit) {
    std::cout << "Group: " << unit.group 
              << ", Teacher: " << unit.teacher 
              << ", Subject: " << unit.subject 
              << ", Auditory: " << unit.auditory 
              << ", Day: " << unit.dt.weekday 
              << ", Lesson: " << unit.dt.lesson_num 
              << std::endl;
}

int main() {
    Database db;
    const UserID userId = 1;

    std::cout << "Генерация тестовых данных...\n";
    generateTestData(4, 10, 10, 8, 12, 0.6);
    std::cout << "Данные сгенерированы в файл Data.txt\n";

    std::cout << "Загрузка данных из файла...\n";
    if (!db.loadFromFile("Data.txt")) {
        std::cerr << "Ошибка загрузки данных!\n";
        return 1;
    }
    std::cout << "Загружено записей: " << db.getTimes().size() << " (внутреннее представление)\n";

    std::cout << "\nСистема управления расписанием готова.\n";
    printMenu();

    std::string input_line;
    while (true) {
        if (!std::getline(std::cin, input_line)) {
            break;
        }

        if (input_line.empty()) {
            continue;
        }

        std::string first_word;
        std::istringstream iss_check(input_line);
        if (iss_check >> first_word) {
            if (toLowerStr(first_word) == "exit") {
                std::cout << "Выход.\n";
                break;
            }
        }

        Command cmd = parse(input_line);

        if (cmd.cmd == CommandType::PRINT && cmd.conditions.empty()) {
            // Особый случай: PRINT без условий должен вывести всё из базы
            // Но так как у нас нет прямого доступа к данным извне,
            // попробуем выполнить команду через execute, который должен это обработать
        }

        if (!db.execute(userId, cmd)) {
            std::cerr << "Ошибка выполнения команды или неверный формат.\n";
            printMenu();
            continue;
        }

        // Если была команда SELECT, выведем результаты из сессии
        if (cmd.cmd == CommandType::SELECT) {
            Session& session = const_cast<Session&>(db.getSessionsManager().getSession(userId)); 
            // Примечание: нужно добавить геттер в Database для sessionsManager
            
            if (session.selected_items.selected_positions.empty()) {
                std::cout << "Ничего не найдено.\n";
            } else {
                std::cout << "--- Найдено элементов: " << session.selected_items.selected_positions.size() << " ---\n";
                // Здесь нужно получить данные по индексам, но прямой доступ сложен без дополнительных методов
                // Для простоты покажем количество
                std::cout << "(Для просмотра деталей используйте расширенный PRINT или доработайте вывод)\n";
            }
        }
        
        if (cmd.cmd == CommandType::PRINT) {
             // PRINT внутри execute уже должен был вывести данные, если реализовано верно
             // Если нет, то нужна доработка execute или дополнительный метод получения данных
        }

        std::cout << "> ";
    }

    return 0;
}