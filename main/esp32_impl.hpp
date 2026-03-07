#include "hardwareapi.hpp"
#include "epd_2in9v2.hpp"

class Esp32Impl : public HardwareAPI
{
public:
    Esp32Impl();
    int DigitalWrite(EPD_IO_PIN pin, uint8_t value) override;
    uint8_t DigitalRead(EPD_IO_PIN pin) override;
    int SPIWriteByte(uint8_t value) override;
    int SPIWriteBytes(uint8_t *pData, uint32_t len) override;
    int Delay(uint32_t ms) override;
    int Init() override;
    bool Available() override;
    ~Esp32Impl() override;
private:
    bool isReady{false};
};

