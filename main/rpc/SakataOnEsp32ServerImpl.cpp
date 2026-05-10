extern "C" {
#include "../mqtt/mqtt_app.h"
#include "SakataOnEsp32ServerImpl.h"
}
#include "../eink/esp32_impl.hpp"
#include "esp_log.h"
#include "epd_2in9v2.hpp"
#include "SakataOnEsp32Server.h"
#include "led_strip.h"

constexpr char TAG[] = "SakataOnEsp32ServerImpl";

static constexpr int WS2812_GPIO = 10;

static EPaper_2in9v2* epd = nullptr;
static led_strip_handle_t led_strip = nullptr;

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
    if (led_strip == nullptr) {
        return;
    }
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, rVal, gVal, bVal));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
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

    // Init WS2812 on GPIO10
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
    };

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000;
    rmt_config.flags.with_dma = false;

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    // Turn off LED on startup
    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    led_strip_refresh(led_strip);
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