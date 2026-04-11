extern "C" {
#include "../mqtt/mqtt_app.h"
#include "SakataOnEsp32ServerImpl.h"
}
#include "../eink/esp32_impl.hpp"
#include "esp_log.h"
#include "epd_2in9v2.hpp"
#include "SakataOnEsp32Server.h"

constexpr char TAG[] = "SakataOnEsp32ServerImpl";

static EPaper_2in9v2* epd = nullptr;

//static std::array<uint8_t, IMAGE_LENGTH + 500> requestBuffer{};
static char reponseBuffer[500] = {0};

// Graph memory
static std::array<uint8_t, IMAGE_LENGTH> image{0};

// tool functions
// Note: 0b0 -> black, 0b1 -> white
static inline void clearImage(bool val = true) {
    for (int index = 0; index < image.size(); index++) {
        image.at(index) = val ? 0xff: 0x0;
    }
}

static inline void setPoint(int32_t xIndex, int32_t yIndex, bool value) {
    int bitIndex = (yIndex * EPD_2IN9_V2_WIDTH) + xIndex;
    int byteIndex = bitIndex / 8;
    int bitOffset = bitIndex % 8;
    if (byteIndex > 0 && byteIndex < image.size()) {
        auto oldValue = image.at(byteIndex);
        auto newValue = value ? (oldValue | 1 << bitOffset) : (oldValue & ~(1 << bitOffset));
        image.at(byteIndex) = newValue;
    }
}

void createTestPattern(bool& rr)
{    
    constexpr int x_len = (EPD_2IN9_V2_WIDTH / 8) + (EPD_2IN9_V2_WIDTH % 8 == 0 ? 0: 1);
    for (int y = 0; y < EPD_2IN9_V2_HEIGHT; ++y)
    {
        for (int x = 0; x < EPD_2IN9_V2_WIDTH; ++x)
        {
            if ((x + y ) % 2 == 0) {
                uint8_t bitVal = rr ? 1: 0;
                int bitIndex = x % 8;
                bitVal <<= bitIndex;
                image.at((x / 8) + y * x_len) |= bitVal;
            } else {
                uint8_t bitVal = rr ? 0: 1;
                int bitIndex = x % 8;
                bitVal <<= bitIndex;
                image.at((x / 8) + y * x_len) |= bitVal;
            }
        }
    }
    rr = !rr;
    return;
}

class SakataOnEsp32ServerImplement: public SakataOnEsp32Server {
    public:
        void fillDisplayMem(uint8_t data[4736])override;
        void clearDisplay(int32_t data)override;
        void setDisplayMem(int32_t xIndex, int32_t yIndex, int32_t value)override;
        void refreshDisplay(uint8_t mode)override;
        void directlyDisplay(uint8_t data[4736], uint8_t mode)override;
        void fillAndRefresh(uint8_t data[4736], uint8_t mode)override;
        void setLed(uint8_t rVal, uint8_t gVal, uint8_t bVal)override;
        int32_t getStatus()override;
};

static SakataOnEsp32ServerImplement server;

void SakataOnEsp32ServerImplement::fillDisplayMem(uint8_t data[4736]) {
    for (int index = 0; index < image.size(); index++) {
        image.at(index) = data[index];
    }
}

void SakataOnEsp32ServerImplement::clearDisplay(int32_t data) {
    clearImage(data != 0);
    epd->Display(image.data());
}

void SakataOnEsp32ServerImplement::setDisplayMem(int32_t xIndex, int32_t yIndex, int32_t value) {
    setPoint(xIndex, yIndex, value != 0);
}

void SakataOnEsp32ServerImplement::refreshDisplay(uint8_t mode) {
    switch(mode) {
        // full refresh
        case 0:
            epd->Display(image.data());
            break;
        // partial refresh
        case 1:
            epd->DisplayPartial(image.data());
            break;
    }
}

void SakataOnEsp32ServerImplement::directlyDisplay(uint8_t data[4736], uint8_t mode) {
    switch(mode) {
        // full refresh
        case 0:
            epd->Display(data);
            break;
        // partial refresh
        case 1:
            epd->DisplayPartial(data);
            break;
    }
}

void SakataOnEsp32ServerImplement::fillAndRefresh(uint8_t data[4736], uint8_t mode) {
    for (int index = 0; index < image.size(); index++) {
        image.at(index) = data[index];
    }
    switch(mode) {
        // full refresh
        case 0:
            epd->Display(image.data());
            break;
        // partial refresh
        case 1:
            epd->DisplayPartial(image.data());
            break;
    }
}

void SakataOnEsp32ServerImplement::setLed(uint8_t rVal, uint8_t gVal, uint8_t bVal) {
    // TODO
}

int32_t SakataOnEsp32ServerImplement::getStatus() {
    return 0;
}

void debug_image() {
    printf("Image Buffer:\n");
    for (int index = 0; index < image.size(); index++) {
        printf("%02x ",image.at(index));
        if (index % 30 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void init_sakata_on_esp32(void) {
    epd = new EPaper_2in9v2(std::unique_ptr<HardwareAPI>(new Esp32Impl()));
    epd->Init();
    //epd->Clear();
}

void mqtt_handle_request(unsigned char *data, int length) {
    if (data == NULL)
        return;
    int response_size = 0;
    if (epd) {
        server.handle_request(data, length, reinterpret_cast<uint8_t *>(reponseBuffer), response_size);
    }
    if (response_size == 0) {
        response_size = 1;
        reponseBuffer[0] = 0x0;
    }
    mqtt_send_response(reponseBuffer, response_size);
}