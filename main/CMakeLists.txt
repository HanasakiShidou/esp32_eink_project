file (GLOB SOURCES 
    "../epd_cpp/lib/e-Paper/2in9v2/*.cpp"
    #"../epd_cpp/lib/Fonts/*.c"
    "app_main.c"
    "mqtt/mqtt_app.c"
    "eink/esp32_impl.cpp"
    "rpc/SakataOnEsp32Server.cpp"
    "rpc/SakataOnEsp32ServerImpl.cpp"
    )

idf_component_register(SRCS ${SOURCES}
                    PRIV_REQUIRES mqtt nvs_flash driver esp_netif led_strip
                    INCLUDE_DIRS 
                    "."
                    "../epd_cpp/lib/e-Paper"
                    "../epd_cpp/lib/e-Paper/2in9v2"
                    "../epd_cpp/lib/Config"
                    "../epd_cpp/lib/Fonts"
                    "../epd_cpp/lib/Hardware"
#                    "../epd_cpp/lib/GUI"
                    )
