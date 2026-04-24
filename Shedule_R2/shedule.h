#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <list>
#include <map>

typedef enum CommandType { ADD, DELETE, SELECT, SAVE, LOAD, PRINT};
typedef enum Field { GROUP, TEACHER, SUBJECT, AUDITORY, DATE, TIME };
typedef enum Relation {LT, GT, EQ};
typedef std::list<Cond> SearchConditions;
typedef int UserID;
typedef std::string Teacher_name;
typedef std::string Subject_name;
typedef std::string Auditory;


//Надо дописать, пока хз как
struct Cond{ 
    Field field;
    Relation relation;
    union {
        int i;
        std::string s;
        double d;
    } value;
};

struct Command {
    CommandType cmd;
    SearchConditions conditions;
};
Command parse(const std::string& query); //Собственно парсер 


//Время
struct DateTime {
    std::string weekday; //День недели
    int lesson_num; //Номер пары 
    std::string toString() const;
    bool operator==(const DateTime& other) const;
    bool operator<(const DateTime& other) const;
};

struct Schedule_unit{
    std::string subject;
    std::string teacher;
    int group = 0;
    DateTime& dt;  //Можно вместо указателей сделать ссылки
    Auditory& auditory;  //Можно вместо указателей сделать ссылки
};

//Структура для работы с матрицей
struct Indexes{
    int timeIndex;      
    int auditoryIndex;  
    
    Indexes(int t = -1, int a = -1) : timeIndex(t), auditoryIndex(a) {}
    
    bool operator==(const Indexes& other) const {
        return timeIndex == other.timeIndex && auditoryIndex == other.auditoryIndex;
    }
};


//Структура для хранения расписания преподавателя
struct TeacherSchedule{
    std::vector<Indexes> positions;    
    void addPosition(const Indexes& position);
    void clear();
    bool isEmpty() const;
    size_t size() const;
};


//Структура для хранения расписания предмета
struct SubjectSchedule{
    std::vector<Indexes> positions;  
    void addPosition(const Indexes& position);
    void clear();
    bool isEmpty() const;
    size_t size() const;
};


//Структура для хранения выбранных элементов
struct SelectedItems{
    std::vector<Indexes> selected_positions;
    void clear();
    bool isEmpty() const;
};


//Сессия одного пользователя
struct Session{
    SelectedItems SelectedItems;
    //Надо подумать об аргументах этих функций и вообще о том, что хочется делать 
    void addSelectedTimes(int userId, const std::vector<DateTime>& times); 
    void addSelectedUnits(int userId, const std::vector<Schedule_unit>& units);
};

//Функция match 
bool match(const Schedule_unit& unit, const SearchConditions& conditions);



class Database{
private:
    std::vector<DateTime> times;     //При получении данных их надо будет отсортировать 
    std::vector<Auditory> auditories;    //встроенной сортировкой 
    std::vector<std::vector<Schedule_unit>> data;

    //Класс Sessions должен быть элементом Database, это отображение из айди в сессию
    class Sessions{
        private:
        std::map<UserID, Session> userSessions; //Контейнер Id - Selected
        public:
        Sessions();
    };
    
    //Хэш-множества
    std::unordered_map<Teacher_name, TeacherSchedule> teacherIndex; 
    std::unordered_map<Subject_name, SubjectSchedule> subjectIndex; 
public:
    Database();
    bool loadFromFile(); // Data.txt
    bool saveToFile() const; //Есть известный файл куда записывать

    //Должна быть функция match вне каких-то классов, которая проверяет,
    //что Shedule_unit подходит под распаршенные условия и в execute будут циклы по матрице,
    // где каждый Shedule_unit будет проверяться
    bool execute(int userId, const Command& cmd);
};

//Название файла куда будет записываться, уже известно (РЕАЛИЗОВАНО)
void generateTestData(int numLessonsPerDay = 4,
                      int numAuditories = 10, int numGroups = 10,
                      int numTeachers = 8, int numSubjects = 12,
                      double occupancyRate = 0.6);

