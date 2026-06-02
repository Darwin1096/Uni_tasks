#include "shedule.h"
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <string>
#include <set>

void generateTestData(int numLessonsPerDay,
                      int numAuditories,
                      int numGroups,
                      int numTeachers,
                      int numSubjects,
                      double occupancyRate) {
    const std::string filename = "Data.txt";
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << " для записи\n";
        return;
    }
    
    const std::vector<std::string> weekdays = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    
    std::vector<std::string> teachers;
    for (int i = 1; i <= numTeachers; ++i)
        teachers.push_back("T_" + std::to_string(i));
    
    std::vector<std::string> subjects;
    for (int i = 1; i <= numSubjects; ++i)
        subjects.push_back("S_" + std::to_string(i));
    
    std::vector<std::string> auditories;
    for (int i = 1; i <= numAuditories; ++i)
        auditories.push_back("R_" + std::to_string(i));
    
    std::vector<int> groups;
    for (int i = 1; i <= numGroups; ++i)
        groups.push_back(i);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> occupancyDist(0.0, 1.0);
    std::uniform_int_distribution<> teacherDist(0, numTeachers - 1);
    std::uniform_int_distribution<> subjectDist(0, numSubjects - 1);
    std::uniform_int_distribution<> auditoryDist(0, numAuditories - 1);
    std::uniform_int_distribution<> groupDist(0, numGroups - 1);

    // Множество для отслеживания уже занятых слотов (день, урок, аудитория)
    std::set<std::tuple<std::string, int, std::string>> usedSlots;

    int totalLessons = 0;
    int maxPossible = weekdays.size() * numLessonsPerDay * numAuditories;

    for (const auto& weekday : weekdays) {
        for (int lesson = 1; lesson <= numLessonsPerDay; ++lesson) {
            for (const auto& auditory : auditories) {
                // Пропускаем с вероятностью (1 - occupancyRate)
                if (occupancyDist(gen) > occupancyRate)
                    continue;

                // Если слот уже занят (на всякий случай), пропускаем
                if (usedSlots.count({weekday, lesson, auditory}) > 0)
                    continue;

                std::string teacher = teachers[teacherDist(gen)];
                std::string subject = subjects[subjectDist(gen)];
                int group = groups[groupDist(gen)];

                outFile << weekday << " "
                        << lesson << " "
                        << auditory << " "
                        << subject << " "
                        << teacher << " "
                        << group << "\n";

                usedSlots.insert({weekday, lesson, auditory});
                totalLessons++;
            }
        }
    }
    
    outFile.close();
    std::cout << "Тестовые данные успешно сгенерированы в файл: " << filename << std::endl;
    std::cout << "Всего сгенерировано уникальных записей: " << totalLessons
              << " из " << maxPossible << " возможных слотов\n";
    std::cout << "Заполненность: " 
              << (totalLessons * 100.0 / maxPossible) << "%\n";
    std::cout << "Формат: день_недели урок аудитория предмет преподаватель группа\n";
}