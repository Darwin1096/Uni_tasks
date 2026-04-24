// main.cpp
#include "shedule.h"
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>

// Вспомогательные функции
static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string toUpper(std::string s) {
    // ИСПРАВЛЕНО: явное приведение char, чтобы избежать ошибки int->string
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

int main() {
    printHelp();
    generateTestData(4, 10, 10, 8, 12, 0.6);
    Database db;
    UserID uid = 1;
    std::string line;
    
    std::cout << "> ";
    while (std::getline(std::cin, line)) {
        line = trim(line);
        if (line.empty()) { std::cout << "> "; continue; }
        
        std::string cmd = getCommand(line);
        
        if (cmd == "HELP") { printHelp(); }
        else if (cmd == "EXIT" || cmd == "QUIT") { break; }
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
            std::string tmp; int a=4,b=10,c=10,d=8,e=12; double f=0.6;
            iss >> tmp >> a >> b >> c >> d >> e >> f;
            generateTestData(a,b,c,d,e,f);
            std::cout << "Сгенерировано в test_data.txt. Используйте LOAD.\n";
        }
        else if (cmd == "ADD") {
            std::istringstream iss(line);
            std::string c,day,aud,subj,teach; int lesson,grp;
            if (!(iss >> c >> day >> lesson >> aud >> subj >> teach >> grp)) {
                std::cout << "Формат: ADD day lesson aud subj teach group\n";
            } else {
                DateTime dt(day, lesson);
                int ti=-1, ai=-1;
                const auto& times = db.getTimes();
                const auto& auds = db.getAuditories();
                for(size_t i=0;i<times.size();++i) if(times[i]==dt){ti=int(i);break;}
                for(size_t i=0;i<auds.size();++i) if(auds[i]==aud){ai=int(i);break;}
                
                if(ti<0||ai<0) { std::cout << "Неизвестное время или аудитория.\n"; }
                /*
                else if(db.checkCollision(teach,dt)) { std::cout << "Преподаватель занят.\n"; }
                else if(db.checkCollision(aud,dt)) { std::cout << "Аудитория занята.\n"; }
                */
                else {
                    Schedule_unit rec(subj,teach,grp,ti,ai);
                    std::cout << (db.insertRecord(rec) ? "Добавлено.\n" : "Ошибка.\n");
                }
            }
        }
        else {
            // Все остальные команды делегируем парсеру
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