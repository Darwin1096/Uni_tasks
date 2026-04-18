#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <list>
#include <map>
#include <iostream>
#include <sstream>

enum class CommandType { ADD, DELETE, SELECT, SAVE, LOAD, PRINT };
enum class Field { GROUP, TEACHER, SUBJECT, AUDITORY, DATE, TIME };
enum class Relation { LT, GT, EQ };

using UserID = int;
using Teacher_name = std::string;
using Subject_name = std::string;
using Auditory = std::string;

// Структура условия поиска
// В C++14 нет std::variant, поэтому храним значение как строку
struct Cond {
    Field field;
    Relation relation;
    std::string value_str; // Храним всё как строку

    // Хелперы для получения значения в нужном типе
    int getInt() const {
        return std::stoi(value_str);
    }
    
    double getDouble() const {
        return std::stod(value_str);
    }
    
    const std::string& getString() const {
        return value_str;
    }
};

using SearchConditions = std::list<Cond>;

struct Command {
    CommandType cmd;
    SearchConditions conditions;
};

Command parse(const std::string& query);

struct DateTime {
    std::string weekday;
    int lesson_num;

    std::string toString() const {
        return weekday + "_" + std::to_string(lesson_num);
    }

    bool operator==(const DateTime& other) const {
        return weekday == other.weekday && lesson_num == other.lesson_num;
    }

    bool operator<(const DateTime& other) const {
        if (weekday != other.weekday) return weekday < other.weekday;
        return lesson_num < other.lesson_num;
    }
};

struct Schedule_unit {
    std::string subject;
    std::string teacher;
    int group;
    DateTime dt;
    Auditory auditory;

    Schedule_unit() : group(0) {}
    
    Schedule_unit(const std::string& s, const std::string& t, int g, const DateTime& d, const Auditory& a)
        : subject(s), teacher(t), group(g), dt(d), auditory(a) {}
};

struct Indexes {
    int timeIndex;
    int auditoryIndex;

    Indexes(int t = -1, int a = -1) : timeIndex(t), auditoryIndex(a) {}

    bool operator==(const Indexes& other) const {
        return timeIndex == other.timeIndex && auditoryIndex == other.auditoryIndex;
    }
};

struct TeacherSchedule {
    std::vector<Indexes> positions;

    void addPosition(const Indexes& position) {
        positions.push_back(position);
    }
    void clear() {
        positions.clear();
    }
    bool isEmpty() const {
        return positions.empty();
    }
    size_t size() const {
        return positions.size();
    }
};

struct SubjectSchedule {
    std::vector<Indexes> positions;

    void addPosition(const Indexes& position) {
        positions.push_back(position);
    }
    void clear() {
        positions.clear();
    }
    bool isEmpty() const {
        return positions.empty();
    }
    size_t size() const {
        return positions.size();
    }
};

struct SelectedItems {
    std::vector<Indexes> selected_positions;

    void clear() { selected_positions.clear(); }
    bool isEmpty() const { return selected_positions.empty(); }
};

struct Session {
    SelectedItems selected_items;

    void addSelectedTimes(UserID userId, const std::vector<DateTime>& times);
    void addSelectedUnits(UserID userId, const std::vector<Schedule_unit>& units);
};

bool match(const Schedule_unit& unit, const SearchConditions& conditions);

class Database {
private:
    std::vector<DateTime> times;
    std::vector<Auditory> auditories;
    std::vector<std::vector<Schedule_unit>> data;

    class Sessions {
    private:
        std::map<UserID, Session> userSessions;
    public:
        Sessions() = default;

        Session& getSession(UserID id) {
            return userSessions[id];
        }

        bool hasSession(UserID id) const {
            return userSessions.find(id) != userSessions.end();
        }

        void removeSession(UserID id) {
            userSessions.erase(id);
        }
    };

    Sessions sessionsManager;

    std::unordered_map<Teacher_name, TeacherSchedule> teacherIndex;
    std::unordered_map<Subject_name, SubjectSchedule> subjectIndex;

public:
    Database();
    bool loadFromFile(const std::string& filename = "Data.txt");
    bool saveToFile(const std::string& filename = "Data.txt") const;

    bool execute(UserID userId, const Command& cmd);

    const std::vector<DateTime>& getTimes() const { return times; }
    const std::vector<Auditory>& getAuditories() const { return auditories; }
};

void generateTestData(int numLessonsPerDay = 4,
                      int numAuditories = 10, int numGroups = 10,
                      int numTeachers = 8, int numSubjects = 12,
                      double occupancyRate = 0.6);