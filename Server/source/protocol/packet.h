#ifndef MC_PACKET_H
#define MC_PACKET_H

#include <vector>
#include <cstdint>
#include <stdexcept>

class c_packet
{
private:
    std::vector<uint8_t> data;
    int32_t read_index = 0;
public:
    c_packet() = default;
    explicit c_packet(const std::vector<uint8_t>& raw);

    uint8_t read_byte();
    void write_byte(uint8_t value);
    int32_t read_var_int();
    void write_var_int(int32_t value);
    int64_t read_var_long();
    void write_var_long(int64_t value);
    std::string read_string(size_t max_chars);
    void write_string(const std::string& str, size_t max_chars);
    int32_t read_int();
    void write_int(int32_t value);
    float read_float();
    void write_float(float value);
    double read_double();
    void write_double(double value);
    size_t get_size();
    std::vector<uint8_t>& get_raw();
    void finalize();
    void clear();
};


#endif