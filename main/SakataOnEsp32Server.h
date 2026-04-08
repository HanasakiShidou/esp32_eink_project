// Auto-generated RPC Server for SakataOnEsp32
#ifndef SAKATAONESP32_SERVER_H
#define SAKATAONESP32_SERVER_H

#include <cstdint>
#include <cstring>

class SakataOnEsp32Server {
public:
    virtual ~SakataOnEsp32Server() = default;

    // Pure virtual functions to be implemented by user
    virtual void fillDisplayMem(uint8_t data[4736]) = 0;
    virtual void setDisplayMem(int32_t xIndex, int32_t yIndex) = 0;
    virtual void refreshDisplay(uint8_t mode) = 0;
    virtual void directlyDisplay(uint8_t data[4736], uint8_t mode) = 0;
    virtual void fillAndRefresh(uint8_t data[4736], uint8_t mode) = 0;
    virtual void setLed(uint8_t rVal, uint8_t gVal, uint8_t bVal) = 0;
    virtual int32_t getStatus() = 0;

    // Entry point to handle a request
    bool handle_request(const uint8_t* request, int request_size, uint8_t* response, int& response_size);

};

#endif // SAKATAONESP32_SERVER_H
