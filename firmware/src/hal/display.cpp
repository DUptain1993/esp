#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

static TFT_eSPI tft = TFT_eSPI();

/* -------- BUFFERS -------- */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[320 * 40];
static lv_color_t buf2[320 * 40];

/* -------- FLUSH -------- */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

/* -------- INIT -------- */
void hal_display_init()
{
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH); // BACKLIGHT ON

    tft.begin();
    tft.setRotation(1);

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 320 * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = 480; // Swapped for Rotation 1
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);
}
