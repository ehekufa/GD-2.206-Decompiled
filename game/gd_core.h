/*
 * Geometry Dash for Derka — ядро игры.
 * Чистый C99, ноль зависимостей: ни Android, ни OpenGL, ни libc-заголовков.
 * Собирается и проверяется на хосте:  gcc -std=c99 -DGD_SIM game/gd_core.c -lm
 *
 * Здесь всё, что можно проверить без экрана: каталог объектов, физика 8 режимов,
 * орбы/пады/порталы, триггеры, модель редактора уровней, аккаунты, сохранения.
 */
#ifndef GD_CORE_H
#define GD_CORE_H

#define GD_MAX_OBJ  2048   /* объектов в уровне */
#define GD_MAX_LVL    12   /* уровней в списке */
#define GD_MAX_PART   64   /* частиц (осколки, искры) */
#define GD_MAX_ACT   256   /* активных действий триггеров */
#define GD_MAX_ACC     8   /* локальных аккаунтов */
#define GD_MAX_SEL   256   /* выделенных объектов в редакторе */
#define GD_MAX_UNDO    3   /* глубина undo */
#define GD_MAX_ITEM   32   /* счётчиков (Count/Pickup) */
#define GD_MAX_CP     16   /* чекпоинтов практики */
#define GD_STR        20   /* длина имени/названия */
#define GD_CH          8   /* цветовых каналов (BG G1 G2 LINE OBJ P1 P2 3DL) */
#define GD_STORE    8192   /* размер буфера сохранений */

/* ---- вкладки редактора. Нумерация как в Geometry Dash Wiki: Блоки=1 … Триггеры=13 */
enum { GD_TAB_BLOCK, GD_TAB_PLAT, GD_TAB_OUTLINE, GD_TAB_SLOPE, GD_TAB_SPIKE,
       GD_TAB_DECO, GD_TAB_ANIM, GD_TAB_ORB, GD_TAB_PAD, GD_TAB_PORTAL,
       GD_TAB_SAW, GD_TAB_SPECIAL, GD_TAB_TRIGGER, GD_TAB_COUNT };

/* ---- каталог объектов: ID совпадают с номерами вкладок-диапазонов */
enum {
  GD_BLOCK = 1, GD_SLAB, GD_PILLAR, GD_BRICK, GD_BLACKBLK,          /*  1-5  */
  GD_PLAT = 10, GD_PLAT3,                                           /* 10-11 */
  GD_OUTLINE = 20, GD_OUTSLAB,                                      /* 20-21 */
  GD_SLOPE_UP = 30, GD_SLOPE_DN,                                    /* 30-31 */
  GD_SPIKE = 40, GD_SPIKE_S,                                        /* 40-41 */
  GD_DECO_COL = 50, GD_DECO_ARROW, GD_DECO_CHAIN, GD_DECO_GLOW,     /* 50-53 */
  GD_MONSTER = 60, GD_MONSTER_S,                                    /* 60-61 */
  GD_ORB_Y = 70, GD_ORB_P, GD_ORB_R, GD_ORB_B, GD_ORB_G, GD_ORB_K,
  GD_ORB_DASH, GD_ORB_DASHP, GD_ORB_SPIDER, GD_ORB_TOGGLE,          /* 70-79 */
  GD_PAD_Y = 80, GD_PAD_P, GD_PAD_R, GD_PAD_B, GD_PAD_DASH,         /* 80-84 */
  GD_P_CUBE = 90, GD_P_SHIP, GD_P_BALL, GD_P_UFO, GD_P_WAVE,
  GD_P_ROBOT, GD_P_SPIDER, GD_P_SWING,                              /* 90-97 */
  GD_P_GRAVD = 98, GD_P_GRAVU,                                      /* 98-99 */
  GD_P_S0 = 100, GD_P_S1, GD_P_S2, GD_P_S3, GD_P_S4, GD_P_S5,       /*100-105*/
  GD_P_MINI = 106, GD_P_BIG, GD_P_DUAL, GD_P_NODUAL,
  GD_P_MIRROR, GD_P_NOMIRROR, GD_P_TPA, GD_P_TPB,                   /*106-113*/
  GD_SAW_S = 120, GD_SAW_M, GD_SAW_L,                               /*120-122*/
  GD_COIN = 130, GD_STARTPOS, GD_CHECKPOINT,                        /*130-132*/
  GD_LBL_D = 140, GD_LBL_J, GD_LBL_S, GD_LBL_H, GD_LBL_F,           /*140-144*/
  /* триггеры (в GD их 131 тип, здесь рабочий набор из 29) */
  GD_T_MOVE = 1000, GD_T_ROTATE, GD_T_SCALE, GD_T_ALPHA, GD_T_COLOR,
  GD_T_PULSE, GD_T_SPAWN, GD_T_TOUCH, GD_T_COUNT, GD_T_INSTCOUNT,
  GD_T_COLLIDE, GD_T_TOGGLE, GD_T_STOP, GD_T_FOLLOW, GD_T_FOLLOWY,
  GD_T_KILL, GD_T_END, GD_T_SHAKE, GD_T_FLASH, GD_T_CAMOFF,
  GD_T_ZOOM, GD_T_PARTICLE, GD_T_PICKUP, GD_T_MOVETGT, GD_T_SHOW,
  GD_T_HIDE, GD_T_SEQ, GD_T_RANDOM, GD_T_N
};

/* игровые режимы */
enum { GD_CUBE, GD_SHIP, GD_BALL, GD_UFO, GD_WAVE, GD_ROBOT, GD_SPIDER,
       GD_SWING, GD_MODE_N };
/* экраны меню */
enum { GD_SCR_MENU, GD_SCR_LEVELS, GD_SCR_PAGE, GD_SCR_GAME, GD_SCR_EDITOR,
       GD_SCR_GARAGE, GD_SCR_ACCOUNT, GD_SCR_SETTINGS, GD_SCR_DONE, GD_SCR_N };
/* фазы игры */
enum { GD_PLAY, GD_DEAD, GD_WIN };
/* инструменты редактора */
enum { GD_TOOL_BUILD, GD_TOOL_EDIT, GD_TOOL_DEL, GD_TOOL_COLOR };
/* easing (порядок как в списке Move-триггера GD) */
enum { GD_EASE_NONE, GD_EASE_INOUT, GD_EASE_IN, GD_EASE_OUT, GD_EASE_ELASTIC,
       GD_EASE_BOUNCE, GD_EASE_EXP, GD_EASE_SINE, GD_EASE_BACK, GD_EASE_N };

typedef struct {
  float x, y;            /* левый нижний угол в блоках */
  float w, h;            /* размер; у пилы w=h=диаметр */
  float rot, sx, sy;     /* поворот (град), масштаб */
  float alpha;           /* прозрачность 0..1 (Alpha trigger) */
  float ox, oy;          /* смещение от триггеров (Move/Follow) */
  float a[8];            /* параметры: триггеры, cid, link, текст */
  unsigned short id;
  unsigned char tab, ngrp, groups[4];
  unsigned char visible, used, layer;
} GDObj;

typedef struct { float x, y, vx, vy, r, life, cr, cg, cb; } GDPart;

/* активное действие триггера */
typedef struct {
  unsigned short id;     /* GD_T_* */
  unsigned short grp;    /* группа-цель */
  float t, dur, prev;    /* время, длительность, прошлое значение easing */
  float v[8];            /* параметры (дельта/цель/цвет) */
  unsigned char ease, alive;
} GDAct;

typedef struct {
  char name[GD_STR], author[GD_STR];
  int  diff, stars, coins, official, plays, best;
  int  startMode, startSpeed, startMini, startDual, startMirror;
  float col[GD_CH][3];
  float len;
  int  nobj;
  GDObj o[GD_MAX_OBJ];
} GDLevel;

typedef struct { float x, y, vy; int mode, grav; } GDCp;   /* чекпоинт практики */

typedef struct {
  float x, y, vx, vy, rot, holdT;
  int mode, grav, mini, grounded, dashT, dashA, dead, jumps;
} GDPlayer;

typedef struct {
  char name[GD_STR];
  unsigned long long hash;
  int stars, orbs, demons, coins, diamonds, icon[GD_MODE_N], c1, c2, glow;
} GDAcc;

typedef struct {
  GDLevel *L;                 /* активный уровень (LV[cur]) */
  int cur, nlv;

  GDPlayer p[2]; int np;      /* дуал = 2 игрока */
  GDPart part[GD_MAX_PART]; int npart;
  GDAct act[GD_MAX_ACT]; int nact;
  GDCp cp[GD_MAX_CP]; int ncp;
  int items[GD_MAX_ITEM];

  float camx, camy, zoom, camrot, shake, flash, accum, t, deadt;
  float fr, fg, fb;           /* цвет вспышки */
  int speed, mode, grav, mini, dual, mirror;
  int phase, hold, holdPrev, clickEdge, attempt, jumps, clicks, coinsGot;
  int practice, screen, tool, tab, selected[GD_MAX_SEL], nsel, selId;
  int killId;                 /* чем убило: для отладки и тестов */
  int curGroup, curObj, editIdx;
  int loggedIn, curAcc;
  unsigned rng;

  GDAcc acc[GD_MAX_ACC]; int nacc;
  int un, utop; int undo_n[GD_MAX_UNDO];
  float undo_len[GD_MAX_UNDO];
  char store[GD_STORE];
} GDGame;

/* ---- описание каталога (для меню редактора и симулятора) */
typedef struct {
  unsigned short id;
  const char *name, *en;
  unsigned char tab, kind;    /* kind: 0 solid 1 hazard 2 slope 3 saw 4 orb
                                 5 pad 6 portal 7 coin 8 deco 9 trigger
                                 10 special 11 letter */
  float w, h;
  float cr, cg, cb;
  const char *params;         /* подсказка по a[] для редактора */
} GDInfo;

extern const GDInfo GD_CAT[];
extern const int    GD_CAT_N;
const GDInfo *gd_info(int id);
const char   *gd_mode_name(int mode);
const char   *gd_mode_en(int mode);
const char   *gd_trig_name(int id);
float         gd_ease(int kind, float x, float rate);
float         gd_speed_mul(int idx);

/* ---- уровни и объекты */
int    gd_lvl_new(GDGame *g, const char *name, const char *author);
GDLevel *gd_level_ptr(GDGame *g, int i);
void   gd_lvl_delete(GDGame *g, int i);
void   gd_lvl_select(GDGame *g, int i);
int    gd_obj_add(GDGame *g, int id, float x, float y);
void   gd_obj_del(GDGame *g, int i);
void   gd_obj_group(GDGame *g, int i, int grp);
void   gd_obj_set(GDGame *g, int i, int slot, float v);
int    gd_obj_at(GDGame *g, float x, float y);
void   gd_lvl_finish(GDGame *g);           /* пересчитать len и сортировку */
int    gd_lvl_from_string(GDGame *g, int li, const char *s);
int    gd_lvl_to_string(GDGame *g, int li, char *buf, int cap);
void   gd_build_demo(GDGame *g);           /* демо-уровни со всем содержимым */

/* ---- редактор */
void gd_ed_init(GDGame *g, int li, int tool, int tab);
void gd_ed_place(GDGame *g, float x, float y);
void gd_ed_pick(GDGame *g, float x, float y, int add);
void gd_ed_box(GDGame *g, float x0, float y0, float x1, float y1);
void gd_ed_clear_sel(GDGame *g);
void gd_ed_move_sel(GDGame *g, float dx, float dy);
void gd_ed_rot_sel(GDGame *g, float deg);
void gd_ed_scale_sel(GDGame *g, float f);
void gd_ed_flip_sel(GDGame *g, int vertical);
void gd_ed_group_sel(GDGame *g, int grp);
void gd_ed_del_sel(GDGame *g);
void gd_ed_copy_sel(GDGame *g);
void gd_ed_paste(GDGame *g, float x, float y);
void gd_ed_undo(GDGame *g);
void gd_ed_push(GDGame *g);

/* ---- аккаунты и сохранения */
int  gd_acc_register(GDGame *g, const char *name, const char *pass, char *err);
int  gd_acc_login(GDGame *g, const char *name, const char *pass, char *err);
void gd_acc_logout(GDGame *g);
int  gd_store_save(GDGame *g, char *buf, int cap);
void gd_store_load(GDGame *g, const char *buf);

/* ---- игра */
void  gd_init(GDGame *g);
void  gd_start(GDGame *g, int li, int practice);
void  gd_respawn(GDGame *g);
void  gd_press(GDGame *g);
void  gd_release(GDGame *g);
void  gd_update(GDGame *g, float dt);
float gd_prog(const GDGame *g);
float gd_pulse(const GDGame *g, int ch);   /* сила пульса канала (Pulse trigger) */
void  gd_scr(GDGame *g, int scr);
void  gd_place_trigger(GDGame *g, int id);       /* быстрый вызов триггера (тесты) */

#endif
