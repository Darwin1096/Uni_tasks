// schedule.cpp
#include "shedule.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <set>
#include <tuple>
#include <cctype>

// ============ Cond ============
Cond::Cond() : field(GROUP), relation(EQ), strValue(""), intValue(0) {}

// ============ DateTime ============
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

// ============ Schedule_unit ============
Schedule_unit::Schedule_unit() : subject(""), teacher(""), group(0), timeIndex(-1), auditoryIndex(-1) {}

Schedule_unit::Schedule_unit(const std::string& subj, const std::string& teach, 
                              int gr, int tIdx, int aIdx)
    : subject(subj), teacher(teach), group(gr), timeIndex(tIdx), auditoryIndex(aIdx) {}

// ============ Indexes ============
Indexes::Indexes(int t, int a) : timeIndex(t), auditoryIndex(a) {}

bool Indexes::operator==(const Indexes& other) const {
    return timeIndex == other.timeIndex && auditoryIndex == other.auditoryIndex;
}

bool Indexes::operator<(const Indexes& other) const {
    if (timeIndex != other.timeIndex) return timeIndex < other.timeIndex;
    return auditoryIndex < other.auditoryIndex;
}

// ============ TeacherSchedule / SubjectSchedule / GroupSchedule ============
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

// ============ SelectedItems ============
void SelectedItems::clear() { selected_positions.clear(); }
bool SelectedItems::isEmpty() const { return selected_positions.empty(); }
size_t SelectedItems::size() const { return selected_positions.size(); }

// ============ Session ============
void Session::clear() { selectedItems.clear(); }

// ============ SessionManager ============
Session& SessionManager::getSession(UserID id) {
    return userSessions[id];
}

void SessionManager::clearSelection(UserID id) {
    auto it = userSessions.find(id);
    if (it != userSessions.end()) {
        it->second.clear();
    }
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

// ============ Command ============
Command::Command() : cmd(ADD), hasSortField(false), sortFieldVal(Cond::GROUP), sortOrder("asc") {}

// ============ Database: private methods ============
int Database::findTimeIndex(const DateTime& dt) const {
    for (size_t i = 0; i < times.size(); ++i) {
        if (times[i] == dt) return static_cast<int>(i);
    }
    return -1;
}

int Database::findAuditoryIndex(const Auditory& aud) const {
    for (size_t i = 0; i < auditories.size(); ++i) {
        if (auditories[i] == aud) return static_cast<int>(i);
    }
    return -1;
}

bool Database::hasCollision(int timeIndex, int auditoryIndex) const {
    if (timeIndex < 0 || timeIndex >= static_cast<int>(data.size())) return false;
    if (auditoryIndex < 0 || auditoryIndex >= static_cast<int>(data[timeIndex].size())) return false;
    return data[timeIndex][auditoryIndex] != nullptr;
}

// ============ Database: public methods ============
Database::Database() {
    // Vectors initialized empty; can be populated via loadFromFile or manually
}

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
        
        if (!(iss >> day >> lessonNum >> auditory >> subject >> teacher >> group)) {
            continue;
        }
        
        DateTime dt(day, lessonNum);
        timeSet.insert(dt);
        audSet.insert(auditory);
        records.emplace_back(day, lessonNum, auditory, subject, teacher, group);
    }
    file.close();
    
    // Build vectors from sets (sorted order)
    times.assign(timeSet.begin(), timeSet.end());
    auditories.assign(audSet.begin(), audSet.end());
    
    // Initialize sparse matrix
    data.resize(times.size());
    for (auto& row : data) {
        row.resize(auditories.size());
    }
    
    // Insert records
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
    switch (cmd.cmd) {
        case ADD:
            return insertRecord(cmd.newRecord);
            
        case DELETE:
        case REMOVE:
            removeRecords(cmd.removeConditions);
            return true;
            
        case SELECT: {
            auto results = select(cmd.conditions);
            std::vector<Indexes> positions;
            for (const auto& unit : results) {
                positions.emplace_back(unit.timeIndex, unit.auditoryIndex);
            }
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
                    
                    if (match(*data[idx.timeIndex][idx.auditoryIndex], *this, cmd.conditions)) {
                        filtered.push_back(idx);
                    }
                }
            }
            sessionManager.setSelection(userId, filtered);
            return true;
        }
            
        case PRINT: {
            const auto& selection = sessionManager.getSelection(userId);
            std::vector<Schedule_unit> toPrint;
            
            for (const auto& idx : selection.selected_positions) {
                if (idx.timeIndex >= 0 && idx.timeIndex < static_cast<int>(data.size()) &&
                    idx.auditoryIndex >= 0 && idx.auditoryIndex < static_cast<int>(data[idx.timeIndex].size()) &&
                    data[idx.timeIndex][idx.auditoryIndex] != nullptr) {
                    toPrint.push_back(*data[idx.timeIndex][idx.auditoryIndex]);
                }
            }
            
            if (cmd.hasSortField) {
                sortSchedule(toPrint, cmd.sortFieldVal, cmd.sortOrder == "asc");
            }
            printSchedule(toPrint, *this, cmd.printFields);
            return true;
        }
            
        case SAVE:
        case LOAD:
            // Filename handling would require additional Command field; placeholder
            return false;
            
        default:
            return false;
    }
}

bool Database::insertRecord(const Schedule_unit& record) {
    // Check auditory collision
    if (hasCollision(record.timeIndex, record.auditoryIndex)) {
        return false;
    }
    
    // Check teacher collision at same time
    auto tIt = teacherIndex.find(record.teacher);
    if (tIt != teacherIndex.end()) {
        for (const auto& pos : tIt->second.positions) {
            if (pos.timeIndex == record.timeIndex) {
                return false;
            }
        }
    }
    
    // Check group collision at same time
    auto gIt = groupIndex.find(record.group);
    if (gIt != groupIndex.end()) {
        for (const auto& pos : gIt->second.positions) {
            if (pos.timeIndex == record.timeIndex) {
                return false;
            }
        }
    }
    
    // Insert record
    auto unit = std::make_unique<Schedule_unit>(record);
    data[record.timeIndex][record.auditoryIndex] = std::move(unit);
    
    // Update indices
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
                
                // Remove from teacher index
                auto tIt = teacherIndex.find(unit.teacher);
                if (tIt != teacherIndex.end()) {
                    auto& pos = tIt->second.positions;
                    pos.erase(std::remove(pos.begin(), pos.end(), Indexes(static_cast<int>(t), static_cast<int>(a))), pos.end());
                }
                
                // Remove from subject index
                auto sIt = subjectIndex.find(unit.subject);
                if (sIt != subjectIndex.end()) {
                    auto& pos = sIt->second.positions;
                    pos.erase(std::remove(pos.begin(), pos.end(), Indexes(static_cast<int>(t), static_cast<int>(a))), pos.end());
                }
                
                // Remove from group index
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
            if (data[t][a] != nullptr && match(*data[t][a], *this, conditions)) {
                results.push_back(*data[t][a]);
            }
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
                data[idx.timeIndex][idx.auditoryIndex] != nullptr) {
                result.push_back(*data[idx.timeIndex][idx.auditoryIndex]);
            }
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
                data[idx.timeIndex][idx.auditoryIndex] != nullptr) {
                result.push_back(*data[idx.timeIndex][idx.auditoryIndex]);
            }
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
                data[idx.timeIndex][idx.auditoryIndex] != nullptr) {
                result.push_back(*data[idx.timeIndex][idx.auditoryIndex]);
            }
        }
    }
    return result;
}

std::vector<Auditory> Database::getFreeAuditories(const DateTime& dt) const {
    std::vector<Auditory> free;
    int tIdx = findTimeIndex(dt);
    if (tIdx < 0) return auditories;
    
    for (size_t a = 0; a < auditories.size(); ++a) {
        if (tIdx < static_cast<int>(data.size()) && a < data[tIdx].size() && data[tIdx][a] == nullptr) {
            free.push_back(auditories[a]);
        }
    }
    return free;
}

bool Database::checkCollision(const std::string& teacher, const DateTime& dt) const {
    int tIdx = findTimeIndex(dt);
    if (tIdx < 0) return false;
    
    auto it = teacherIndex.find(teacher);
    if (it != teacherIndex.end()) {
        for (const auto& pos : it->second.positions) {
            if (pos.timeIndex == tIdx) return true;
        }
    }
    return false;
}

bool Database::checkCollision(int group, const DateTime& dt) const {
    int tIdx = findTimeIndex(dt);
    if (tIdx < 0) return false;
    
    auto it = groupIndex.find(group);
    if (it != groupIndex.end()) {
        for (const auto& pos : it->second.positions) {
            if (pos.timeIndex == tIdx) return true;
        }
    }
    return false;
}

bool Database::checkCollision(const Auditory& auditory, const DateTime& dt) const {
    int tIdx = findTimeIndex(dt);
    int aIdx = findAuditoryIndex(auditory);
    if (tIdx < 0 || aIdx < 0) return false;
    return hasCollision(tIdx, aIdx);
}

const std::vector<DateTime>& Database::getTimes() const { return times; }
const std::vector<Auditory>& Database::getAuditories() const { return auditories; }
SessionManager& Database::getSessionManager() { return sessionManager; }
const SessionManager& Database::getSessionManager() const { return sessionManager; }

// ============ match function ============
bool match(const Schedule_unit& unit, const Database& db, const SearchConditions& conditions) {
    for (const auto& cond : conditions) {
        std::string strVal;
        int intVal = 0;
        bool isStringField = true;
        
        switch (cond.field) {
            case Cond::GROUP:
                intVal = unit.group;
                isStringField = false;
                break;
            case Cond::TEACHER:
                strVal = unit.teacher;
                break;
            case Cond::SUBJECT:
                strVal = unit.subject;
                break;
            case Cond::AUDITORY:
                if (unit.auditoryIndex >= 0 && unit.auditoryIndex < static_cast<int>(db.getAuditories().size())) {
                    strVal = db.getAuditories()[unit.auditoryIndex];
                }
                break;
            case Cond::DATE:
            case Cond::DAY:
                if (unit.timeIndex >= 0 && unit.timeIndex < static_cast<int>(db.getTimes().size())) {
                    strVal = db.getTimes()[unit.timeIndex].weekday;
                }
                break;
            case Cond::TIME:
            case Cond::LESSON_NUM:
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
                case Cond::CONTAINS: condResult = false; break; // N/A for numeric
            }
        }
        
        if (!condResult) return false;
    }
    return true;
}

// ============ parse function ============
Command parse(const std::string& query) {
    Command cmd;
    std::istringstream iss(query);
    std::string command;
    iss >> command;
    
    if (command == "ADD") cmd.cmd = ADD;
    else if (command == "DELETE" || command == "REMOVE") cmd.cmd = REMOVE;
    else if (command == "SELECT") cmd.cmd = SELECT;
    else if (command == "RESELECT") cmd.cmd = RESELECT;
    else if (command == "SAVE") cmd.cmd = SAVE;
    else if (command == "LOAD") cmd.cmd = LOAD;
    else if (command == "PRINT") cmd.cmd = PRINT;
    
    // Simplified: full parsing would require detailed query language specification
    return cmd;
}

// ============ generateTestData function ============
void generateTestData(int numLessonsPerDay, int numAuditories, int numGroups,
                      int numTeachers, int numSubjects, double occupancyRate) {
    std::ofstream file("test_data.txt");
    if (!file.is_open()) return;
    
    std::vector<std::string> days = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    std::vector<std::string> teachers, subjects;
    
    for (int i = 0; i < numTeachers; ++i) teachers.push_back("Teacher" + std::to_string(i + 1));
    for (int i = 0; i < numSubjects; ++i) subjects.push_back("Subject" + std::to_string(i + 1));
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> audDist(1, numAuditories);
    std::uniform_int_distribution<> teachDist(0, numTeachers - 1);
    std::uniform_int_distribution<> subjDist(0, numSubjects - 1);
    std::uniform_int_distribution<> groupDist(101, 100 + numGroups);
    std::uniform_real_distribution<> probDist(0.0, 1.0);
    
    for (const auto& day : days) {
        for (int lesson = 1; lesson <= numLessonsPerDay; ++lesson) {
            for (int aud = 1; aud <= numAuditories; ++aud) {
                if (probDist(gen) < occupancyRate) {
                    std::string auditory = "Aud_" + std::to_string(aud);
                    std::string subject = subjects[subjDist(gen)];
                    std::string teacher = teachers[teachDist(gen)];
                    int group = groupDist(gen);
                    
                    file << day << " " << lesson << " " << auditory << " "
                         << subject << " " << teacher << " " << group << "\n";
                }
            }
        }
    }
    file.close();
}

// ============ printSchedule function ============
void printSchedule(const std::vector<Schedule_unit>& schedule, 
                   const Database& db,
                   const std::vector<Cond::Field>& fields) {
    for (const auto& unit : schedule) {
        bool first = true;
        
        auto printField = [&](Cond::Field f) {
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
                case Cond::DATE:
                case Cond::DAY:
                    if (unit.timeIndex >= 0 && unit.timeIndex < static_cast<int>(db.getTimes().size()))
                        std::cout << db.getTimes()[unit.timeIndex].weekday;
                    break;
                case Cond::TIME:
                case Cond::LESSON_NUM:
                    if (unit.timeIndex >= 0 && unit.timeIndex < static_cast<int>(db.getTimes().size()))
                        std::cout << db.getTimes()[unit.timeIndex].lesson_num;
                    break;
            }
        };
        
        if (fields.empty()) {
            // Print all default fields
            if (unit.timeIndex >= 0 && unit.timeIndex < static_cast<int>(db.getTimes().size())) {
                std::cout << db.getTimes()[unit.timeIndex].weekday << " "
                          << db.getTimes()[unit.timeIndex].lesson_num << " ";
            }
            if (unit.auditoryIndex >= 0 && unit.auditoryIndex < static_cast<int>(db.getAuditories().size())) {
                std::cout << db.getAuditories()[unit.auditoryIndex] << " ";
            }
            std::cout << unit.subject << " " << unit.teacher << " " << unit.group;
        } else {
            for (auto f : fields) printField(f);
        }
        std::cout << "\n";
    }
}

// ============ sortSchedule function ============
void sortSchedule(std::vector<Schedule_unit>& schedule, 
                  Cond::Field field, 
                  bool ascending) {
    std::sort(schedule.begin(), schedule.end(), [field, ascending](const Schedule_unit& a, const Schedule_unit& b) {
        int cmp = 0;
        
        switch (field) {
            case Cond::GROUP:
                cmp = (a.group < b.group) ? -1 : (a.group > b.group) ? 1 : 0;
                break;
            case Cond::TEACHER:
                cmp = (a.teacher < b.teacher) ? -1 : (a.teacher > b.teacher) ? 1 : 0;
                break;
            case Cond::SUBJECT:
                cmp = (a.subject < b.subject) ? -1 : (a.subject > b.subject) ? 1 : 0;
                break;
            case Cond::AUDITORY:
                cmp = (a.auditoryIndex < b.auditoryIndex) ? -1 : (a.auditoryIndex > b.auditoryIndex) ? 1 : 0;
                break;
            case Cond::DATE:
            case Cond::DAY:
            case Cond::TIME:
            case Cond::LESSON_NUM:
                cmp = (a.timeIndex < b.timeIndex) ? -1 : (a.timeIndex > b.timeIndex) ? 1 : 0;
                break;
            default:
                cmp = 0;
                break;
        }
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}