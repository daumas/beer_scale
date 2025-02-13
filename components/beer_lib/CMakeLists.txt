set(COMPONENT_SRCDIRS
"src"
)
 
set(COMPONENT_ADD_INCLUDEDIRS
"src"
)
 
set(COMPONENT_REQUIRES
"arduino" "Adafruit_BusIO" "Adafruit_GFX_Library" "Adafruit_ST7735_and_ST7789_Library" "Ai_Esp32_Rotary_Encoder" "ESP32_WS2812_Lib" "HX711" "TonePlayer"
)
 
register_component()
 
target_compile_definitions(${COMPONENT_TARGET} PUBLIC -DESP32)
target_compile_options(${COMPONENT_TARGET} PRIVATE -fno-rtti) 