#include "ui.h"
#include "theme.h"
#include "gestures.h"
#include "../core/comms.h"
#include "../core/commands.h"
#include "../hal/display.h"
#include "../app/settings.h"

#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"

// ---------------------------------------------------------------------------
//  State
// ---------------------------------------------------------------------------
static SemaphoreHandle_t s_lvgl_mutex = nullptr;

static lv_obj_t *s_scr      = nullptr;
static lv_obj_t *s_page_lbl = nullptr;
static lv_obj_t *s_back_btn = nullptr;

// Bottom HUD.
static lv_obj_t *s_hud_dev    = nullptr;
static lv_obj_t *s_hud_status = nullptr;
static lv_obj_t *s_hud_cpu    = nullptr;

static lv_obj_t *s_pages[PAGE_COUNT] = {nullptr};
static int       s_cur_page = PAGE_HOME;

// Home dashboard live widgets.
static lv_obj_t *s_home_uptime = nullptr;
static lv_obj_t *s_home_heap   = nullptr;
static lv_obj_t *s_home_wifi   = nullptr;

// Terminal.
static lv_obj_t *s_stream_ta = nullptr;
static lv_obj_t *s_term_input = nullptr;
static lv_obj_t *s_kb         = nullptr;

// WiFi scanner.
static lv_obj_t *s_wifi_list   = nullptr;
static lv_obj_t *s_wifi_status = nullptr;
static bool      s_wifi_scanning = false;
static String    s_wifi_target_ssid = "";

// System monitor.
static lv_obj_t *s_sys_chip   = nullptr;
static lv_obj_t *s_sys_cpu    = nullptr;
static lv_obj_t *s_sys_heapbar = nullptr;
static lv_obj_t *s_sys_heaplbl = nullptr;
static lv_obj_t *s_sys_flash  = nullptr;
static lv_obj_t *s_sys_uptime = nullptr;
static lv_obj_t *s_sys_temp   = nullptr;
static lv_obj_t *s_ota_bar    = nullptr;
static lv_obj_t *s_ota_lbl    = nullptr;

// Settings.
static lv_obj_t *s_set_bright = nullptr;

// Mirror.
static lv_obj_t  *s_mirror_canvas = nullptr;
static lv_color_t *s_canvas_buf   = nullptr;

// Devices.
static lv_obj_t *s_dev_list   = nullptr;
static lv_obj_t *s_dev_active = nullptr;

#define CANVAS_W 160
#define CANVAS_H 160
#define CONTENT_Y 40
#define CONTENT_H 396

static const char *PAGE_TITLES[PAGE_COUNT] = {
    "HOME", "TERMINAL", "WIFI", "SYSTEM", "SETTINGS", "MIRROR", "DEVICES",
};

// ---------------------------------------------------------------------------
//  Threading lock
// ---------------------------------------------------------------------------
void ui_lock(void)   { if (s_lvgl_mutex) xSemaphoreTakeRecursive(s_lvgl_mutex, portMAX_DELAY); }
void ui_unlock(void) { if (s_lvgl_mutex) xSemaphoreGiveRecursive(s_lvgl_mutex); }
bool ui_try_lock(uint32_t ms)
{
    if (!s_lvgl_mutex) return false;
    return xSemaphoreTakeRecursive(s_lvgl_mutex, pdMS_TO_TICKS(ms)) == pdTRUE;
}

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------
static void fmt_uptime(char *buf, size_t n)
{
    uint32_t s = millis() / 1000;
    snprintf(buf, n, "%02u:%02u:%02u", (s / 3600), (s / 60) % 60, s % 60);
}

static void fmt_bytes(char *buf, size_t n, uint32_t bytes)
{
    if (bytes >= 1024 * 1024) snprintf(buf, n, "%.1fMB", bytes / 1048576.0);
    else if (bytes >= 1024)   snprintf(buf, n, "%.1fKB", bytes / 1024.0);
    else                      snprintf(buf, n, "%uB", (unsigned)bytes);
}

static lv_color_t heap_color(uint32_t freePct)
{
    if (freePct < 15) return THEME_COL_ERR;
    if (freePct < 35) return THEME_COL_WARN;
    return THEME_COL_OK;
}

// ---------------------------------------------------------------------------
//  Navigation
// ---------------------------------------------------------------------------
int ui_current_page(void) { return s_cur_page; }

void ui_show_page(int page)
{
    if (page < 0 || page >= PAGE_COUNT) return;
    s_cur_page = page;
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (!s_pages[i]) continue;
        if (i == page) lv_obj_clear_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (s_page_lbl) lv_label_set_text(s_page_lbl, PAGE_TITLES[page]);
    if (s_back_btn) {
        if (page == PAGE_HOME) lv_obj_add_flag(s_back_btn, LV_OBJ_FLAG_HIDDEN);
        else                   lv_obj_clear_flag(s_back_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_next_page(void)
{
    ui_show_page((s_cur_page + 1) % PAGE_COUNT);
    comms_send_cmd(CH_CONTROL, DEVICE_SELF, CMD_PAGE_NEXT);
}
void ui_prev_page(void)
{
    ui_show_page((s_cur_page - 1 + PAGE_COUNT) % PAGE_COUNT);
    comms_send_cmd(CH_CONTROL, DEVICE_SELF, CMD_PAGE_PREV);
}
void ui_quick_action(void)
{
    ui_set_status("QUICK"); ui_set_status_color(THEME_COL_WARN);
    comms_send_cmd(CH_CONTROL, DEVICE_SELF, CMD_QUICK_ACTION);
}
void ui_execute(void)
{
    ui_set_status("EXEC"); ui_set_status_color(THEME_COL_OK);
    comms_send_cmd(CH_CONTROL, DEVICE_SELF, CMD_EXECUTE);
}

// ---------------------------------------------------------------------------
//  HUD
// ---------------------------------------------------------------------------
void ui_set_status(const char *txt) { if (s_hud_status && txt) lv_label_set_text(s_hud_status, txt); }
void ui_set_status_color(lv_color_t c) { if (s_hud_status) lv_obj_set_style_text_color(s_hud_status, c, LV_PART_MAIN); }
void ui_set_cpu(int pct)
{
    if (!s_hud_cpu) return;
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    char b[16]; snprintf(b, sizeof(b), "CPU %d%%", pct);
    lv_label_set_text(s_hud_cpu, b);
}

// ---------------------------------------------------------------------------
//  Events
// ---------------------------------------------------------------------------
static void nav_btn_cb(lv_event_t *e) { ui_show_page((int)(intptr_t)lv_event_get_user_data(e)); }
static void back_btn_cb(lv_event_t *e) { (void)e; ui_show_page(PAGE_HOME); }

static void term_input_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    const char *txt = lv_textarea_get_text(ta);
    if (txt && strlen(txt) > 0) {
        comms_send_cmd(CH_CONTROL, DEVICE_SELF, CMD_INPUT, (const uint8_t*)txt, (uint8_t)strlen(txt));
        lv_textarea_set_text(ta, "");
    }
}

static void term_kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        if (s_stream_ta) lv_obj_set_height(s_stream_ta, CONTENT_H - 8);
    }
}

static void term_ta_event_cb(lv_event_t *e)
{
    if (s_kb) lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    if (s_stream_ta) lv_obj_set_height(s_stream_ta, CONTENT_H - 8);
}

static void term_input_focus_cb(lv_event_t *e)
{
    if (s_kb) lv_obj_clear_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    if (s_stream_ta) lv_obj_set_height(s_stream_ta, CONTENT_H - 160);
}

static void wifi_connect_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *mbox = lv_msgbox_get_from_btn(obj);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);
    const char *pass = lv_textarea_get_text(ta);

    if (s_wifi_target_ssid.length() > 0) {
        WiFi.begin(s_wifi_target_ssid.c_str(), pass);
        Settings::getInstance().setWifiCreds(s_wifi_target_ssid, pass);
        ui_set_status("WIFI CONNECT");
    }
    lv_msgbox_close(mbox);
}

static void wifi_row_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *row_text = lv_list_get_btn_text(s_wifi_list, btn);
    String row = String(row_text);
    int start = 3; 
    int end = row.lastIndexOf("  ");
    if (end > start) {
        s_wifi_target_ssid = row.substring(start, end);
        s_wifi_target_ssid.trim();

        static const char *btns[] = {"Connect", "Cancel", ""};
        lv_obj_t *mbox = lv_msgbox_create(NULL, "WiFi Connect", s_wifi_target_ssid.c_str(), btns, true);
        lv_obj_center(mbox);

        lv_obj_t *ta = lv_textarea_create(mbox);
        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_password_mode(ta, true);
        lv_obj_set_width(ta, 200);

        lv_obj_t *kb = lv_keyboard_create(lv_scr_act());
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_set_size(kb, 320, 160);
        
        lv_obj_add_event_cb(mbox, [](lv_event_t *e) {
            lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
            lv_obj_del(kb);
        }, LV_EVENT_DELETE, kb);

        lv_obj_t *connect_btn = lv_msgbox_get_btns(mbox);
        lv_obj_add_event_cb(connect_btn, wifi_connect_cb, LV_EVENT_CLICKED, ta);
    }
}

static void wifi_scan_cb(lv_event_t *e)
{
    (void)e;
    if (s_wifi_scanning) return;
    s_wifi_scanning = true;
    if (s_wifi_status) {
        lv_label_set_text(s_wifi_status, "scanning...");
        lv_obj_set_style_text_color(s_wifi_status, THEME_COL_WARN, LV_PART_MAIN);
    }
    WiFi.scanDelete();
    WiFi.scanNetworks(true /*async*/, true /*show hidden*/);
}

static void bright_cb(lv_event_t *e)
{
    lv_obj_t *s = lv_event_get_target(e);
    hal_display_set_brightness((uint8_t)lv_slider_get_value(s));
}

static void obf_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    comms_set_obfuscation(lv_obj_has_state(sw, LV_STATE_CHECKED));
}

static void reboot_cb(lv_event_t *e) { (void)e; ESP.restart(); }

static void dev_refresh_cb(lv_event_t *e)
{
    comms_send_cmd(CH_ROUTING, DEVICE_SELF, RT_LIST);
    ui_set_status("DEV REFRESH");
}

static void dev_row_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    uint8_t id = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    comms_send_cmd(CH_ROUTING, DEVICE_SELF, RT_SELECT, &id, 1);
}

static void wifi_reconnect_label(void)
{
    if (!s_home_wifi) return;
    char b[40];
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(b, sizeof(b), LV_SYMBOL_WIFI " %s", WiFi.SSID().c_str());
        lv_obj_set_style_text_color(s_home_wifi, THEME_COL_OK, LV_PART_MAIN);
    } else {
        snprintf(b, sizeof(b), LV_SYMBOL_WIFI " idle");
        lv_obj_set_style_text_color(s_home_wifi, THEME_COL_DIM, LV_PART_MAIN);
    }
    lv_label_set_text(s_home_wifi, b);
}

// ---------------------------------------------------------------------------
//  Live update timer (runs inside lv_timer_handler -> already UI-locked)
// ---------------------------------------------------------------------------
static void populate_wifi_results(int n)
{
    if (!s_wifi_list) return;
    lv_obj_clean(s_wifi_list);
    if (n <= 0) {
        if (s_wifi_status) {
            lv_label_set_text(s_wifi_status, "no networks found");
            lv_obj_set_style_text_color(s_wifi_status, THEME_COL_DIM, LV_PART_MAIN);
        }
        return;
    }
    if (n > 20) n = 20;
    for (int i = 0; i < n; i++) {
        int rssi = WiFi.RSSI(i);
        bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        char row[64];
        snprintf(row, sizeof(row), "%s %.20s  %ddBm",
                 open ? LV_SYMBOL_WIFI : LV_SYMBOL_WARNING,
                 WiFi.SSID(i).c_str(), rssi);
        lv_obj_t *btn = lv_list_add_btn(s_wifi_list, NULL, row);
        lv_obj_set_style_bg_color(btn, THEME_COL_PANEL, LV_PART_MAIN);
        lv_obj_set_style_text_color(btn, rssi > -67 ? THEME_COL_OK : THEME_COL_TEXT, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, wifi_row_cb, LV_EVENT_CLICKED, nullptr);
    }
    if (s_wifi_status) {
        char b[24]; snprintf(b, sizeof(b), "%d networks", n);
        lv_label_set_text(s_wifi_status, b);
        lv_obj_set_style_text_color(s_wifi_status, THEME_COL_ACCENT, LV_PART_MAIN);
    }
}

static void update_system_page(void)
{
    char b[48];
    if (s_sys_chip) {
        snprintf(b, sizeof(b), "%s  x%d core", ESP.getChipModel(), ESP.getChipCores());
        lv_label_set_text(s_sys_chip, b);
    }
    if (s_sys_cpu) {
        snprintf(b, sizeof(b), "CPU: %d MHz", ESP.getCpuFreqMHz());
        lv_label_set_text(s_sys_cpu, b);
    }
    uint32_t freeh = ESP.getFreeHeap();
    uint32_t total = ESP.getHeapSize();
    uint32_t freePct = total ? (freeh * 100 / total) : 0;
    if (s_sys_heapbar) {
        lv_bar_set_value(s_sys_heapbar, 100 - freePct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_sys_heapbar, heap_color(freePct), LV_PART_INDICATOR);
    }
    if (s_sys_heaplbl) {
        char fh[16], th[16]; fmt_bytes(fh, sizeof(fh), freeh); fmt_bytes(th, sizeof(th), total);
        snprintf(b, sizeof(b), "RAM: %s free / %s", fh, th);
        lv_label_set_text(s_sys_heaplbl, b);
    }
    if (s_sys_flash) {
        char fs[16]; fmt_bytes(fs, sizeof(fs), ESP.getFlashChipSize());
        char sk[16]; fmt_bytes(sk, sizeof(sk), ESP.getSketchSize());
        snprintf(b, sizeof(b), "Flash: %s  App: %s", fs, sk);
        lv_label_set_text(s_sys_flash, b);
    }
    if (s_sys_uptime) {
        char up[16]; fmt_uptime(up, sizeof(up));
        snprintf(b, sizeof(b), "Uptime: %s", up);
        lv_label_set_text(s_sys_uptime, b);
    }
    if (s_sys_temp) {
        snprintf(b, sizeof(b), "Temp: %.1f C", (double)temperatureRead());
        lv_label_set_text(s_sys_temp, b);
    }
}

static void update_home_page(void)
{
    char b[40];
    if (s_home_uptime) {
        char up[16]; fmt_uptime(up, sizeof(up));
        snprintf(b, sizeof(b), LV_SYMBOL_POWER " %s", up);
        lv_label_set_text(s_home_uptime, b);
    }
    if (s_home_heap) {
        char fh[16]; fmt_bytes(fh, sizeof(fh), ESP.getFreeHeap());
        snprintf(b, sizeof(b), LV_SYMBOL_SAVE " %s", fh);
        lv_label_set_text(s_home_heap, b);
    }
    wifi_reconnect_label();
}

static void live_update_cb(lv_timer_t *t)
{
    (void)t;
    // Finalise async WiFi scans regardless of current page.
    if (s_wifi_scanning) {
        int n = WiFi.scanComplete();
        if (n >= 0) { populate_wifi_results(n); s_wifi_scanning = false; }
        else if (n == WIFI_SCAN_FAILED) {
            if (s_wifi_status) lv_label_set_text(s_wifi_status, "scan failed");
            s_wifi_scanning = false;
        }
    }
    switch (s_cur_page) {
    case PAGE_HOME:   update_home_page();   break;
    case PAGE_SYSTEM: update_system_page(); break;
    default: break;
    }
}

// ---------------------------------------------------------------------------
//  Page builders
// ---------------------------------------------------------------------------
static lv_obj_t *make_page(void)
{
    lv_obj_t *p = lv_obj_create(s_scr);
    lv_obj_remove_style_all(p);
    theme_apply_screen(p);
    lv_obj_set_size(p, 320, CONTENT_H);
    lv_obj_set_pos(p, 0, CONTENT_Y);
    lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

static lv_obj_t *make_tile(lv_obj_t *parent, const char *sym, const char *text,
                           int target, int x, int y, int w, int h)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_remove_style_all(b);
    theme_apply_button(b);
    lv_obj_set_size(b, w, h);
    lv_obj_set_pos(b, x, y);
    lv_obj_add_event_cb(b, nav_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)target);

    lv_obj_t *icon = lv_label_create(b);
    lv_label_set_text(icon, sym);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -18);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, 18);
    return b;
}

static void build_home(void)
{
    lv_obj_t *p = make_page();
    s_pages[PAGE_HOME] = p;

    // Live stats strip.
    lv_obj_t *strip = lv_obj_create(p);
    lv_obj_remove_style_all(strip);
    theme_apply_panel(strip);
    lv_obj_set_size(strip, 296, 32);
    lv_obj_align(strip, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_clear_flag(strip, LV_OBJ_FLAG_SCROLLABLE);

    s_home_uptime = lv_label_create(strip);
    lv_label_set_text(s_home_uptime, LV_SYMBOL_POWER " 00:00:00");
    lv_obj_set_style_text_color(s_home_uptime, THEME_COL_TEXT, LV_PART_MAIN);
    lv_obj_align(s_home_uptime, LV_ALIGN_LEFT_MID, 2, 0);

    s_home_heap = lv_label_create(strip);
    lv_label_set_text(s_home_heap, LV_SYMBOL_SAVE " --");
    lv_obj_set_style_text_color(s_home_heap, THEME_COL_TEXT, LV_PART_MAIN);
    lv_obj_align(s_home_heap, LV_ALIGN_CENTER, 0, 0);

    s_home_wifi = lv_label_create(strip);
    lv_label_set_text(s_home_wifi, LV_SYMBOL_WIFI " idle");
    lv_obj_set_style_text_color(s_home_wifi, THEME_COL_DIM, LV_PART_MAIN);
    lv_obj_align(s_home_wifi, LV_ALIGN_RIGHT_MID, -2, 0);

    // 2x2 tiles.
    const int margin = 12, gap = 12, top = 46;
    const int bw = (320 - 2 * margin - gap) / 2;
    const int bh = (CONTENT_H - top - margin - gap) / 2;
    int x0 = margin, y0 = top;
    make_tile(p, LV_SYMBOL_USB,      "USB",      PAGE_TERMINAL, x0,            y0,            bw, bh);
    make_tile(p, LV_SYMBOL_WIFI,     "WiFi",     PAGE_WIFI,     x0 + bw + gap, y0,            bw, bh);
    make_tile(p, LV_SYMBOL_SETTINGS, "System",   PAGE_SYSTEM,   x0,            y0 + bh + gap, bw, bh);
    make_tile(p, LV_SYMBOL_LIST,     "Settings", PAGE_SETTINGS, x0 + bw + gap, y0 + bh + gap, bw, bh);
}

static void build_terminal(void)
{
    lv_obj_t *p = make_page();
    s_pages[PAGE_TERMINAL] = p;

    s_stream_ta = lv_textarea_create(p);
    lv_obj_remove_style_all(s_stream_ta);
    theme_apply_panel(s_stream_ta);
    lv_obj_set_size(s_stream_ta, 312, CONTENT_H - 44);
    lv_obj_align(s_stream_ta, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(s_stream_ta, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_stream_ta, THEME_COL_OK, LV_PART_MAIN);
    lv_textarea_set_cursor_click_pos(s_stream_ta, false);
    lv_textarea_set_text(s_stream_ta,
        "CONTROL TERMINAL v1.0\n"
        "[ok] display ST7796 320x480\n"
        "[ok] touch XPT2046\n"
        "[ok] LVGL 8 / DMA\n"
        "[..] awaiting host link\n");
    lv_obj_add_event_cb(s_stream_ta, term_ta_event_cb, LV_EVENT_CLICKED, nullptr);

    s_term_input = lv_textarea_create(p);
    lv_obj_set_size(s_term_input, 312, 34);
    lv_obj_align(s_term_input, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_textarea_set_one_line(s_term_input, true);
    lv_textarea_set_placeholder_text(s_term_input, "Type command...");
    lv_obj_add_event_cb(s_term_input, term_input_cb, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(s_term_input, term_input_focus_cb, LV_EVENT_FOCUSED, nullptr);

    s_kb = lv_keyboard_create(s_scr);
    lv_obj_set_size(s_kb, 320, 160);
    lv_keyboard_set_textarea(s_kb, s_term_input);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_kb, term_kb_event_cb, LV_EVENT_ALL, nullptr);
}

static void build_wifi(void)
{
    lv_obj_t *p = make_page();
    s_pages[PAGE_WIFI] = p;

    lv_obj_t *scan = lv_btn_create(p);
    lv_obj_remove_style_all(scan);
    theme_apply_button(scan);
    lv_obj_set_size(scan, 110, 34);
    lv_obj_align(scan, LV_ALIGN_TOP_LEFT, 6, 4);
    lv_obj_add_event_cb(scan, wifi_scan_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *sl = lv_label_create(scan);
    lv_label_set_text(sl, LV_SYMBOL_REFRESH " SCAN");
    lv_obj_center(sl);

    s_wifi_status = lv_label_create(p);
    lv_label_set_text(s_wifi_status, "tap SCAN");
    lv_obj_set_style_text_color(s_wifi_status, THEME_COL_DIM, LV_PART_MAIN);
    lv_obj_align(s_wifi_status, LV_ALIGN_TOP_RIGHT, -6, 12);

    s_wifi_list = lv_list_create(p);
    lv_obj_remove_style_all(s_wifi_list);
    theme_apply_panel(s_wifi_list);
    lv_obj_set_size(s_wifi_list, 308, CONTENT_H - 50);
    lv_obj_align(s_wifi_list, LV_ALIGN_BOTTOM_MID, 0, -2);
}

static lv_obj_t *sys_row(lv_obj_t *parent, int y)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_add_style(l, &style_text, LV_PART_MAIN);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(l, LV_ALIGN_TOP_LEFT, 12, y);
    lv_label_set_text(l, "...");
    return l;
}

static void build_system(void)
{
    lv_obj_t *p = make_page();
    s_pages[PAGE_SYSTEM] = p;

    int y = 8;
    s_sys_chip   = sys_row(p, y); y += 26;
    s_sys_cpu    = sys_row(p, y); y += 26;
    s_sys_heaplbl = sys_row(p, y); y += 22;

    s_sys_heapbar = lv_bar_create(p);
    lv_obj_set_size(s_sys_heapbar, 296, 12);
    lv_obj_align(s_sys_heapbar, LV_ALIGN_TOP_MID, 0, y); y += 24;
    lv_obj_set_style_bg_color(s_sys_heapbar, THEME_COL_PANEL, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_sys_heapbar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_sys_heapbar, THEME_COL_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_sys_heapbar, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_sys_heapbar, THEME_COL_OK, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_sys_heapbar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(s_sys_heapbar, 0, 100);

    s_sys_flash  = sys_row(p, y); y += 26;
    s_sys_uptime = sys_row(p, y); y += 26;
    s_sys_temp   = sys_row(p, y); y += 30;

    // OTA progress section (driven by Pi push).
    lv_obj_t *otat = lv_label_create(p);
    lv_obj_add_style(otat, &style_accent, LV_PART_MAIN);
    lv_obj_set_style_text_font(otat, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_label_set_text(otat, "OTA UPDATE");
    lv_obj_align(otat, LV_ALIGN_TOP_LEFT, 12, y); y += 24;

    s_ota_bar = lv_bar_create(p);
    lv_obj_set_size(s_ota_bar, 296, 18);
    lv_obj_align(s_ota_bar, LV_ALIGN_TOP_MID, 0, y); y += 22;
    lv_obj_set_style_bg_color(s_ota_bar, THEME_COL_PANEL, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_ota_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_ota_bar, THEME_COL_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_ota_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ota_bar, THEME_COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_ota_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(s_ota_bar, 0, 100);
    lv_bar_set_value(s_ota_bar, 0, LV_ANIM_OFF);

    s_ota_lbl = lv_label_create(p);
    lv_obj_add_style(s_ota_lbl, &style_text, LV_PART_MAIN);
    lv_label_set_text(s_ota_lbl, "idle");
    lv_obj_align(s_ota_lbl, LV_ALIGN_TOP_LEFT, 12, y);

    update_system_page();
}

static void build_settings(void)
{
    lv_obj_t *p = make_page();
    s_pages[PAGE_SETTINGS] = p;

    // Brightness.
    lv_obj_t *bl = lv_label_create(p);
    lv_obj_add_style(bl, &style_text, LV_PART_MAIN);
    lv_label_set_text(bl, LV_SYMBOL_EYE_OPEN " Backlight");
    lv_obj_align(bl, LV_ALIGN_TOP_LEFT, 12, 14);

    s_set_bright = lv_slider_create(p);
    lv_obj_set_size(s_set_bright, 280, 14);
    lv_obj_align(s_set_bright, LV_ALIGN_TOP_MID, 0, 44);
    lv_slider_set_range(s_set_bright, 5, 100);
    lv_slider_set_value(s_set_bright, hal_display_get_brightness(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_set_bright, THEME_COL_PANEL, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_set_bright, THEME_COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_set_bright, THEME_COL_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(s_set_bright, bright_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Link obfuscation toggle.
    lv_obj_t *ol = lv_label_create(p);
    lv_obj_add_style(ol, &style_text, LV_PART_MAIN);
    lv_label_set_text(ol, LV_SYMBOL_SHUFFLE " Link obfuscation");
    lv_obj_align(ol, LV_ALIGN_TOP_LEFT, 12, 92);

    lv_obj_t *sw = lv_switch_create(p);
    lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -12, 86);
    lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, THEME_COL_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, obf_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Firmware version.
    lv_obj_t *ver = lv_label_create(p);
    lv_obj_set_style_text_color(ver, THEME_COL_DIM, LV_PART_MAIN);
    lv_label_set_text(ver, "firmware v1.0.0  /  LVGL 8 / TFT_eSPI");
    lv_obj_align(ver, LV_ALIGN_TOP_LEFT, 12, 140);

    // Reboot.
    lv_obj_t *rb = lv_btn_create(p);
    lv_obj_remove_style_all(rb);
    theme_apply_button(rb);
    lv_obj_set_size(rb, 160, 44);
    lv_obj_align(rb, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_border_color(rb, THEME_COL_ERR, LV_PART_MAIN);
    lv_obj_set_style_text_color(rb, THEME_COL_ERR, LV_PART_MAIN);
    lv_obj_add_event_cb(rb, reboot_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *rl = lv_label_create(rb);
    lv_label_set_text(rl, LV_SYMBOL_POWER " REBOOT");
    lv_obj_center(rl);
}

static void build_mirror(void)
{
    lv_obj_t *p = make_page();
    s_pages[PAGE_MIRROR] = p;

    lv_obj_t *lbl = lv_label_create(p);
    lv_label_set_text(lbl, "MIRROR: terminal mode");
    lv_obj_add_style(lbl, &style_text, LV_PART_MAIN);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 2);

    size_t buf_sz = (size_t)CANVAS_W * CANVAS_H * sizeof(lv_color_t);
    s_canvas_buf = (lv_color_t *)heap_caps_malloc(buf_sz, MALLOC_CAP_8BIT);
    if (s_canvas_buf) {
        s_mirror_canvas = lv_canvas_create(p);
        lv_canvas_set_buffer(s_mirror_canvas, s_canvas_buf, CANVAS_W, CANVAS_H, LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(s_mirror_canvas, LV_ALIGN_CENTER, 0, 12);
        lv_canvas_fill_bg(s_mirror_canvas, THEME_COL_PANEL, LV_OPA_COVER);
    }
}

static void build_devices(void)
{
    lv_obj_t *p = make_page();
    s_pages[PAGE_DEVICES] = p;

    lv_obj_t *t = lv_label_create(p);
    lv_label_set_text(t, "DEVICES");
    lv_obj_add_style(t, &style_title, LV_PART_MAIN);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *refresh = lv_btn_create(p);
    lv_obj_remove_style_all(refresh);
    theme_apply_button(refresh);
    lv_obj_set_size(refresh, 100, 30);
    lv_obj_align(refresh, LV_ALIGN_TOP_RIGHT, -6, 6);
    lv_obj_add_event_cb(refresh, dev_refresh_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *rl = lv_label_create(refresh);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH " SCAN");
    lv_obj_center(rl);

    s_dev_active = lv_label_create(p);
    lv_label_set_text(s_dev_active, "active: 0x01");
    lv_obj_add_style(s_dev_active, &style_accent, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_dev_active, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(s_dev_active, LV_ALIGN_TOP_MID, 0, 36);

    s_dev_list = lv_list_create(p);
    lv_obj_remove_style_all(s_dev_list);
    theme_apply_panel(s_dev_list);
    lv_obj_set_size(s_dev_list, 308, CONTENT_H - 80);
    lv_obj_align(s_dev_list, LV_ALIGN_BOTTOM_MID, 0, -4);
}

static void build_chrome(void)
{
    lv_obj_t *hdr = lv_obj_create(s_scr);
    lv_obj_remove_style_all(hdr);
    theme_apply_panel(hdr);
    lv_obj_set_size(hdr, 320, 38);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    s_back_btn = lv_btn_create(hdr);
    lv_obj_remove_style_all(s_back_btn);
    theme_apply_button(s_back_btn);
    lv_obj_set_size(s_back_btn, 34, 28);
    lv_obj_align(s_back_btn, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_add_event_cb(s_back_btn, back_btn_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *back_lbl = lv_label_create(s_back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
    lv_obj_center(back_lbl);
    lv_obj_add_flag(s_back_btn, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, "CONTROL TERMINAL");
    lv_obj_add_style(title, &style_title, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 42, 0);

    s_page_lbl = lv_label_create(hdr);
    lv_label_set_text(s_page_lbl, PAGE_TITLES[PAGE_HOME]);
    lv_obj_set_style_text_color(s_page_lbl, THEME_COL_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_page_lbl, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(s_page_lbl, LV_ALIGN_RIGHT_MID, -4, 0);

    // HUD.
    lv_obj_t *hud = lv_obj_create(s_scr);
    lv_obj_remove_style_all(hud);
    theme_apply_panel(hud);
    lv_obj_set_size(hud, 320, 40);
    lv_obj_set_pos(hud, 0, 440);
    lv_obj_clear_flag(hud, LV_OBJ_FLAG_SCROLLABLE);

    s_hud_dev = lv_label_create(hud);
    lv_label_set_text(s_hud_dev, "DEV 0x01");
    lv_obj_set_style_text_color(s_hud_dev, THEME_COL_ACCENT, LV_PART_MAIN);
    lv_obj_align(s_hud_dev, LV_ALIGN_LEFT_MID, 4, 0);

    s_hud_status = lv_label_create(hud);
    lv_label_set_text(s_hud_status, "READY");
    lv_obj_set_style_text_color(s_hud_status, THEME_COL_OK, LV_PART_MAIN);
    lv_obj_align(s_hud_status, LV_ALIGN_CENTER, 0, 0);

    s_hud_cpu = lv_label_create(hud);
    lv_label_set_text(s_hud_cpu, "CPU 0%");
    lv_obj_set_style_text_color(s_hud_cpu, THEME_COL_TEXT, LV_PART_MAIN);
    lv_obj_align(s_hud_cpu, LV_ALIGN_RIGHT_MID, -4, 0);
}

// ---------------------------------------------------------------------------
//  App-facing accessors
// ---------------------------------------------------------------------------
lv_obj_t *ui_stream_textarea(void) { return s_stream_ta; }
lv_obj_t *ui_mirror_canvas(void)   { return s_mirror_canvas; }

void ui_ota_set_progress(int pct, const char *status)
{
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    if (s_ota_bar) lv_bar_set_value(s_ota_bar, pct, LV_ANIM_OFF);
    if (s_ota_lbl && status) lv_label_set_text(s_ota_lbl, status);
}

void ui_devices_clear(void) { if (s_dev_list) lv_obj_clean(s_dev_list); }

void ui_devices_add(uint8_t id, bool active, uint32_t age_sec)
{
    if (!s_dev_list) return;
    char row[64];
    snprintf(row, sizeof(row), "0x%02X  %s  %lus ago", id, active ? "ACTIVE" : "up", (unsigned long)age_sec);
    lv_obj_t *btn = lv_list_add_btn(s_dev_list, active ? LV_SYMBOL_USB : NULL, row);
    lv_obj_set_style_bg_color(btn, active ? THEME_COL_ACCENT : THEME_COL_PANEL, LV_PART_MAIN);
    if (active) lv_obj_set_style_bg_opa(btn, LV_OPA_40, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, dev_row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)id);
}

void ui_set_active_device_label(uint8_t id)
{
    char buf[24];
    if (s_dev_active) { snprintf(buf, sizeof(buf), "active: 0x%02X", id); lv_label_set_text(s_dev_active, buf); }
    if (s_hud_dev)    { snprintf(buf, sizeof(buf), "DEV 0x%02X", id);    lv_label_set_text(s_hud_dev, buf); }
}

// ---------------------------------------------------------------------------
//  Init
// ---------------------------------------------------------------------------
void ui_init(void)
{
    s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();

    // WiFi in station mode (not connected) so scanning works on demand.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    theme_init();

    s_scr = lv_scr_act();
    lv_obj_remove_style_all(s_scr);
    theme_apply_screen(s_scr);
    lv_obj_clear_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);

    build_chrome();
    build_home();
    build_terminal();
    build_wifi();
    build_system();
    build_settings();
    build_mirror();
    build_devices();

    gestures_init(s_scr);

    lv_timer_create(live_update_cb, 1000, nullptr);

    ui_show_page(PAGE_HOME);
}
