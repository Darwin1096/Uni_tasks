// result.h
#ifndef RESULT_H
#define RESULT_H

#include <cstdint>
#include <string>
#include <vector>
#include "shedule.h"   // для Schedule_unit

struct Result {
    bool success = true;
    std::string message;
    int affectedCount = 0;
    std::vector<Schedule_unit> records;
    int rows = 0;
    int cols = 0;
    std::vector<std::string> times;      // "Weekday lesson_num"
    std::vector<std::string> auditories; // названия аудиторий

    Result() = default;
    Result(bool ok, const std::string& msg) : success(ok), message(msg) {}

    // Сериализация в бинарный буфер (сетевой порядок байт)
    std::vector<uint8_t> serialize() const;

    // Десериализация из бинарных данных
    static Result deserialize(const std::vector<uint8_t>& data);
};

#endif