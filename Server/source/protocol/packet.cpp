#include "packet.h"
#include <iostream>

c_packet::c_packet(const std::vector<uint8_t>& raw) : read_index(0) {
    if (raw.empty()) {
        throw std::runtime_error("Cannot create packet from empty data");
    }

    // Find where the length VarInt ends
    size_t length_varint_size = 0;
    for (size_t i = 0; i < raw.size() && i < 5; i++) {
        length_varint_size = i + 1;
        if ((raw[i] & 0x80) == 0) {
            break;
        }
    }

    if (length_varint_size == 0 || length_varint_size >= raw.size()) {
        throw std::runtime_error("Invalid packet: malformed length prefix");
    }

    // Copy only the packet data (excluding the length prefix)
    data.assign(raw.begin() + length_varint_size, raw.end());
    read_index = 0;  // Start reading from the actual packet data
}


uint8_t c_packet::read_byte()
{
    if (read_index >= data.size()) 
        throw std::runtime_error("Read past end of packet");

    int8_t value = this->data.at(this->read_index);
    this->read_index++;
    return value;
}

int32_t c_packet::read_var_int() 
{
    int num_read = 0;
    int32_t result = 0;
    uint8_t read;
    do 
    {
        read = this->read_byte();
        int32_t value = (read & 0b01111111);
        result |= (value << (7 * num_read));

        num_read++;
        if (num_read > 5)
            throw std::runtime_error("VarInt is too big");
    }
    while ((read & 0b10000000) != 0);

    return result;
}

int64_t c_packet::read_var_long() 
{
    int num_read = 0;
    int64_t result = 0;
    uint8_t read;
    do 
    {
        read = this->read_byte();
        int64_t value = (read & 0b01111111);
        result |= (value << (7 * num_read));

        num_read++;
        if (num_read > 10)
        {
            throw std::runtime_error("VarLong is too big");
        }
    }
    while ((read & 0b10000000) != 0);

    return result;
}

void c_packet::write_byte(uint8_t value)
{
    this->data.push_back(value);
}

void c_packet::write_var_int(int32_t value) 
{
    do 
    {
        uint8_t temp = value & 0b01111111;
        value >>= 7;
        if (value != 0) 
        {
            temp |= 0b10000000;
        }
        this->write_byte(temp);
    }
    while (value != 0);
}

void c_packet::write_var_long(int64_t value) 
{
    do 
    {
        uint8_t temp = value & 0b01111111;
        value >>= 7;
        if (value != 0) 
        {
            temp |= 0b10000000;
        }
        this->write_byte(temp);
    }
    while (value != 0);
}

std::string c_packet::read_string(size_t max_chars) {
    int32_t byte_length = this->read_var_int();

    if (byte_length < 0 || byte_length > static_cast<int32_t>(max_chars * 4)) {
        throw std::runtime_error("String byte length exceeds allowed maximum");
    }

    if (this->read_index + byte_length > this->get_size()) {
        throw std::runtime_error("Not enough bytes left in packet to read string");
    }

    std::string result(reinterpret_cast<const char*>(&this->get_raw()[this->read_index]), byte_length);
    this->read_index += byte_length;

    // Optional: Enforce max character count (UTF-8 decoding)
    size_t char_count = 0;
    for (size_t i = 0; i < result.size(); ++i) {
        unsigned char c = result[i];
        if ((c & 0xC0) != 0x80) char_count++; // count only UTF-8 lead bytes
    }
    if (char_count > max_chars) {
        throw std::runtime_error("String exceeds maximum allowed characters");
    }

    return result;
}

void c_packet::write_string(const std::string& str, size_t max_chars) {
    // Check UTF-8 character count (if needed)
    size_t char_count = 0;
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c = str[i];
        if ((c & 0xC0) != 0x80) char_count++;
    }

    if (char_count > max_chars)
        throw std::runtime_error("String exceeds character limit");

    this->write_var_int(static_cast<int32_t>(str.size())); // UTF-8 byte length

    for (char c : str)
        this->write_byte(static_cast<uint8_t>(c));
}

int32_t c_packet::read_int()
{
    if (this->read_index + 4 > this->data.size()) {
        throw std::runtime_error("Not enough bytes to read int");
    }

    int32_t result = 0;
    result |= (this->data[read_index++] << 24);
    result |= (this->data[read_index++] << 16);
    result |= (this->data[read_index++] << 8);
    result |= (this->data[read_index++]);

    return result;
}

void c_packet::write_int(int32_t value)
{
    this->data.push_back((value >> 24) & 0xFF);
    this->data.push_back((value >> 16) & 0xFF);
    this->data.push_back((value >> 8) & 0xFF);
    this->data.push_back(value & 0xFF);
}

float c_packet::read_float()
{
    if (this->read_index + 4 > this->data.size()) {
        throw std::runtime_error("Not enough bytes to read float");
    }

    uint32_t raw = 0;
    raw |= (static_cast<uint32_t>(this->data[this->read_index++]) << 24);
    raw |= (static_cast<uint32_t>(this->data[this->read_index++]) << 16);
    raw |= (static_cast<uint32_t>(this->data[this->read_index++]) << 8);
    raw |= (static_cast<uint32_t>(this->data[this->read_index++]));

    float result;
    std::memcpy(&result, &raw, sizeof(float));
    return result;
}

void c_packet::write_float(float value)
{
    uint32_t raw;
    std::memcpy(&raw, &value, sizeof(float));

    this->data.push_back((raw >> 24) & 0xFF);
    this->data.push_back((raw >> 16) & 0xFF);
    this->data.push_back((raw >> 8) & 0xFF);
    this->data.push_back(raw & 0xFF);
}

double c_packet::read_double()
{
    if (this->read_index + 8 > this->data.size()) {
        throw std::runtime_error("Not enough bytes to read double");
    }

    uint64_t raw = 0;
    raw |= (static_cast<uint64_t>(this->data[this->read_index++]) << 56);
    raw |= (static_cast<uint64_t>(this->data[this->read_index++]) << 48);
    raw |= (static_cast<uint64_t>(this->data[this->read_index++]) << 40);
    raw |= (static_cast<uint64_t>(this->data[this->read_index++]) << 32);
    raw |= (static_cast<uint64_t>(this->data[this->read_index++]) << 24);
    raw |= (static_cast<uint64_t>(this->data[this->read_index++]) << 16);
    raw |= (static_cast<uint64_t>(this->data[this->read_index++]) << 8);
    raw |= (static_cast<uint64_t>(this->data[this->read_index++]));

    double result;
    std::memcpy(&result, &raw, sizeof(double));
    return result;
}

void c_packet::write_double(double value)
{
    uint64_t raw;
    std::memcpy(&raw, &value, sizeof(double));

    this->data.push_back((raw >> 56) & 0xFF);
    this->data.push_back((raw >> 48) & 0xFF);
    this->data.push_back((raw >> 40) & 0xFF);
    this->data.push_back((raw >> 32) & 0xFF);
    this->data.push_back((raw >> 24) & 0xFF);
    this->data.push_back((raw >> 16) & 0xFF);
    this->data.push_back((raw >> 8) & 0xFF);
    this->data.push_back(raw & 0xFF);
}

void c_packet::write_long(int64_t value)
{
    for (int i = 7; i >= 0; --i)
    {
        this->data.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
    }
}

int64_t c_packet::read_long()
{
    if (this->read_index + 8 > this->data.size())
        throw std::runtime_error("Not enough data to read long");

    int64_t result = 0;
    for (int i = 0; i < 8; ++i)
    {
        result = (result << 8) | this->data[this->read_index++];
    }
    return result;
}

void c_packet::write_nbt_string(const std::string& str) {
    if (str.length() > 0xFFFF)
        throw std::runtime_error("NBT string too long");

    this->write_byte((str.length() >> 8) & 0xFF);  // High byte
    this->write_byte(str.length() & 0xFF);         // Low byte

    for (char c : str) {
        this->write_byte(static_cast<uint8_t>(c));
    }
}

std::string c_packet::read_nbt_string() {
    uint16_t length = (this->read_byte() << 8) | this->read_byte();
    std::string result;

    for (int i = 0; i < length; ++i) {
        result += static_cast<char>(this->read_byte());
    }

    return result;
}

size_t c_packet::get_size()
{
    return this->data.size();
}

std::vector<uint8_t>& c_packet::get_raw()
{
    return this->data;
}

void c_packet::finalize()
{
    std::vector<uint8_t> with_length;

    // Compute VarInt of current packet size
    int32_t len = static_cast<int32_t>(this->data.size());

    // Write length as VarInt
    do {
        uint8_t temp = len & 0b01111111;
        len >>= 7;
        if (len != 0) {
            temp |= 0b10000000;
        }
        with_length.push_back(temp);
    } while (len != 0);

    // Append the actual packet data
    with_length.insert(with_length.end(), this->data.begin(), this->data.end());

    // Replace internal data
    this->data = std::move(with_length);
}

void c_packet::clear()
{
    this->data.clear();
    this->read_index = 0;
}
