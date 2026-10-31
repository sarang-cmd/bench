/* gint.h (host preview stub) - the subset of gint the Bench add-in uses.
 *
 * This header is NOT used by the calculator build: the real gint headers live
 * in the fxSDK.  tools/sim/ exists so the screens can be compiled and run on a
 * PC, which is how the screenshots in the README are produced and how the
 * layout is checked without hardware. */

#ifndef SIM_GINT_H
#define SIM_GINT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint16_t color_t;
typedef unsigned int sim_uint;

#define DWIDTH 396
#define DHEIGHT 224

#define C_RGB(r, g, b) (((r) << 11) | ((g) << 6) | (b))
#define C_BLACK C_RGB(0, 0, 0)
#define C_WHITE C_RGB(31, 63, 31)
#define C_NONE ((color_t)0xFFFF)

typedef struct { int dummy; } font_t;

/* ---- display ---- */
enum { DTEXT_LEFT = 0, DTEXT_CENTER = 1, DTEXT_RIGHT = 2,
       DTEXT_TOP = 0, DTEXT_MIDDLE = 1, DTEXT_BOTTOM = 2 };

void dclear(color_t color);
void drect(int x1, int y1, int x2, int y2, color_t color);
void dpixel(int x, int y, color_t color);
void dline(int x1, int y1, int x2, int y2, color_t color);
void dhline(int y, color_t color);
void dvline(int x, color_t color);
void dtext(int x, int y, int fg, char const *str);
void dtext_opt(int x, int y, int fg, int bg, int halign, int valign,
               char const *str, int size);
void dsize(char const *str, font_t const *font, int *w, int *h);
void dnsize(char const *str, int size, font_t const *font, int *w, int *h);
char const *drsize(char const *str, font_t const *font, int width, int *w);
font_t const *dfont(font_t const *font);
font_t const *dfont_default(void);
void dupdate(void);

/* ---- keys ---- */
typedef struct {
    sim_uint time : 16;
    sim_uint : 3;
    sim_uint mod : 1;
    sim_uint shift : 1;
    sim_uint alpha : 1;
    sim_uint type : 2;
    sim_uint key : 8;
} key_event_t;

enum { KEYEV_NONE = 0, KEYEV_DOWN = 1, KEYEV_UP = 2, KEYEV_HOLD = 3 };

enum {
    KEY_F1 = 0x91, KEY_F2 = 0x92, KEY_F3 = 0x93, KEY_F4 = 0x94,
    KEY_F5 = 0x95, KEY_F6 = 0x96,
    KEY_SHIFT = 0x81, KEY_OPTN = 0x82, KEY_VARS = 0x83, KEY_MENU = 0x84,
    KEY_LEFT = 0x85, KEY_UP = 0x86,
    KEY_ALPHA = 0x71, KEY_SQUARE = 0x72, KEY_POWER = 0x73, KEY_EXIT = 0x74,
    KEY_DOWN = 0x75, KEY_RIGHT = 0x76,
    KEY_XOT = 0x61, KEY_LOG = 0x62, KEY_LN = 0x63, KEY_SIN = 0x64,
    KEY_COS = 0x65, KEY_TAN = 0x66,
    KEY_FRAC = 0x51, KEY_FD = 0x52, KEY_LEFTP = 0x53, KEY_RIGHTP = 0x54,
    KEY_COMMA = 0x55, KEY_ARROW = 0x56,
    KEY_7 = 0x41, KEY_8 = 0x42, KEY_9 = 0x43, KEY_DEL = 0x44,
    KEY_4 = 0x31, KEY_5 = 0x32, KEY_6 = 0x33, KEY_MUL = 0x34, KEY_DIV = 0x35,
    KEY_1 = 0x21, KEY_2 = 0x22, KEY_3 = 0x23, KEY_ADD = 0x24, KEY_SUB = 0x25,
    KEY_0 = 0x11, KEY_DOT = 0x12, KEY_EXP = 0x13, KEY_NEG = 0x14,
    KEY_EXE = 0x15,
};

enum {
    GETKEY_MOD_SHIFT = 0x01,
    GETKEY_MOD_ALPHA = 0x02,
    GETKEY_BACKLIGHT = 0x04,
    GETKEY_MENU = 0x08,
    GETKEY_REP_ARROWS = 0x10,
    GETKEY_REP_ALL = 0x20,
    GETKEY_REP_PROFILE = 0x40,
    GETKEY_FEATURES = 0x80,
    GETKEY_NONE = 0x00,
    GETKEY_DEFAULT = 0xdf,
};

key_event_t getkey(void);
key_event_t getkey_opt(int options, volatile int *timeout);
key_event_t pollevent(void);
key_event_t waitevent(volatile int *timeout);
int keydown(int key);
int keydown_all(int first, ...);
int keydown_any(int first, ...);
void clearevents(void);

/* ---- timers ---- */
enum { TIMER_CONTINUE = 0, TIMER_STOP = 1 };
enum { TIMER_ANY = -1, TIMER_TMU = -2, TIMER_ETMU = -3,
       TIMER_Pphi_4 = 0x00, TIMER_Pphi_16 = 0x10, TIMER_Pphi_64 = 0x20,
       TIMER_Pphi_256 = 0x30 };

typedef union {
    void *p;
    int i;
    unsigned int u;
} gint_call_arg_t;

typedef struct {
    void *function;
    gint_call_arg_t args[4];
} gint_call_t;

typedef int (*gint_call_fn)(void *);

#define GINT_CALL(func, ...) \
    ((gint_call_t){ .function = (void *)func, .args = { __VA_ARGS__ } })

int gint_call(gint_call_t cb);
int timer_configure(int timer, uint64_t delay_us, gint_call_t callback);
void timer_start(int timer);
void timer_stop(int timer);
int timer_timeout(volatile void *arg);

/* ---- rtc ---- */
uint32_t rtc_ticks(void);

/* ---- heap ---- */
void *kmalloc(size_t size, char const *arena_name);
void *krealloc(void *ptr, size_t size);
void kfree(void *ptr);
void *kmalloc_max(size_t *size, char const *arena_name);

/* ---- files ---- */
#define BFile_File 1
#define BFile_Folder 5
#define BFile_ReadOnly 0x01
#define BFile_WriteOnly 0x02
#define BFile_ReadWrite 0x03
#define BFile_Share 0x80

struct BFile_FileInfo {
    uint16_t index;
    uint16_t type;
    uint32_t file_size;
    uint32_t data_size;
    uint32_t property;
    void *address;
};

int BFile_Create(uint16_t const *path, int type, int *size);
int BFile_Remove(uint16_t const *path);
int BFile_Rename(uint16_t const *oldpath, uint16_t const *newpath);
int BFile_Open(uint16_t const *path, int mode);
int BFile_Close(int fd);
int BFile_Size(int fd);
int BFile_Write(int fd, void const *data, int even_size);
int BFile_Read(int handle, void *data, int size, int whence);
int BFile_Seek(int fd, int offset);
int BFile_FindFirst(uint16_t const *pattern, int *shandle, uint16_t *foundfile,
                    struct BFile_FileInfo *fileinfo);
int BFile_FindNext(int shandle, uint16_t *foundfile,
                   struct BFile_FileInfo *fileinfo);
int BFile_FindClose(int shandle);

/* ---- world switch ---- */
int gint_world_switch(gint_call_t function);
void gint_osmenu(void);

#endif /* SIM_GINT_H */
