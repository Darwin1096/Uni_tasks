// shedule.cpp
#include "shedule.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <set>
#include <tuple>
#include <cctype>
#include <stdexcept>
#include <iomanip>

/* ======================== Cond ======================== */
Cond::Cond() : field(GROUP), relation(EQ), strValue(""), intValue(0) {}

/* ======================== DateTime ======================== */
DateTime::DateTime() : weekday(""), lesson_num(0) {}
DateTime::DateTime(const std::string& day, int num) : weekday(day), lesson_num(num) {}

std::string DateTime::toString() const {
    return weekday + " " + std::to_string(lesson_num);
}

bool DateTime::operator==(const DateTime& other) const {
    return weekday == other.weekday && lesson_num == other.lesson_num;
}

bool DateTime::operator<(const DateTime& other) const {
    static const std::vector<std::string> dayOrder = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
    };
    auto it1 = std::find(dayOrder.begin(), dayOrder.end(), weekday);
    auto it2 = std::find(dayOrder.begin(), dayOrder.end(), other.weekday);
    int idx1 = static_cast<int>(std::distance(dayOrder.begin(), it1));
    int idx2 = static_cast<int>(std::distance(dayOrder.begin(), it2));
    if (idx1 != idx2) return idx1 < idx2;
    return lesson_num < other.lesson_num;
}

/* ======================== Schedule_unit ======================== */
Schedule_unit::Schedule_unit() : subject(""), teacher(""), group(0), timeIndex(-1), auditoryIndex(-1) {}
Schedule_unit::Schedule_unit(const std::string& subj, const std::string& teach, 
                              int gr, int tIdx, int aIdx)
    : subject(subj), teacher(teach), group(gr), timeIndex(tIdx), auditoryIndex(aIdx) {}

/* ======================== Indexes ======================== */
Indexes::Indexes(int t, int a) : timeIndex(t), auditoryIndex(a) {}
bool Indexes::operator==(const Indexes& other) const {
    return timeIndex == other.timeIndex && auditoryIndex == other.auditoryIndex;
}
bool Indexes::operator<(const Indexes& other) const {
    if (timeIndex != other.timeIndex) return timeIndex < other.timeIndex;
    return auditoryIndex < other.auditoryIndex;
}

/* ======================== TeacherSchedule / SubjectSchedule / GroupSchedule ======================== */
void TeacherSchedule::addPosition(const Indexes& position) { positions.push_back(position); }
void TeacherSchedule::clear() { positions.clear(); }
bool TeacherSchedule::isEmpty() const { return positions.empty(); }
size_t TeacherSchedule::size() const { return positions.size(); }

void SubjectSchedule::addPosition(const Indexes& position) { positions.push_back(position); }
void SubjectSchedule::clear() { positions.clear(); }
bool SubjectSchedule::isEmpty() const { return positions.empty(); }
size_t SubjectSchedule::size() const { return positions.size(); }

void GroupSchedule::addPosition(const Indexes& position) { positions.push_back(position); }
void GroupSchedule::clear() { positions.clear(); }
bool GroupSchedule::isEmpty() const { return positions.empty(); }
size_t GroupSchedule::size() const { return positions.size(); }

/* ======================== SelectedItems ======================== */
void SelectedItems::clear() { selected_positions.clear(); }
bool SelectedItems::isEmpty() const { return selected_positions.empty(); }
size_t SelectedItems::size() const { return selected_positions.size(); }

/* ======================== Session ======================== */
void Session::clear() { selectedItems.clear(); }

/* ======================== SessionManager ======================== */
Session& SessionManager::getSession(UserID id) { return userSessions[id]; }
void SessionManager::clearSelection(UserID id) {
    auto it = userSessions.find(id);
    if (it != userSessions.end()) it->second.clear();
}
void SessionManager::addToSelection(UserID id, const Indexes& idx) {
    userSessions[id].selectedItems.selected_positions.push_back(idx);
}
void SessionManager::setSelection(UserID id, const std::vector<Indexes>& positions) {
    userSessions[id].selectedItems.selected_positions = positions;
}
const SelectedItems& SessionManager::getSelection(UserID id) const {
    static const SelectedItems empty;
    auto it = userSessions.find(id);
    return (it != userSessions.end()) ? it->second.selectedItems : empty;
}

/* ======================== Command ======================== */
Command::Command() : cmd(ADD), hasSortField(false), sortFieldVal(Cond::GROUP),
                     sortOrder("asc"), valid(true), fullOutput(false) {}

/* ======================== Database: private methods ======================== */
int Database::findTimeIndex(const DateTime& dt) const {
    for (size_t i = 0; i < times.size(); ++i)
        if (times[i] == dt) return static_cast<int>(i);
    return -1;
}
int Database::findAuditoryIndex(const Auditory& aud) const {
    for (size_t i = 0; i < auditories.size(); ++i)
        if (auditories[i] == aud) return static_cast<int>(i);
    return -1;
}
bool Database::hasCollision(int timeIndex, int auditoryIndex) const {
    if (timeIndex < 0 || timeIndex >= static_cast<int>(data.size())) return false;
    if (auditoryIndex < 0 || auditoryIndex >= static_cast<int>(data[timeIndex].size())) return false;
    return data[timeIndex][auditoryIndex] != nullptr;
}

/* ======================== Database: public methods ======================== */
Database::Database() {}

bool Database::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    std::string line;
    std::set<Auditory> audSet;
    std::set<DateTime> timeSet;
    std::vector<std::tuple<std::string, int, std::string, std::string, std::string, int>> records;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string day, auditory, subject, teacher;
        int lessonNum, group;
        if (!(iss >> day >> lessonNum >> auditory >> subject >> teacher >> group)) continue;
        
        DateTime dt(day, lessonNum);
        timeSet.insert(dt);
        audSet.insert(auditory);
        records.emplace_back(day, lessonNum, auditory, subject, teacher, group);
    }
    file.close();
    
    times.assign(timeSet.begin(), timeSet.end());
    auditories.assign(audSet.begin(), audSet.end());
    
    data.resize(times.size());
    for (auto& row : data) row.resize(auditories.size());
    
    for (const auto& rec : records) {
        std::string day, auditory, subject, teacher;
        int lessonNum, group;
        std::tie(day, lessonNum, auditory, subject, teacher, group) = rec;
        
        DateTime dt(day, lessonNum);
        int tIdx = findTimeIndex(dt);
        int aIdx = findAuditoryIndex(auditory);
        if (tIdx >= 0 && aIdx >= 0) {
            auto unit = std::make_unique<Schedule_unit>(subject, teacher, group, tIdx, aIdx);
            data[tIdx][aIdx] = std::move(unit);
            teacherIndex[teacher].addPosition(Indexes(tIdx, aIdx));
            subjectIndex[subject].addPosition(Indexes(tIdx, aIdx));
            groupIndex[group].addPosition(Indexes(tIdx, aIdx));
        }
    }
    return true;
}

bool Database::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    for (size_t t = 0; t < data.size(); ++t) {
        for (size_t a = 0; a < data[t].size(); ++a) {
            if (data[t][a] != nullptr) {
                const auto& unit = *data[t][a];
                file << times[t].weekday << " " 
                     << times[t].lesson_num << " "
                     << auditories[a] << " "
                     << unit.subject << " "
                     << unit.teacher << " "
                     << unit.group << "\n";
            }
        }
    }
    file.close();
    return true;
}

bool Database::execute(UserID userId, const Command& cmd) {
    if (!cmd.valid) {
        std::cout << "Ошибка: " << cmd.errorMsg << "\n";
        return false;
    }

    switch (cmd.cmd) {
        case ADD:
            return insertRecord(cmd.newRecord);
            
        case REMOVE:
            removeRecords(cmd.removeConditions);
            return true;
            
        case SELECT: {
            auto results = select(cmd.conditions);
            std::vector<Indexes> positions;
            for (const auto& unit : results)
                positions.emplace_back(unit.timeIndex, unit.auditoryIndex);
            sessionManager.setSelection(userId, positions);
            return true;
        }
            
        case RESELECT: {
            const auto& current = sessionManager.getSelection(userId);
            std::vector<Indexes> filtered;
            for (const auto& idx : current.selected_positions) {
                if (idx.timeIndex >= 0 && idx.timeIndex < static_cast<int>(data.size()) &&
                    idx.auditoryIndex >= 0 && idx.auditoryIndex < static_cast<int>(data[idx.timeIndex].size()) &&
                    data[idx.timeIndex][idx.auditoryIndex] != nullptr) {
                    if (match(*data[idx.timeIndex][idx.auditoryIndex], *this, cmd.conditions))
                        filtered.push_back(idx);
                }
            }
            sessionManager.setSelection(userId, filtered);
            return true;
        }
            
        case PRINT: {
            std::vector<Indexes> positions;
            if (cmd.fullOutput) {
                // PRINT без аргументов — вся матрица
                for (size_t t = 0; t < data.size(); ++t)
                    for (size_t a = 0; a < data[t].size(); ++a)
                        if (data[t][a] != nullptr)
                            positions.emplace_back(static_cast<int>(t), static_cast<int>(a));
            }
            else if (cmd.conditions.empty()) {
                // PRINT SELECT — текущая выборка
                const auto& selection = sessionManager.getSelection(userId);
                if (selection.isEmpty()) {
                    std::cout << "Выборка пуста. Сначала выполните SELECT.\n";
                    return true;
                }
                positions = selection.selected_positions;
            }
            else {
                // Быстрый поиск по условию
                auto results = select(cmd.conditions);
                if (results.empty()) {
                    std::cout << "Ничего не найдено.\n";
                    return true;
                }
                for (const auto& unit : results)
                    positions.emplace_back(unit.timeIndex, unit.auditoryIndex);
            }
            printMatrix(positions, *this);
            return true;
        }
            
        case SAVE:
        case LOAD:
            return false;
            
        default:
            return false;
    }
}

bool Database::insertRecord(const Schedule_unit& record) {
    if (hasCollision(record.timeIndex, record.auditoryIndex)) return false;
    
    auto tIt = teacherIndex.find(record.teacher);
    if (tIt != teacherIndex.end()) {
        for (const auto& pos : tIt->second.positions)
            if (pos.timeIndex == record.timeIndex) return false;
    }
    auto gIt = groupIndex.find(record.group);
    if (gIt != groupIndex.end()) {
        for (const auto& pos : gIt->second.positions)
            if (pos.timeIndex == record.timeIndex) return false;
    }
    
    auto unit = std::make_unique<Schedule_unit>(record);
    data[record.timeIndex][record.auditoryIndex] = std::move(unit);
    
    teacherIndex[record.teacher].addPosition(Indexes(record.timeIndex, record.auditoryIndex));
    subjectIndex[record.subject].addPosition(Indexes(record.timeIndex, record.auditoryIndex));
    groupIndex[record.group].addPosition(Indexes(record.timeIndex, record.auditoryIndex));
    return true;
}

size_t Database::removeRecords(const SearchConditions& conditions) {
    size_t removed = 0;
    for (size_t t = 0; t < data.size(); ++t) {
        for (size_t a = 0; a < data[t].size(); ++a) {
            if (data[t][a] != nullptr && match(*data[t][a], *this, conditions)) {
                const auto& unit = *data[t][a];
                
                auto tIt = teacherIndex.find(unit.teacher);
                if (tIt != teacherIndex.end()) {
                    auto& pos = tIt->second.positions;
                    pos.erase(std::remove(pos.begin(), pos.end(), Indexes(static_cast<int>(t), static_cast<int>(a))), pos.end());
                }
                auto sIt = subjectIndex.find(unit.subject);
                if (sIt != subjectIndex.end()) {
                    auto& pos = sIt->second.positions;
                    pos.erase(std::remove(pos.begin(), pos.end(), Indexes(static_cast<int>(t), static_cast<int>(a))), pos.end());
                }
                auto gIt = groupIndex.find(unit.group);
                if (gIt != groupIndex.end()) {
                    auto& pos = gIt->second.positions;
                    pos.erase(std::remove(pos.begin(), pos.end(), Indexes(static_cast<int>(t), static_cast<int>(a))), pos.end());
                }
                data[t][a].reset();
                ++removed;
            }
        }
    }
    return removed;
}

std::vector<Schedule_unit> Database::select(const SearchConditions& conditions) const {
    std::vector<Schedule_unit> results;
    for (size_t t = 0; t < data.size(); ++t) {
        for (size_t a = 0; a < data[t].size(); ++a) {
            if (data[t][a] != nullptr && match(*data[t][a], *this, conditions))
                results.push_back(*data[t][a]);
        }
    }
    return results;
}

std::vector<Schedule_unit> Database::getTeacherSchedule(const std::string& teacher) const {
    std::vector<Schedule_unit> result;
    auto it = teacherIndex.find(teacher);
    if (it != teacherIndex.end()) {
        for (const auto& idx : it->second.positions) {
            if (idx.timeIndex >= 0 && idx.timeIndex < static_cast<int>(data.size()) &&
                idx.auditoryIndex >= 0 && idx.auditoryIndex < static_cast<int>(data[idx.timeIndex].size()) &&
                data[idx.timeIndex][idx.auditoryIndex] != nullptr)
                result.push_back(*data[idx.timeIndex][idx.auditoryIndex]);
        }
    }
    return result;
}

std::vector<Schedule_unit> Database::getGroupSchedule(int group) const {
    std::vector<Schedule_unit> result;
    auto it = groupIndex.find(group);
    if (it != groupIndex.end()) {
        for (const auto& idx : it->second.positions) {
            if (idx.timeIndex >= 0 && idx.timeIndex < static_cast<int>(data.size()) &&
                idx.auditoryIndex >= 0 && idx.auditoryIndex < static_cast<int>(data[idx.timeIndex].size()) &&
                data[idx.timeIndex][idx.auditoryIndex] != nullptr)
                result.push_back(*data[idx.timeIndex][idx.auditoryIndex]);
        }
    }
    return result;
}

std::vector<Schedule_unit> Database::getSubjectSchedule(const std::string& subject) const {
    std::vector<Schedule_unit> result;
    auto it = subjectIndex.find(subject);
    if (it != subjectIndex.end()) {
        for (const auto& idx : it->second.positions) {
            if (idx.timeIndex >= 0 && idx.timeIndex < static_cast<int>(data.size()) &&
                idx.auditoryIndex >= 0 && idx.auditoryIndex < static_cast<int>(data[idx.timeIndex].size()) &&
                data[idx.timeIndex][idx.auditoryIndex] != nullptr)
                result.push_back(*data[idx.timeIndex][idx.auditoryIndex]);
        }
    }
    return result;
}

std::vector<Auditory> Database::getFreeAuditories(const DateTime& dt) const {
    std::vector<Auditory> free;
    int tIdx = findTimeIndex(dt);
    if (tIdx < 0) return auditories;
    for (size_t a = 0; a < auditories.size(); ++a)
        if (tIdx < static_cast<int>(data.size()) && a < data[tIdx].size() && data[tIdx][a] == nullptr)
            free.push_back(auditories[a]);
    return free;
}

const Schedule_unit* Database::getUnit(int t, int a) const {
    if (t < 0 || t >= static_cast<int>(data.size()) ||
        a < 0 || a >= static_cast<int>(data[t].size()))
        return nullptr;
    return data[t][a].get();
}

const std::vector<DateTime>& Database::getTimes() const { return times; }
const std::vector<Auditory>& Database::getAuditories() const { return auditories; }
SessionManager& Database::getSessionManager() { return sessionManager; }
const SessionManager& Database::getSessionManager() const { return sessionManager; }

/* ======================== match ======================== */
bool match(const Schedule_unit& unit, const Database& db, const SearchConditions& conditions) {
    for (const auto& cond : conditions) {
        std::string strVal;
        int intVal = 0;
        bool isStringField = true;
        
        switch (cond.field) {
            case Cond::GROUP: intVal = unit.group; isStringField = false; break;
            case Cond::TEACHER: strVal = unit.teacher; break;
            case Cond::SUBJECT: strVal = unit.subject; break;
            case Cond::AUDITORY:
                if (unit.auditoryIndex >= 0 && unit.auditoryIndex < static_cast<int>(db.getAuditories().size()))
                    strVal = db.getAuditories()[unit.auditoryIndex];
                break;
            case Cond::DATE: case Cond::DAY:
                if (unit.timeIndex >= 0 && unit.timeIndex < static_cast<int>(db.getTimes().size()))
                    strVal = db.getTimes()[unit.timeIndex].weekday;
                break;
            case Cond::TIME: case Cond::LESSON_NUM:
                if (unit.timeIndex >= 0 && unit.timeIndex < static_cast<int>(db.getTimes().size())) {
                    intVal = db.getTimes()[unit.timeIndex].lesson_num;
                    isStringField = false;
                }
                break;
        }
        
        bool condResult = false;
        if (isStringField) {
            switch (cond.relation) {
                case Cond::EQ: condResult = (strVal == cond.strValue); break;
                case Cond::NE: condResult = (strVal != cond.strValue); break;
                case Cond::CONTAINS: condResult = (strVal.find(cond.strValue) != std::string::npos); break;
                case Cond::LT: condResult = (strVal < cond.strValue); break;
                case Cond::GT: condResult = (strVal > cond.strValue); break;
                case Cond::LE: condResult = (strVal <= cond.strValue); break;
                case Cond::GE: condResult = (strVal >= cond.strValue); break;
            }
        } else {
            switch (cond.relation) {
                case Cond::EQ: condResult = (intVal == cond.intValue); break;
                case Cond::NE: condResult = (intVal != cond.intValue); break;
                case Cond::LT: condResult = (intVal < cond.intValue); break;
                case Cond::GT: condResult = (intVal > cond.intValue); break;
                case Cond::LE: condResult = (intVal <= cond.intValue); break;
                case Cond::GE: condResult = (intVal >= cond.intValue); break;
                case Cond::CONTAINS: condResult = false; break;
            }
        }
        if (!condResult) return false;
    }
    return true;
}

/* ======================== Вспомогательные функции парсера ======================== */
namespace {
    std::string toUpper(const std::string& s) {
        std::string res = s;
        for (auto& c : res)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return res;
    }

    bool parseField(const std::string& token, Cond::Field& out) {
        static const std::map<std::string, Cond::Field> fieldMap = {
            {"GROUP", Cond::GROUP},
            {"TEACHER", Cond::TEACHER},
            {"SUBJECT", Cond::SUBJECT},
            {"AUDITORY", Cond::AUDITORY},
            {"DATE", Cond::DATE},
            {"DAY", Cond::DAY},
            {"TIME", Cond::TIME},
            {"LESSON_NUM", Cond::LESSON_NUM}
        };
        auto it = fieldMap.find(toUpper(token));
        if (it != fieldMap.end()) {
            out = it->second;
            return true;
        }
        return false;
    }

    Cond::Relation parseRelation(const std::string& token) {
        if (token == "==") return Cond::EQ;
        if (token == "!=") return Cond::NE;
        if (token == "<")  return Cond::LT;
        if (token == ">")  return Cond::GT;
        if (token == "<=") return Cond::LE;
        if (token == ">=") return Cond::GE;
        if (toUpper(token) == "CONTAINS") return Cond::CONTAINS;
        return Cond::EQ;
    }

    bool isNumericField(Cond::Field field) {
        return field == Cond::GROUP || field == Cond::TIME || field == Cond::LESSON_NUM;
    }
}

/* ======================== parse ======================== */
Command parse(const std::string& query) {
    Command cmd;
    std::istringstream iss(query);
    std::string token;

    if (!(iss >> token)) { cmd.valid = false; cmd.errorMsg = "Пустая команда"; return cmd; }
    std::string command = toUpper(token);

    if (command == "ADD")           cmd.cmd = ADD;
    else if (command == "DELETE" || command == "REMOVE") cmd.cmd = REMOVE;
    else if (command == "SELECT")   cmd.cmd = SELECT;
    else if (command == "RESELECT") cmd.cmd = RESELECT;
    else if (command == "SAVE")     cmd.cmd = SAVE;
    else if (command == "LOAD")     cmd.cmd = LOAD;
    else if (command == "PRINT")    cmd.cmd = PRINT;
    else { cmd.valid = false; cmd.errorMsg = "Неизвестная команда: " + command; return cmd; }

    // SELECT / RESELECT / REMOVE
    if (cmd.cmd == SELECT || cmd.cmd == RESELECT || cmd.cmd == REMOVE) {
        while (iss >> token) {
            std::string upperToken = toUpper(token);
            if (upperToken == "AND") continue;

            Cond::Field field;
            if (!parseField(upperToken, field)) {
                cmd.valid = false; cmd.errorMsg = "Неизвестное поле: " + token; return cmd;
            }
            std::string relToken;
            if (!(iss >> relToken)) {
                cmd.valid = false; cmd.errorMsg = "Ожидается оператор после поля"; return cmd;
            }
            Cond::Relation rel = parseRelation(relToken);
            std::string value;
            if (!(iss >> value)) {
                cmd.valid = false; cmd.errorMsg = "Ожидается значение после оператора"; return cmd;
            }

            Cond cond;
            cond.field = field;
            cond.relation = rel;
            if (isNumericField(field)) {
                try {
                    cond.intValue = std::stoi(value);
                } catch (...) {
                    cmd.valid = false; cmd.errorMsg = "Некорректное число: " + value; return cmd;
                }
                cond.strValue = value;
            } else {
                cond.strValue = value;
                cond.intValue = 0;
            }

            if (cmd.cmd == REMOVE)
                cmd.removeConditions.push_back(cond);
            else
                cmd.conditions.push_back(cond);
        }
    }
    // PRINT
    else if (cmd.cmd == PRINT) {
        if (!(iss >> token)) {
            cmd.fullOutput = true;
            return cmd;
        }
        std::string upperToken = toUpper(token);
        if (upperToken == "SELECT") {
            return cmd;
        }
        Cond::Field field;
        if (!parseField(upperToken, field)) {
            cmd.valid = false;
            cmd.errorMsg = "Неизвестное поле: " + token + ". Допустимо: PRINT, PRINT SELECT, PRINT ПОЛЕ ЗНАЧЕНИЕ";
            return cmd;
        }
        std::string value;
        if (!(iss >> value)) {
            cmd.valid = false; cmd.errorMsg = "Ожидается значение после поля"; return cmd;
        }

        Cond cond;
        cond.field = field;
        cond.relation = Cond::EQ;
        if (isNumericField(field)) {
            try {
                cond.intValue = std::stoi(value);
            } catch (...) {
                cmd.valid = false; cmd.errorMsg = "Некорректное числовое значение: " + value;
                return cmd;
            }
            cond.strValue = value;
        } else {
            cond.strValue = value;
            cond.intValue = 0;
        }
        cmd.conditions.push_back(cond);
    }

    return cmd;
}

/* ======================== printMatrix ======================== */
void printMatrix(const std::vector<Indexes>& selection, const Database& db) {
    const auto& times = db.getTimes();
    const auto& auditories = db.getAuditories();
    if (times.empty() || auditories.empty()) {
        std::cout << "Расписание пусто.\n";
        return;
    }

    const int timeWidth = 12;
    const int cellWidth = 22;

    std::cout << std::setw(timeWidth) << " ";
    for (const auto& aud : auditories) {
        std::string shortAud = aud;
        if (shortAud.size() > static_cast<size_t>(cellWidth - 2))
            shortAud = shortAud.substr(0, cellWidth - 2);
        std::cout << " " << std::setw(cellWidth) << shortAud;
    }
    std::cout << "\n";

    std::set<Indexes> selectedSet(selection.begin(), selection.end());

    for (size_t t = 0; t < times.size(); ++t) {
        std::string dayShort = times[t].weekday.substr(0, 3);
        std::string timeLabel = dayShort + " " + std::to_string(times[t].lesson_num);
        std::cout << std::setw(timeWidth) << timeLabel;

        for (size_t a = 0; a < auditories.size(); ++a) {
            bool isSelected = selectedSet.count(Indexes(static_cast<int>(t), static_cast<int>(a))) > 0;
            if (isSelected) {
                const Schedule_unit* unit = db.getUnit(static_cast<int>(t), static_cast<int>(a));
                if (unit) {
                    std::stringstream ss;
                    ss << unit->subject << " " << unit->teacher << "(" << unit->group << ")";
                    std::string info = ss.str();
                    if (info.size() > static_cast<size_t>(cellWidth - 2))
                        info = info.substr(0, cellWidth - 2);
                    std::cout << " " << std::setw(cellWidth) << info;
                } else {
                    std::cout << " " << std::setw(cellWidth) << "---";
                }
            } else {
                std::cout << " " << std::setw(cellWidth) << "---";
            }
        }
        std::cout << "\n";
    }
}

/* ======================== printSchedule (запасная) ======================== */
void printSchedule(const std::vector<Schedule_unit>& schedule, 
                   const Database& db,
                   const std::vector<Cond::Field>& fields) {
    for (const auto& unit : schedule) {
        bool first = true;
        for (auto f : fields) {
            if (!first) std::cout << " ";
            first = false;
            switch (f) {
                case Cond::GROUP: std::cout << unit.group; break;
                case Cond::TEACHER: std::cout << unit.teacher; break;
                case Cond::SUBJECT: std::cout << unit.subject; break;
                case Cond::AUDITORY:
                    if (unit.auditoryIndex >= 0 && unit.auditoryIndex < static_cast<int>(db.getAuditories().size()))
                        std::cout << db.getAuditories()[unit.auditoryIndex];
                    break;
                case Cond::DATE: case Cond::DAY:
                    if (unit.timeIndex >= 0 && unit.timeIndex < static_cast<int>(db.getTimes().size()))
                        std::cout << db.getTimes()[unit.timeIndex].weekday;
                    break;
                case Cond::TIME: case Cond::LESSON_NUM:
                    if (unit.timeIndex >= 0 && unit.timeIndex < static_cast<int>(db.getTimes().size()))
                        std::cout << db.getTimes()[unit.timeIndex].lesson_num;
                    break;
            }
        }
        std::cout << "\n";
    }
}

/* ======================== sortSchedule ======================== */
void sortSchedule(std::vector<Schedule_unit>& schedule, 
                  Cond::Field field, 
                  bool ascending) {
    std::sort(schedule.begin(), schedule.end(), [field, ascending](const Schedule_unit& a, const Schedule_unit& b) {
        int cmp = 0;
        switch (field) {
            case Cond::GROUP:
                cmp = (a.group < b.group) ? -1 : (a.group > b.group) ? 1 : 0; break;
            case Cond::TEACHER:
                cmp = (a.teacher < b.teacher) ? -1 : (a.teacher > b.teacher) ? 1 : 0; break;
            case Cond::SUBJECT:
                cmp = (a.subject < b.subject) ? -1 : (a.subject > b.subject) ? 1 : 0; break;
            case Cond::AUDITORY:
                cmp = (a.auditoryIndex < b.auditoryIndex) ? -1 : (a.auditoryIndex > b.auditoryIndex) ? 1 : 0; break;
            case Cond::DATE: case Cond::DAY: case Cond::TIME: case Cond::LESSON_NUM:
                cmp = (a.timeIndex < b.timeIndex) ? -1 : (a.timeIndex > b.timeIndex) ? 1 : 0; break;
            default: cmp = 0;
        }
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}