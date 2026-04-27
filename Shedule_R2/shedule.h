#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <cctype>
#include <iostream>

// ============ TYPEDEFS ============
typedef int UserID;
typedef std::string Teacher_name;
typedef std::string Subject_name;
typedef std::string Auditory;

// ============ STRUCTS & ENUMS ============
struct Cond { 
    enum Field { 
        GROUP, TEACHER, SUBJECT, AUDITORY, DATE, TIME, DAY, LESSON_NUM
    };
    enum Relation {
        LT, GT, EQ, NE, LE, GE, CONTAINS
    };
    Field field;
    Relation relation;
    std::string strValue;
    int intValue;
    Cond();
};

typedef std::list<Cond> SearchConditions;

typedef enum CommandType { 
    ADD, DELETE, SELECT, RESELECT, SAVE, LOAD, PRINT, REMOVE
} CommandType;

struct DateTime {
    std::string weekday;
    int lesson_num;
    DateTime();
    DateTime(const std::string& day, int num);
    std::string toString() const;
    bool operator==(const DateTime& other) const;
    bool operator<(const DateTime& other) const;
};

struct Schedule_unit {
    std::string subject, teacher;
    int group, timeIndex, auditoryIndex;
    Schedule_unit();
    Schedule_unit(const std::string& subj, const std::string& teach, 
                  int gr, int tIdx, int aIdx);
};

struct Indexes {
    int timeIndex, auditoryIndex;
    Indexes(int t = -1, int a = -1);
    bool operator==(const Indexes& other) const;
    bool operator<(const Indexes& other) const;
};

struct TeacherSchedule {
    std::vector<Indexes> positions;
    void addPosition(const Indexes& p);
    void clear();
    bool isEmpty() const;
    size_t size() const;
};

struct SubjectSchedule {
    std::vector<Indexes> positions;
    void addPosition(const Indexes& p);
    void clear();
    bool isEmpty() const;
    size_t size() const;
};

struct GroupSchedule {
    std::vector<Indexes> positions;
    void addPosition(const Indexes& p);
    void clear();
    bool isEmpty() const;
    size_t size() const;
};

struct SelectedItems {
    std::vector<Indexes> selected_positions;
    void clear();
    bool isEmpty() const;
    size_t size() const;
};

struct Session {
    SelectedItems selectedItems;
    void clear();
};

class SessionManager {
    std::map<UserID, Session> userSessions;
public:
    Session& getSession(UserID id);
    void clearSelection(UserID id);
    void addToSelection(UserID id, const Indexes& idx);
    void setSelection(UserID id, const std::vector<Indexes>& positions);
    const SelectedItems& getSelection(UserID id) const;
};

struct Command {
    CommandType cmd;
    SearchConditions conditions;
    std::vector<Cond::Field> printFields;   // больше не используется, оставлен для совместимости
    bool hasSortField;
    Cond::Field sortFieldVal;
    std::string sortOrder;
    Schedule_unit newRecord;
    SearchConditions removeConditions;
    bool valid;
    std::string errorMsg;
    bool fullOutput;   // true, если PRINT вызван без аргументов (показать всё расписание)
    Command();
};

class Database {
    std::vector<DateTime> times;
    std::vector<Auditory> auditories;
    std::vector<std::vector<std::unique_ptr<Schedule_unit>>> data;
    std::unordered_map<Teacher_name, TeacherSchedule> teacherIndex;
    std::unordered_map<Subject_name, SubjectSchedule> subjectIndex;
    std::unordered_map<int, GroupSchedule> groupIndex;
    SessionManager sessionManager;

    int findTimeIndex(const DateTime& dt) const;
    int findAuditoryIndex(const Auditory& aud) const;
    bool hasCollision(int ti, int ai) const;

public:
    Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool loadFromFile(const std::string& fname);
    bool saveToFile(const std::string& fname) const;
    bool execute(UserID uid, const Command& cmd);
    bool insertRecord(const Schedule_unit& rec);
    size_t removeRecords(const SearchConditions& conds);
    std::vector<Schedule_unit> select(const SearchConditions& conds) const;

    std::vector<Schedule_unit> getTeacherSchedule(const std::string& t) const;
    std::vector<Schedule_unit> getGroupSchedule(int g) const;
    std::vector<Schedule_unit> getSubjectSchedule(const std::string& s) const;
    std::vector<Auditory> getFreeAuditories(const DateTime& dt) const;

    const Schedule_unit* getUnit(int ti, int ai) const;
    const std::vector<DateTime>& getTimes() const;
    const std::vector<Auditory>& getAuditories() const;
    SessionManager& getSessionManager();
    const SessionManager& getSessionManager() const;
};

// Декларации функций
bool match(const Schedule_unit& unit, const Database& db, const SearchConditions& conds);
Command parse(const std::string& query);
void generateTestData(int lessonsPerDay = 4, int auditories = 10, int groups = 10,
                      int teachers = 8, int subjects = 12, double occupancyRate = 0.6);
void printSchedule(const std::vector<Schedule_unit>& schedule, const Database& db,
                   const std::vector<Cond::Field>& fields);
void printMatrix(const std::vector<Indexes>& selection, const Database& db);
void sortSchedule(std::vector<Schedule_unit>& schedule, Cond::Field field, bool ascending = true);

#endif