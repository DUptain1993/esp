#include "display.h"
#include "esp_heap_caps.h"

TFT_eSPI tft = TFT_eSPI(HAL_DISP_HOR_RES, HAL_DISP_VER_RES);

// LVGL draw buffer descriptor + two line buffers (double buffering).
// Allocated from DMA-capable heap to keep static DRAM (.bss) free for WiFi.
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf_a = nullptr;
static lv_color_t *buf_b = nullptr;
static lv_disp_drv_t disp_drv;

void hal_display_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    const uint32_t w = (area->x2 - area->x1 + 1);
    const uint32_t h = (area->y2 - area->y1 + 1);

    // Push only the dirty region (minimised redraw area) using DMA-capable path.
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true); // true = swap bytes for RGB565
    tft.endWrite();

    lv_disp_flush_ready(drv);
}

// Backlight PWM (LEDC) configuration.
#define BL_LEDC_CHANNEL 7
#define BL_LEDC_FREQ    5000
#define BL_LEDC_RES     8       // 8-bit: 0..255
static uint8_t s_brightness = 100;

void hal_display_set_brightness(uint8_t pct)
{
    if (pct > 100) pct = 100;
    s_brightness = pct;
    uint32_t duty = (uint32_t)pct * 255 / 100; // active HIGH
    ledcWrite(BL_LEDC_CHANNEL, duty);
}

uint8_t hal_display_get_brightness(void) { return s_brightness; }

void hal_display_backlight(bool on)
{
    hal_display_set_brightness(on ? s_brightness : 0);
}

void hal_display_init(void)
{
    // Backlight via LEDC PWM; start off until the panel is ready (no flash).
    ledcSetup(BL_LEDC_CHANNEL, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(HAL_BL_PIN, BL_LEDC_CHANNEL);
    ledcWrite(BL_LEDC_CHANNEL, 0);

    // TFT_eSPI: SPI mode 0, 40 MHz, DMA enabled (pins/freq from build flags).
    tft.begin();
    tft.setRotation(0);          // portrait 320x480
    tft.initDMA();               // enable DMA transfers
    tft.fillScreen(TFT_BLACK);

    // LVGL double buffer init (DMA-capable RAM).
    const size_t buf_bytes = HAL_DISP_BUF_PIXELS * sizeof(lv_color_t);
    buf_a = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    buf_b = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    lv_disp_draw_buf_init(&draw_buf, buf_a, buf_b, HAL_DISP_BUF_PIXELS);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = HAL_DISP_HOR_RES;
    disp_drv.ver_res = HAL_DISP_VER_RES;
    disp_drv.flush_cb = hal_display_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 0;   // partial / dirty-region refresh only
    lv_disp_drv_register(&disp_drv);

    // Panel initialised: enable backlight so the UI is visible on first boot.
    hal_display_backlight(true);
}
