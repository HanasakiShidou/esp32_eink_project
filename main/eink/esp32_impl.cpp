#include "esp32_impl.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <array>

static constexpr std::array<int, EPD_IO_PIN::_LEN> ESP32C3_MAPPINGS = {
    -1, // PWR
    3,  // DIN
    4,  // CLK
    5,  // CS
    2,  // DC
    1,  // RST
    0,  // BUSY
};

auto constexpr SPI_HOST = SPI2_HOST;
constexpr int TX_BUFFER_SIZE = 16;

// Esp32 SPI driver is not thread safe, so put a global value here is okay
spi_device_handle_t spi_handle = nullptr;

Esp32Impl::Esp32Impl()
{
    // init gpio
    auto outputGpio = {
        ESP32C3_MAPPINGS.at(EPD_IO_PIN::CS),
        ESP32C3_MAPPINGS.at(EPD_IO_PIN::DC),
        ESP32C3_MAPPINGS.at(EPD_IO_PIN::RST),
    };
    for (auto pinIndex: outputGpio) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1ULL << pinIndex);
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
    }

    auto inputGpio = {ESP32C3_MAPPINGS.at(EPD_IO_PIN::BUSY)};
    for (auto pinIndex: inputGpio) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << pinIndex);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
    }

    // init spi
    esp_err_t ret;
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = ESP32C3_MAPPINGS.at(EPD_IO_PIN::DIN);
    buscfg.miso_io_num = -1;                    // input is not used.
    buscfg.sclk_io_num = ESP32C3_MAPPINGS.at(EPD_IO_PIN::CLK);
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = TX_BUFFER_SIZE;
    
    ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits = 0;
    devcfg.address_bits = 0;
    devcfg.dummy_bits = 0;
    devcfg.mode = 0;                             // SPI mode 0 (CPOL=0，CPHA=0);
    devcfg.clock_speed_hz = 1 * 1000 * 1000;     // Clock freq is 1MHz;
    devcfg.spics_io_num = -1;
    devcfg.flags = 0;
    devcfg.queue_size = 1;
    devcfg.pre_cb = NULL;
    devcfg.post_cb = NULL;

    ret = spi_bus_add_device(SPI_HOST, &devcfg, &spi_handle);

    ESP_ERROR_CHECK(ret);

    isReady = true;
}

int Esp32Impl::DigitalWrite(EPD_IO_PIN pin, uint8_t value) {
    auto ret = gpio_set_level(static_cast<gpio_num_t>(ESP32C3_MAPPINGS.at(pin)), static_cast<uint32_t>(value));
    return static_cast<int>(ret == ESP_OK);
}

uint8_t Esp32Impl::DigitalRead(EPD_IO_PIN pin) {
    return static_cast<uint8_t>(gpio_get_level(static_cast<gpio_num_t>(ESP32C3_MAPPINGS.at(pin))));
}

int Esp32Impl::SPIWriteByte(uint8_t value) {
    esp_err_t ret;

    spi_transaction_t trans = {};
    trans.length = sizeof(value) * 8;
    trans.tx_buffer = &value;
    trans.rx_buffer = nullptr;
    trans.flags = 0;
    
    ret = spi_device_transmit(spi_handle, &trans);

    return ret == ESP_OK ? 0: 1;
}

int Esp32Impl::SPIWriteBytes(uint8_t *pData, uint32_t len) {
    esp_err_t ret;

    spi_transaction_t trans = {};
    trans.length = len * 8;
    trans.tx_buffer = pData;
    trans.rx_buffer = nullptr;
    trans.flags = 0;
    
    ret = spi_device_transmit(spi_handle, &trans);

    return ret == ESP_OK ? 0: 1;
}

int Esp32Impl::Delay(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
    return 0;
}

int Esp32Impl::Init() {
    return 0;
}

bool Esp32Impl::Available() {
    return isReady;
}

Esp32Impl::~Esp32Impl()
{

}