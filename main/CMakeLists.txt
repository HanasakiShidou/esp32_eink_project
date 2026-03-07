file (GLOB SOURCES 
    "../epd_cpp/lib/e-Paper/2in9v2/*.cpp"
    "../epd_cpp/lib/Fonts/*.c"
#    "../epd_cpp/lib/GUI/*.c"
    "app_main.c"
    "esp32_impl.cpp"
    "epaper_demo.cpp"
    )

idf_component_register(SRCS ${SOURCES}
                    PRIV_REQUIRES mqtt nvs_flash driver esp_netif
                    INCLUDE_DIRS 
                    "."
                    "../epd_cpp/lib/e-Paper"
                    "../epd_cpp/lib/e-Paper/2in9v2"
                    "../epd_cpp/lib/Config"
                    "../epd_cpp/lib/Fonts"
                    "../epd_cpp/lib/Hardware"
#                    "../epd_cpp/lib/GUI"
                    )
