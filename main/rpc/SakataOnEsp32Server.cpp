// Auto-generated RPC Server Implementation for SakataOnEsp32
#include "SakataOnEsp32Server.h"


// Serialize/Deserialize templates.
template<typename T>
static void serialize(uint8_t* buffer, T value, int& offset) {
    static_assert(sizeof(T) <= 8, "Type size too large");
    for(size_t i = 0; i < sizeof(T); ++i) {
        buffer[offset++] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }
}

template<typename T>
static void deserialize(const uint8_t* buffer, T& value, int& offset) {
    static_assert(sizeof(T) <= 8, "Type size too large");
    value = 0;
    for(size_t i = 0; i < sizeof(T); ++i) {
        value |= static_cast<T>(static_cast<uint64_t>(buffer[offset + i]) << (i * 8));
    }
    offset += sizeof(T);
}

// For bool values
template<>
void serialize<bool>(uint8_t* buffer, bool value, int& offset) {
    buffer[offset++] = value ? 1 : 0;
}

template<>
void deserialize<bool>(const uint8_t* buffer, bool& value, int& offset) {
    value = buffer[offset++] != 0;
}

// For arrays
template<typename T, size_t N>
void serialize_array(uint8_t* buffer, T (arr)[N], int& offset) {
    for(size_t i = 0; i < N; ++i) {
        serialize<T>(buffer, arr[i], offset);
    }
}

template<typename T, size_t N>
void deserialize_array(const uint8_t* buffer, T (&arr)[N], int& offset) {
    for(size_t i = 0; i < N; ++i) {
        deserialize<T>(buffer, arr[i], offset);
    }
}


bool SakataOnEsp32Server::handle_request(const uint8_t* request, int request_size, uint8_t* response, int& response_size) {
    if (request_size < 1) {
        return false;
    }

    uint8_t func_id = request[0];
    int offset = 1;

    switch (func_id) {
        case 0x01:  // fillDisplayMem
        {
            uint8_t data[4736];
            deserialize_array<uint8_t, 4736>(request, data, offset);
            // Call actual function
            fillDisplayMem(data);
            response_size = 0;
        }
        break;
        case 0x02:  // clearDisplay
        {
            int32_t data;
            deserialize<int32_t>(request, data, offset);
            // Call actual function
            clearDisplay(data);
            response_size = 0;
        }
        break;
        case 0x03:  // setDisplayMem
        {
            int32_t xIndex;
            deserialize<int32_t>(request, xIndex, offset);
            int32_t yIndex;
            deserialize<int32_t>(request, yIndex, offset);
            int32_t value;
            deserialize<int32_t>(request, value, offset);
            // Call actual function
            setDisplayMem(xIndex, yIndex, value);
            response_size = 0;
        }
        break;
        case 0x04:  // refreshDisplay
        {
            uint8_t mode;
            deserialize<uint8_t>(request, mode, offset);
            // Call actual function
            refreshDisplay(mode);
            response_size = 0;
        }
        break;
        case 0x05:  // directlyDisplay
        {
            uint8_t data[4736];
            deserialize_array<uint8_t, 4736>(request, data, offset);
            uint8_t mode;
            deserialize<uint8_t>(request, mode, offset);
            // Call actual function
            directlyDisplay(data, mode);
            response_size = 0;
        }
        break;
        case 0x06:  // fillAndRefresh
        {
            uint8_t data[4736];
            deserialize_array<uint8_t, 4736>(request, data, offset);
            uint8_t mode;
            deserialize<uint8_t>(request, mode, offset);
            // Call actual function
            fillAndRefresh(data, mode);
            response_size = 0;
        }
        break;
        case 0x07:  // setLed
        {
            uint8_t rVal;
            deserialize<uint8_t>(request, rVal, offset);
            uint8_t gVal;
            deserialize<uint8_t>(request, gVal, offset);
            uint8_t bVal;
            deserialize<uint8_t>(request, bVal, offset);
            // Call actual function
            setLed(rVal, gVal, bVal);
            response_size = 0;
        }
        break;
        case 0x08:  // getStatus
        {

            // Call actual function
            int32_t result = getStatus();

            // Serialize return value
            offset = 0;
            serialize<int32_t>(response, result, offset);
            response_size = offset;
        }
        break;
        default:
            return false;
    }

    return true;
}
