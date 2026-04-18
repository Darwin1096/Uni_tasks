#include "shedule.h"
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <string>

void generateTestData(int numLessonsPerDay, int numAuditories, int numGroups, int numTeachers, int numSubjects, double occupancyRate){
    const std::string filename = "Data.txt";
    std::ofstream outFile(filename);

    if (!outFile.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << " для записи\n";
        return;
    }
    
    const std::vector<std::string> weekdays = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    
    // Генерируем список преподавателей
    std::vector<std::string> teachers;
    for (int i = 1; i <= numTeachers; ++i) {
        teachers.push_back("Teacher_" + std::to_string(i));
    }
    
    // Генерируем список предметов
    std::vector<std::string> subjects;
    for (int i = 1; i <= numSubjects; ++i) {
        subjects.push_back("Subject_" + std::to_string(i));
    }
    
    // Генерируем список аудиторий
    std::vector<std::string> auditories;
    for (int i = 1; i <= numAuditories; ++i) {
        auditories.push_back("Room_" + std::to_string(i));
    }
    
    // Генерируем список групп
    std::vector<int> groups;
    for (int i = 1; i <= numGroups; ++i) {
        groups.push_back(i);
    }
    
    // Генератор случайных чисел
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> occupancyDist(0.0, 1.0);
    std::uniform_int_distribution<> teacherDist(0, numTeachers - 1);
    std::uniform_int_distribution<> subjectDist(0, numSubjects - 1);
    std::uniform_int_distribution<> auditoryDist(0, numAuditories - 1);
    std::uniform_int_distribution<> groupDist(0, numGroups - 1);
    
    int totalLessons = 0;
    

    for (const auto& weekday : weekdays) {
        for (int lesson = 1; lesson <= numLessonsPerDay; ++lesson) {
            
            // Проверяем, должна ли быть пара в это время
            if (occupancyDist(gen) > occupancyRate) {
                continue; // Пропускаем, если пара не должна быть
            }
            
            // Генерируем случайные данные для пары
            std::string teacher = teachers[teacherDist(gen)];
            std::string subject = subjects[subjectDist(gen)];
            std::string auditory = auditories[auditoryDist(gen)];
            int group = groups[groupDist(gen)];
            outFile << weekday << " "
                    << teacher << " "
                    << subject << " "
                    << auditory << " "
                    << group << " "
                    << lesson << "\n";
            
            totalLessons++;
        }
    }
    
    outFile.close();
    
    std::cout << "Тестовые данные успешно сгенерированы в файл: " << filename << std::endl;
    std::cout << "Сгенерировано расписание с " << weekdays.size() << " днями по " 
              << numLessonsPerDay << " пар в день\n";
    std::cout << "Всего сгенерировано пар: " << totalLessons << " из " 
              << (weekdays.size() * numLessonsPerDay) << " возможных\n";
    std::cout << "Заполненность: " << (totalLessons * 100.0 / (weekdays.size() * numLessonsPerDay)) << "%\n";
    std::cout << "Формат: день_недели преподаватель предмет аудитория группа номер_пары\n";
}   