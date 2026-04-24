#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <list>
#include <map>
#include <cctype>
#include <memory> // Нужно для std::unique_ptr

// ============ TYPEDEFS ============
typedef int UserID;
typedef std::string Teacher_name;
typedef std::string Subject_name;
typedef std::string Auditory;

// ============ STRUCTS & ENUMS ============

// Структура условия должна быть объявлена ДО typedef SearchConditions
struct Cond { 
    enum Field { 
        GROUP, 
        TEACHER, 
        SUBJECT, 
        AUDITORY, 
        DATE, 
        TIME,
        DAY,
        LESSON_NUM
    };

    enum Relation {
        LT,      // <
        GT,      // >
        EQ,      // ==
        NE,      // !=
        LE,      // <=
        GE,      // >=
        CONTAINS // содержит
    };

    Field field;
    Relation relation;
    std::string strValue;  // для строк
    int intValue;          // для чисел
    
    Cond();
};

// Теперь тип SearchConditions известен, так как Cond уже определен
typedef std::list<Cond> SearchConditions;

typedef enum CommandType { 
    ADD, 
    DELETE, 
    SELECT, 
    RESELECT,
    SAVE, 
    LOAD, 
    PRINT,
    REMOVE
} CommandType;

// Время (день + пара)
struct DateTime {
    std::string weekday;  // День недели
    int lesson_num;       // Номер пары 
    
    DateTime();
    DateTime(const std::string& day, int num);
    
    std::string toString() const;
    bool operator==(const DateTime& other) const;
    bool operator<(const DateTime& other) const;
};

// Элемент расписания
struct Schedule_unit {
    std::string subject;
    std::string teacher;
    int group;
    int timeIndex;      // индекс в векторе times
    int auditoryIndex;  // индекс в векторе auditories
    
    Schedule_unit();
    Schedule_unit(const std::string& subj, const std::string& teach, 
                  int gr, int tIdx, int aIdx);
};

// Индексы в матрице
struct Indexes {
    int timeIndex;      
    int auditoryIndex;  
    
    Indexes(int t = -1, int a = -1);
    
    bool operator==(const Indexes& other) const;
    bool operator<(const Indexes& other) const;
};

// Расписание преподавателя (индексы ячеек)
struct TeacherSchedule {
    std::vector<Indexes> positions;    
    
    void addPosition(const Indexes& position);
    void clear();
    bool isEmpty() const;
    size_t size() const;
};

// Расписание предмета
struct SubjectSchedule {
    std::vector<Indexes> positions;  
    
    void addPosition(const Indexes& position);
    void clear();
    bool isEmpty() const;
    size_t size() const;
};

// Расписание группы
struct GroupSchedule {
    std::vector<Indexes> positions;
    
    void addPosition(const Indexes& position);
    void clear();
    bool isEmpty() const;
    size_t size() const;
};

// Выбранные элементы (результат поиска)
struct SelectedItems {
    std::vector<Indexes> selected_positions;
    
    void clear();
    bool isEmpty() const;
    size_t size() const;
};

// Сессия пользователя
struct Session {
    SelectedItems selectedItems;
    
    void clear();
};

// Менеджер сессий
class SessionManager {
private:
    std::map<UserID, Session> userSessions;
    
public:
    Session& getSession(UserID id);
    void clearSelection(UserID id);
    void addToSelection(UserID id, const Indexes& idx);
    void setSelection(UserID id, const std::vector<Indexes>& positions);
    const SelectedItems& getSelection(UserID id) const;
};

// Команда (распарсенный запрос)
struct Command {
    CommandType cmd;
    SearchConditions conditions;
    std::vector<Cond::Field> printFields;      // какие поля выводить
    // В C++14 вместо std::optional<Field> можно использовать указатель или bool flag
    bool hasSortField;
    Cond::Field sortFieldVal;
    
    std::string sortOrder; // "asc" / "desc"
    Schedule_unit newRecord;
    SearchConditions removeConditions;
    
    Command();
};

// ============ CLASS DATABASE ============

class Database {
private:
    std::vector<DateTime> times;
    std::vector<Auditory> auditories;
    
    // В C++14 используем unique_ptr. nullptr означает пустую ячейку.
    std::vector<std::vector<std::unique_ptr<Schedule_unit>>> data;
    
    std::unordered_map<Teacher_name, TeacherSchedule> teacherIndex;
    std::unordered_map<Subject_name, SubjectSchedule> subjectIndex;
    std::unordered_map<int, GroupSchedule> groupIndex;
    
    SessionManager sessionManager;
    
    int findTimeIndex(const DateTime& dt) const;
    int findAuditoryIndex(const Auditory& aud) const;
    bool hasCollision(int timeIndex, int auditoryIndex) const;
    
public:
    Database();
    // Запрещаем копирование, так как unique_ptr не копируется (только перемещается)
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    
    bool execute(UserID userId, const Command& cmd);
    bool insertRecord(const Schedule_unit& record);
    size_t removeRecords(const SearchConditions& conditions);
    std::vector<Schedule_unit> select(const SearchConditions& conditions) const;
    
    std::vector<Schedule_unit> getTeacherSchedule(const std::string& teacher) const;
    std::vector<Schedule_unit> getGroupSchedule(int group) const;
    std::vector<Schedule_unit> getSubjectSchedule(const std::string& subject) const;
    
    std::vector<Auditory> getFreeAuditories(const DateTime& dt) const;
    
   // bool checkCollision(const std::string& teacher, const DateTime& dt) const;
    //bool checkCollision(int group, const DateTime& dt) const;
    //bool checkCollision(const Auditory& auditory, const DateTime& dt) const;
    
    const std::vector<DateTime>& getTimes() const;
    const std::vector<Auditory>& getAuditories() const;
    SessionManager& getSessionManager();
    const SessionManager& getSessionManager() const;
};

// ============ FUNCTION DECLARATIONS ============

// Проверяет соответствие записи списку условий (все условия должны выполняться)
bool match(const Schedule_unit& unit, const Database& db, const SearchConditions& conditions);

Command parse(const std::string& query);

void generateTestData(int numLessonsPerDay,
                      int numAuditories, int numGroups,
                      int numTeachers, int numSubjects,
                      double occupancyRate);

void printSchedule(const std::vector<Schedule_unit>& schedule, 
                   const Database& db,
                   const std::vector<Cond::Field>& fields = {});

void sortSchedule(std::vector<Schedule_unit>& schedule, 
                  Cond::Field field, 
                  bool ascending = true);