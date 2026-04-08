extern "C" {
#include "mqtt_app.h"
#include "SakataOnEsp32ServerImpl.h"
}
#include "esp32_impl.hpp"
#include "esp_log.h"
#include "epd_2in9v2.hpp"
#include "SakataOnEsp32Server.h"

static EPaper_2in9v2* epd = nullptr;

std::array<uint8_t, IMAGE_LENGTH + 500> requestBuffer{};
uint8_t reponseBuffer[500] = {};
std::array<uint8_t, IMAGE_LENGTH> image{};

class SakataOnEsp32ServerImplement: public SakataOnEsp32Server {
    public:
        void fillDisplayMem(uint8_t data[4736])override;
        void setDisplayMem(int32_t xIndex, int32_t yIndex)override;
        void refreshDisplay(uint8_t mode)override;
        void directlyDisplay(uint8_t data[4736], uint8_t mode)override;
        void fillAndRefresh(uint8_t data[4736], uint8_t mode)override;
        void setLed(uint8_t rVal, uint8_t gVal, uint8_t bVal)override;
        int32_t getStatus()override;
};

static SakataOnEsp32ServerImplement server;

void SakataOnEsp32ServerImplement::fillDisplayMem(uint8_t data[4736]) {

}

void SakataOnEsp32ServerImplement::setDisplayMem(int32_t xIndex, int32_t yIndex) {

}

void SakataOnEsp32ServerImplement::refreshDisplay(uint8_t mode) {

}

void SakataOnEsp32ServerImplement::directlyDisplay(uint8_t data[4736], uint8_t mode) {

}

void SakataOnEsp32ServerImplement::fillAndRefresh(uint8_t data[4736], uint8_t mode) {

}

void SakataOnEsp32ServerImplement::setLed(uint8_t rVal, uint8_t gVal, uint8_t bVal) {

}

int32_t SakataOnEsp32ServerImplement::getStatus() {
    return 0;
}

//bool handle_request(const uint8_t* request, int request_size, uint8_t* response, int& response_size);

void init_sakata_on_esp32(void) {
    epd = new EPaper_2in9v2(std::unique_ptr<HardwareAPI>(new Esp32Impl()));
    epd->Init();
    epd->Clear();
}

void mqtt_handle_request(unsigned char *data, int length) {
    if (!epd)
        return;

    int response_size = 0;

    server.handle_request(data, length, reponseBuffer, response_size);

    mqtt_handle_request(reponseBuffer, response_size);
}