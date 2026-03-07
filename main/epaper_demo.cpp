extern "C" {
#include "epaper_demo.h"
}
#include "esp32_impl.hpp"
#include "esp_log.h"

EPaper_2in9v2* epd = nullptr;

void createTestPattern(std::array<uint8_t, IMAGE_LENGTH>& image, bool& rr)
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

void clearTestPattern(std::array<uint8_t, IMAGE_LENGTH>& image) {
    for (int index = 0; index < image.size(); index++) {
        image.at(index) = 0xff;
    }
}

void fillTestPattern(std::array<uint8_t, IMAGE_LENGTH>& image) {
    for (int index = 0; index < image.size(); index++) {
        image.at(index) = 0x0;
    }
}

std::array<uint8_t, IMAGE_LENGTH> image{};

int startDemo() {
    epd = new EPaper_2in9v2(std::unique_ptr<HardwareAPI>(new Esp32Impl()));
    epd->Init();
    epd->Clear();
    bool rr = false;
    fillTestPattern(image);
    epd->Display(image.data());
    ESP_LOGI("ESP32Impl", "print black done!");
    epd->Delay(5000);

    clearTestPattern(image);
    createTestPattern(image, rr);
    epd->Display(image.data());
    ESP_LOGI("ESP32Impl", "print test pic done!");
    epd->Delay(5000);

    clearTestPattern(image);
    epd->Display(image.data());
    ESP_LOGI("ESP32Impl", "print clear pic done!");
    epd->Delay(5000);

    epd->Clear();

    return 0;
}