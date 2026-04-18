#include "shedule.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
    std::string trim(const std::string& str) {
        size_t start = 0;
        while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start]))) {
            ++start;
        }
        if (start == str.length()) {
            return "";
        }
        size_t end = str.length() - 1;
        while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
            --end;
        }
        return str.substr(start, end - start + 1);
    }

    std::string toLower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
        return str;
    }

    Field parseField(const std::string& fieldStr) {
        std::string f = toLower(fieldStr);
        if (f == "group") return Field::GROUP;
        if (f == "teacher") return Field::TEACHER;
        if (f == "subject") return Field::SUBJECT;
        if (f == "auditory") return Field::AUDITORY;
        if (f == "date" || f == "weekday") return Field::DATE;
        if (f == "time" || f == "lesson_num") return Field::TIME;
        return Field::GROUP; // По умолчанию
    }

    Relation parseRelation(const std::string& op) {
        if (op == "<" || op == "LT") return Relation::LT;
        if (op == ">" || op == "GT") return Relation::GT;
        return Relation::EQ;
    }

    bool parseCondition(const std::string& condition, Cond& cond) {
        size_t pos_op = std::string::npos;
        const std::vector<std::string> operators = {"<=", ">=", "<", ">", "="};
        
        for (const auto& op_str : operators) {
            pos_op = condition.find(op_str);
            if (pos_op != std::string::npos) {
                std::string fieldStr = trim(condition.substr(0, pos_op));
                std::string op = op_str;
                std::string value = trim(condition.substr(pos_op + op.length()));

                cond.field = parseField(fieldStr);
                cond.relation = parseRelation(op);
                cond.value_str = value;
                return true;
            }
        }
        return false;
    }
}

Command parse(const std::string& query) {
    Command result;
    result.cmd = CommandType::PRINT; // По умолчанию
    result.conditions.clear();

    std::istringstream iss(query);
    std::string token;

    if (!(iss >> token)) {
        return result;
    }

    std::string cmd_lower = toLower(token);

    if (cmd_lower == "add") {
        result.cmd = CommandType::ADD;
    } else if (cmd_lower == "delete") {
        result.cmd = CommandType::DELETE;
    } else if (cmd_lower == "select") {
        result.cmd = CommandType::SELECT;
    } else if (cmd_lower == "save") {
        result.cmd = CommandType::SAVE;
    } else if (cmd_lower == "load") {
        result.cmd = CommandType::LOAD;
    } else if (cmd_lower == "print") {
        result.cmd = CommandType::PRINT;
    } else {
        return result; // Неизвестная команда
    }

    std::string line;
    std::getline(iss, line);
    line = trim(line);

    if (line.empty()) {
        return result;
    }

    size_t where_pos = toLower(line).find("where");
    
    if (where_pos != std::string::npos) {
        std::string where_part = trim(line.substr(where_pos + 5));

        // Парсинг условий после WHERE
        std::istringstream conditions_iss(where_part);
        std::string cond_token;
        std::string full_condition;
        
        while (conditions_iss >> cond_token) {
            if (toLower(cond_token) == "and") {
                if (!full_condition.empty()) {
                    Cond cond;
                    if (parseCondition(full_condition, cond)) {
                        result.conditions.push_back(cond);
                    }
                    full_condition.clear();
                }
            } else {
                if (!full_condition.empty()) {
                    full_condition += " ";
                }
                full_condition += cond_token;
            }
        }
        
        if (!full_condition.empty()) {
            Cond cond;
            if (parseCondition(full_condition, cond)) {
                result.conditions.push_back(cond);
            }
        }
    }

    return result;
}

bool match(const Schedule_unit& unit, const SearchConditions& conditions) {
    for (const auto& cond : conditions) {
        std::string unit_val_str;
        int unit_val_int = 0;
        bool isInt = false;

        switch (cond.field) {
            case Field::GROUP:
                unit_val_int = unit.group;
                isInt = true;
                break;
            case Field::TEACHER:
                unit_val_str = unit.teacher;
                break;
            case Field::SUBJECT:
                unit_val_str = unit.subject;
                break;
            case Field::AUDITORY:
                unit_val_str = unit.auditory;
                break;
            case Field::DATE:
                unit_val_str = unit.dt.weekday;
                break;
            case Field::TIME:
                unit_val_int = unit.dt.lesson_num;
                isInt = true;
                break;
        }

        if (isInt) {
            try {
                int val_int = std::stoi(cond.value_str);
                if (cond.relation == Relation::EQ && unit_val_int != val_int) return false;
                else if (cond.relation == Relation::LT && unit_val_int >= val_int) return false;
                else if (cond.relation == Relation::GT && unit_val_int <= val_int) return false;
            } catch (...) {
                return false;
            }
        } else {
            if (cond.relation == Relation::EQ && toLower(unit_val_str) != toLower(cond.value_str)) {
                return false;
            }
            // Для строк поддерживаем только EQ
        }
    }
    return true;
}

void Session::addSelectedTimes(UserID userId, const std::vector<DateTime>& times) {
    // В данной реализации userId не используется, но может быть использован для многопользовательского режима
    for (const auto& dt : times) {
        // Логика добавления временных слотов
        // В текущей структуре SelectedItems хранятся Indexes, а не DateTime
        // Возможно, потребуется доработка структуры
    }
}

void Session::addSelectedUnits(UserID userId, const std::vector<Schedule_unit>& units) {
    // В данной реализации userId не используется
    // Логика добавления выбранных единиц расписания
}

Database::Database() {}

bool Database::loadFromFile(const std::string& filename) {
    data.clear();
    times.clear();
    auditories.clear();
    teacherIndex.clear();
    subjectIndex.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream record_stream(line);
        Schedule_unit unit;
        std::string weekday;
        
        // Формат: subject teacher group weekday lesson_num auditory
        if (record_stream >> unit.subject >> unit.teacher >> unit.group >> weekday >> unit.dt.lesson_num >> unit.auditory) {
            unit.dt.weekday = weekday;
            
            // Добавляем в данные
            data.push_back({unit});
            
            // Обновляем индексы
            TeacherSchedule& ts = teacherIndex[unit.teacher];
            Indexes idx(static_cast<int>(times.size()), static_cast<int>(auditories.size()));
            ts.addPosition(idx);
            
            SubjectSchedule& ss = subjectIndex[unit.subject];
            ss.addPosition(idx);
            
            // Обновляем списки времен и аудиторий (упрощенно)
            bool timeFound = false;
            for (const auto& t : times) {
                if (t.weekday == weekday && t.lesson_num == unit.dt.lesson_num) {
                    timeFound = true;
                    break;
                }
            }
            if (!timeFound) {
                times.push_back(unit.dt);
            }
            
            bool audFound = false;
            for (const auto& a : auditories) {
                if (a == unit.auditory) {
                    audFound = true;
                    break;
                }
            }
            if (!audFound) {
                auditories.push_back(unit.auditory);
            }
        }
    }
    file.close();
    return true;
}

bool Database::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file for writing: " << filename << std::endl;
        return false;
    }

    for (const auto& row : data) {
        for (const auto& unit : row) {
            file << unit.subject << " " << unit.teacher << " " << unit.group << " "
                 << unit.dt.weekday << " " << unit.dt.lesson_num << " "
                 << unit.auditory << "\n";
        }
    }
    file.close();
    return true;
}

bool Database::execute(UserID userId, const Command& cmd) {
    Session& session = sessionsManager.getSession(userId);

    switch (cmd.cmd) {
        case CommandType::ADD: {
            // Логика добавления записи
            // Требуется дополнительная информация о добавляемой записи
            break;
        }
        case CommandType::DELETE: {
            // Удаление записей по условиям
            auto it = std::remove_if(data.begin(), data.end(),
                [&](const std::vector<Schedule_unit>& row) {
                    for (const auto& unit : row) {
                        if (match(unit, cmd.conditions)) {
                            return true;
                        }
                    }
                    return false;
                });
            data.erase(it, data.end());
            break;
        }
        case CommandType::SELECT: {
            std::vector<Schedule_unit> results;
            for (const auto& row : data) {
                for (const auto& unit : row) {
                    if (match(unit, cmd.conditions)) {
                        results.push_back(unit);
                    }
                }
            }
            session.addSelectedUnits(userId, results);
            break;
        }
        case CommandType::SAVE: {
            // Аргументы команды нужно передавать отдельно или парсить из условий
            return saveToFile("Data.txt");
        }
        case CommandType::LOAD: {
            return loadFromFile("Data.txt");
        }
        case CommandType::PRINT: {
            for (const auto& row : data) {
                for (const auto& unit : row) {
                    if (cmd.conditions.empty() || match(unit, cmd.conditions)) {
                        std::cout << unit.subject << " " << unit.teacher << " " << unit.group << " "
                                  << unit.dt.weekday << " " << unit.dt.lesson_num << " "
                                  << unit.auditory << std::endl;
                    }
                }
            }
            break;
        }
    }
    return true;
}