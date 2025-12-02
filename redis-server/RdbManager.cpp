#include "RdbManager.hpp"

#include <fstream>

namespace kredis
{
void RdbManager::write_length(std::ofstream &ofile, std::size_t length)
{
    /*    
    Bits	How to parse
    00	    The next 6 bits represent the length
    01	    Read one additional byte. The combined 14 bits represent the length
    10	    Discard the remaining 6 bits. The next 4 bytes from the stream represent the length
    11	    The next object is encoded in a special format. The remaining 6 bits indicate the format.
            May be used to store numbers or Strings, see String Encoding.
    */

    if (length <= 63)
    {
        unsigned char byte = static_cast<unsigned char>(length & 0b00111111);
        ofile.put(byte);
    }
    else if (length <= 16383)
    {
        unsigned char second_byte = static_cast<unsigned char>(length & 0xff);
        unsigned char first_byte = 0b01000000 | static_cast<unsigned char>((length >> 8) & 0b00111111);
        ofile.put(first_byte);
        ofile.put(second_byte);
    }
    else if (length <= 0xFFFFFFFF)
    {
        auto length_32bits = static_cast<std::uint32_t>(length);
        ofile.put(0b10000000);
        ofile.write(reinterpret_cast<const char*>(&length_32bits), sizeof(length_32bits));
    }
    // Ignore 11 case for now
}

std::size_t RdbManager::read_length(std::ifstream &ifile)
{
    char byte;
    ifile.get(byte);
    int first_two_bits = (byte >> 6) & 0x3;
    switch (first_two_bits)
    {
        case 0:
        {
            return static_cast<std::size_t>(byte & 0b00111111);
        }
        case 1:
        {
            char second_byte;
            ifile.get(second_byte);
            std::size_t length = static_cast<std::uint16_t>(second_byte) + (static_cast<std::uint16_t>(byte & 0b00111111) << 8);
            return length;
        }
        case 2:
        {
            std::uint32_t length = 0;
            ifile.read(reinterpret_cast<char*>(&length), sizeof(length));
            return length;
        }
        case 3:
        default:
            return 0;
    }

    return 0;
}

bool RdbManager::save(const RdbManager::DictionarySnapshotType& dictionary_snapshot)
{
    std::ofstream ofile{"dump.rdb", std::ios::binary |  std::ios::trunc};
    if (!ofile.is_open())
    {
        return false;
    }
    ofile.write("REDIS0005", 9);
    ofile.put(0xFA);

    // Redis version as key and value
    // 0b00 000111 VERSION
    // 0b00 000001 3
    ofile.put(0x07);
    ofile.write("VERSION", 7);
    ofile.put(0x01);
    ofile.put('3');
    
    // Database selector 0
    ofile.put(0xFE);
    ofile.put(0x00);
    ofile.put(0xFB);

    // Length of dict and length of expiry dict - we keep both values same for now
    const std::size_t dict_size = dictionary_snapshot.size();
    write_length(ofile, dict_size);
    write_length(ofile, dict_size);

    // FD <4 byte> - expiry in secs
    // FC <8 byte> - expiry in millisecs - we use only FC
    // value type
    // key
    // value
    for (const auto& [key, value] : dictionary_snapshot)
    {
        if (value.m_expiry_timestamp.has_value())
        {
            ofile.put(0xFC);
            std::uint64_t expiry_millis = value.m_expiry_timestamp.value();
            ofile.write(reinterpret_cast<const char*>(&expiry_millis), sizeof(expiry_millis));
        }
        if (std::holds_alternative<RespString>(*value.m_value))
        {
            ofile.put(0x00); // String type
            write_length(ofile, key.size());
            ofile.write(key.data(), key.size());

            const auto& val_str = std::get<RespString>(*value.m_value).m_str;
            write_length(ofile, val_str.size());
            ofile.write(val_str.data(), val_str.size());
        }
        else if (std::holds_alternative<RespArray>(*value.m_value))
        {
            ofile.put(0x01); // List type
            write_length(ofile, key.size());
            ofile.write(key.data(), key.size());

            const auto& val_arr = std::get<RespArray>(*value.m_value);
            write_length(ofile, val_arr.size());
            for (const auto& val : val_arr)
            {
                // Assuming the value is RespString for now
                const auto& val_str = std::get<RespString>(val).m_str;
                write_length(ofile, val_str.size());
                ofile.write(val_str.data(), val_str.size());
            }
        }
    }

    ofile.put(0xFF);  // EOF

    // Put dummy value for checksum for now
    std::uint64_t dummy_checksum = 0;
    ofile.write(reinterpret_cast<const char*>(&dummy_checksum), sizeof(dummy_checksum));

    ofile.close();
    return true;
}

std::optional<RdbManager::DictionarySnapshotType> RdbManager::load()
{
    std::ifstream ifile{"dump.rdb",  std::ios::binary};
    if (!ifile.is_open())
    {
        return std::nullopt;
    }
    // Ignore first 10 bytes - REDIS0005 0xFA - useless for now
    // Ignore next 10 bytes - redis version as key and value - useless for now
    // Ignore next 3 bytes - database selector - useless for now
    ifile.seekg(23, std::ios::beg);
    
    // Read dict length, expiry dict length - use dict length
    std::size_t dict_length = read_length(ifile);
    read_length(ifile); // expiry dict length - ignore for now
    
    // Read key-value pairs, store in dictionary_snapshot
    DictionarySnapshotType dictionary_snapshot{dict_length};
    for (std::size_t index = 0; index < dict_length; index++)
    {
        char first_byte;
        std::optional<std::uint64_t> expiry_timestamp = std::nullopt;
        ifile.get(first_byte);
        if (first_byte == static_cast<char>(0xFC))
        {
            // Read expiry timestamp - 8 bytes
            std::uint64_t expiry_millis = 0;
            ifile.read(reinterpret_cast<char*>(&expiry_millis), sizeof(expiry_millis));
            expiry_timestamp = expiry_millis;

            // Read next byte for value type
            ifile.get(first_byte);
        }
        if (first_byte == static_cast<char>(0x00))
        {
            auto key_len = read_length(ifile);
            std::string key(key_len, '\0');
            ifile.read(key.data(), key_len);

            auto val_len = read_length(ifile);
            std::string value(val_len, '\0');
            ifile.read(value.data(), val_len);

            dictionary_snapshot[index] = std::make_pair(key, DictionaryValue{std::make_shared<RespType>(RespString{value, true}), expiry_timestamp});
        }
        else if (first_byte == static_cast<char>(0x01))
        {
            auto key_len = read_length(ifile);
            std::string key(key_len, '\0');
            ifile.read(key.data(), key_len);

            auto list_size = read_length(ifile);
            RespArray list_values{list_size};
            for (std::size_t list_index = 0; list_index < list_size; list_index++)
            {
                auto val_len = read_length(ifile);
                std::string value(val_len, '\0');
                ifile.read(value.data(), val_len);
                list_values[list_index] = RespString{value, true};
            }
            dictionary_snapshot[index] = std::make_pair(key, DictionaryValue{std::make_shared<RespType>(list_values), expiry_timestamp});
        }
        else
        {
            ifile.close();
            return std::nullopt;
        }
    }

    // Read EOF
    char eof_byte;
    ifile.get(eof_byte);

    // Ignore checksum for now

    ifile.close();
    return dictionary_snapshot;
}
} // namespace kredis