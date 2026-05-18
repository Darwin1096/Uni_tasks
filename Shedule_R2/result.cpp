// result.cpp
#include "result.h"
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>

// Вспомогательные функции для упаковки/распаковки uint32_t в big-endian
static void writeUint32(std::vector<uint8_t>& buf, uint32_t val) {
    uint32_t net = htonl(val);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&net);
    buf.insert(buf.end(), p, p + sizeof(net));
}

static uint32_t readUint32(const uint8_t*& data, size_t& offset) {
    uint32_t net;
    std::memcpy(&net, data + offset, sizeof(net));
    offset += sizeof(net);
    return ntohl(net);
}

static void writeString(std::vector<uint8_t>& buf, const std::string& s) {
    writeUint32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

static std::string readString(const uint8_t* data, size_t& offset) {
    uint32_t len = readUint32(data, offset);
    std::string s(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    return s;
}

static void writeScheduleUnit(std::vector<uint8_t>& buf, const Schedule_unit& u) {
    writeString(buf, u.subject);
    writeString(buf, u.teacher);
    writeUint32(buf, static_cast<uint32_t>(u.group));
    writeUint32(buf, static_cast<uint32_t>(u.timeIndex));
    writeUint32(buf, static_cast<uint32_t>(u.auditoryIndex));
}

static Schedule_unit readScheduleUnit(const uint8_t* data, size_t& offset) {
    Schedule_unit u;
    u.subject = readString(data, offset);
    u.teacher = readString(data, offset);
    u.group = static_cast<int>(readUint32(data, offset));
    u.timeIndex = static_cast<int>(readUint32(data, offset));
    u.auditoryIndex = static_cast<int>(readUint32(data, offset));
    return u;
}

std::vector<uint8_t> Result::serialize() const {
    std::vector<uint8_t> buf;

    // success (1 байт)
    buf.push_back(success ? 1 : 0);

    // message
    writeString(buf, message);

    // affectedCount
    writeUint32(buf, static_cast<uint32_t>(affectedCount));

    // records
    writeUint32(buf, static_cast<uint32_t>(records.size()));
    for (const auto& rec : records) {
        writeScheduleUnit(buf, rec);
    }

    // rows, cols
    writeUint32(buf, static_cast<uint32_t>(rows));
    writeUint32(buf, static_cast<uint32_t>(cols));

    // times
    writeUint32(buf, static_cast<uint32_t>(times.size()));
    for (const auto& t : times) {
        writeString(buf, t);
    }

    // auditories
    writeUint32(buf, static_cast<uint32_t>(auditories.size()));
    for (const auto& a : auditories) {
        writeString(buf, a);
    }

    return buf;
}

Result Result::deserialize(const std::vector<uint8_t>& data) {
    Result res;
    const uint8_t* ptr = data.data();
    size_t offset = 0;

    // success
    if (offset >= data.size()) throw std::runtime_error("Invalid data");
    res.success = (ptr[offset++] != 0);

    // message
    res.message = readString(ptr, offset);

    // affectedCount
    res.affectedCount = static_cast<int>(readUint32(ptr, offset));

    // records
    uint32_t recCount = readUint32(ptr, offset);
    res.records.reserve(recCount);
    for (uint32_t i = 0; i < recCount; ++i) {
        res.records.push_back(readScheduleUnit(ptr, offset));
    }

    // rows, cols
    res.rows = static_cast<int>(readUint32(ptr, offset));
    res.cols = static_cast<int>(readUint32(ptr, offset));

    // times
    uint32_t timesCount = readUint32(ptr, offset);
    res.times.reserve(timesCount);
    for (uint32_t i = 0; i < timesCount; ++i) {
        res.times.push_back(readString(ptr, offset));
    }

    // auditories
    uint32_t audCount = readUint32(ptr, offset);
    res.auditories.reserve(audCount);
    for (uint32_t i = 0; i < audCount; ++i) {
        res.auditories.push_back(readString(ptr, offset));
    }

    return res;
}