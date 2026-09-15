/*
 * Geometry Dash for Derka — ядро: каталог, физика 8 режимов, триггеры,
 * редактор уровней, аккаунты, сохранения.
 * Чистый C99. Один шаг = 1/240 c, поэтому поведение не зависит от FPS.
 *
 * Поведение сверено с Geometry Dash Wiki (Triggers, Objects, Portals) и с
 * прежней версией этого файла (240 Гц, normal speed 10.386 блока/с).
 */
#include "gd_core.h"
#include <math.h>

#define STEP     (1.0f / 240.0f)  /* физика GD тикает 240 раз в секунду */
#define NORMAL     10.386f        /* «normal speed» = 10.386 блока/с (311.58 u/s) */
#define GRAV       95.0f
#define JUMPV      20.5f          /* -> высота 2.21 блока, дальность 4.49 */
#define CEIL       12.0f          /* высота уровня: потолок для перевёрнутой гравитации */
#define UNITS      30.0f          /* 1 блок = 30 юнитов GD (Move trigger в юнитах) */
#define EPS        0.07f

/* ============================================================ каталог объектов */

const GDInfo GD_CAT[] = {
  /*  id            имя            en             вкладка      kind w    h    r    g    b   параметры a[] */
  { GD_BLOCK,     "Блок",        "Block",        GD_TAB_BLOCK,  0, 1,   1,   .26f,.34f,.62f, "" },
  { GD_SLAB,      "Плита",       "Slab",         GD_TAB_BLOCK,  0, 1,  .5f,  .26f,.34f,.62f, "" },
  { GD_PILLAR,    "Столб",       "Pillar",       GD_TAB_BLOCK,  0,.5f,  1,   .26f,.34f,.62f, "" },
  { GD_BRICK,     "Кирпич",      "Brick",        GD_TAB_BLOCK,  0, 1,   1,   .42f,.26f,.22f, "" },
  { GD_BLACKBLK,  "Чёрный блок", "Black block",  GD_TAB_BLOCK,  0, 1,   1,   .06f,.07f,.14f, "" },
  { GD_PLAT,      "Платформа",   "Platform",     GD_TAB_PLAT,   0, 1,  .5f,  .30f,.38f,.70f, "" },
  { GD_PLAT3,     "Платформа 3", "Platform 3",   GD_TAB_PLAT,   0, 3,  .5f,  .30f,.38f,.70f, "" },
  { GD_OUTLINE,   "Обводка",     "Outline",      GD_TAB_OUTLINE,0, 1,   1,   .85f,.90f,1.0f, "" },
  { GD_OUTSLAB,   "Обводка-пл.", "Outline slab", GD_TAB_OUTLINE,0, 1,  .5f,  .85f,.90f,1.0f, "" },
  { GD_SLOPE_UP,  "Склон вверх", "Slope up",     GD_TAB_SLOPE,  2, 1,   1,   .30f,.38f,.70f, "" },
  { GD_SLOPE_DN,  "Склон вниз",  "Slope down",   GD_TAB_SLOPE,  2, 1,   1,   .30f,.38f,.70f, "" },
  { GD_SPIKE,     "Шип",         "Spike",        GD_TAB_SPIKE,  1, 1,   1,   .93f,.96f,1.0f, "" },
  { GD_SPIKE_S,   "Шип малый",   "Small spike",  GD_TAB_SPIKE,  1, 1,  .5f,  .93f,.96f,1.0f, "" },
  { GD_DECO_COL,  "Колонна",     "Pillar deco",  GD_TAB_DECO,   8, 1,   3,   .50f,.55f,.80f, "" },
  { GD_DECO_ARROW,"Стрелка",     "Arrow",        GD_TAB_DECO,   8, 1,   1,   .50f,.55f,.80f, "" },
  { GD_DECO_CHAIN,"Цепь",        "Chain",        GD_TAB_DECO,   8, 1,   3,   .50f,.55f,.80f, "" },
  { GD_DECO_GLOW, "Свечение",    "Glow",         GD_TAB_DECO,   8, 2,   2,   .60f,.70f,1.0f, "" },
  { GD_MONSTER,   "Монстр",      "Monster",      GD_TAB_ANIM,   1, 1,   1,   .80f,.30f,.35f, "" },
  { GD_MONSTER_S, "Монстр мал.", "Small monster",GD_TAB_ANIM,   1,.6f, .6f,  .80f,.30f,.35f, "" },
  { GD_ORB_Y,     "Жёлтый орб",  "Yellow orb",   GD_TAB_ORB,    4, 1,   1,   1.0f,.85f,.20f, "" },
  { GD_ORB_P,     "Розовый орб", "Pink orb",     GD_TAB_ORB,    4, 1,   1,   1.0f,.45f,.75f, "" },
  { GD_ORB_R,     "Красный орб", "Red orb",      GD_TAB_ORB,    4, 1,   1,   1.0f,.25f,.25f, "" },
  { GD_ORB_B,     "Синий орб",   "Blue orb",     GD_TAB_ORB,    4, 1,   1,   .25f,.55f,1.0f, "" },
  { GD_ORB_G,     "Зелёный орб", "Green orb",    GD_TAB_ORB,    4, 1,   1,   .30f,1.0f,.40f, "" },
  { GD_ORB_K,     "Чёрный орб",  "Black orb",    GD_TAB_ORB,    4, 1,   1,   .12f,.12f,.18f, "" },
  { GD_ORB_DASH,  "Дэш-орб",     "Dash orb",     GD_TAB_ORB,    4, 1,   1,   .30f,.90f,1.0f, "a0=угол" },
  { GD_ORB_DASHP, "Дэш розовый", "Pink dash",    GD_TAB_ORB,    4, 1,   1,   1.0f,.45f,.90f, "a0=угол" },
  { GD_ORB_SPIDER,"Паук-орб",    "Spider orb",   GD_TAB_ORB,    4, 1,   1,   .70f,.40f,1.0f, "" },
  { GD_ORB_TOGGLE,"Триггер-орб", "Toggle orb",   GD_TAB_ORB,    4, 1,   1,   .95f,.95f,.95f, "a0=группа" },
  { GD_PAD_Y,     "Жёлтый пад",  "Yellow pad",   GD_TAB_PAD,    5, 1,  .5f,  1.0f,.85f,.20f, "" },
  { GD_PAD_P,     "Розовый пад", "Pink pad",     GD_TAB_PAD,    5, 1,  .5f,  1.0f,.45f,.75f, "" },
  { GD_PAD_R,     "Красный пад", "Red pad",      GD_TAB_PAD,    5, 1,  .5f,  1.0f,.25f,.25f, "" },
  { GD_PAD_B,     "Синий пад",   "Blue pad",     GD_TAB_PAD,    5, 1,  .5f,  .25f,.55f,1.0f, "" },
  { GD_PAD_DASH,  "Дэш-пад",     "Dash pad",     GD_TAB_PAD,    5, 1,  .5f,  .30f,.90f,1.0f, "a0=угол" },
  { GD_P_CUBE,    "Портал куба", "Cube portal",  GD_TAB_PORTAL, 6, 1,   3,   .30f,1.0f,.45f, "" },
  { GD_P_SHIP,    "Портал кор.", "Ship portal",  GD_TAB_PORTAL, 6, 1,   3,   1.0f,.45f,.75f, "" },
  { GD_P_BALL,    "Портал шара", "Ball portal",  GD_TAB_PORTAL, 6, 1,   3,   1.0f,.30f,.30f, "" },
  { GD_P_UFO,     "Портал UFO",  "UFO portal",   GD_TAB_PORTAL, 6, 1,   3,   1.0f,.60f,.20f, "" },
  { GD_P_WAVE,    "Портал волны","Wave portal",  GD_TAB_PORTAL, 6, 1,   3,   .30f,.60f,1.0f, "" },
  { GD_P_ROBOT,   "Портал робота","Robot portal",GD_TAB_PORTAL, 6, 1,   3,   .95f,.95f,.95f, "" },
  { GD_P_SPIDER,  "Портал паука","Spider portal",GD_TAB_PORTAL, 6, 1,   3,   .70f,.40f,1.0f, "" },
  { GD_P_SWING,   "Портал свинга","Swing portal",GD_TAB_PORTAL, 6, 1,   3,   1.0f,.85f,.20f, "" },
  { GD_P_GRAVD,   "Гравитация ↓","Gravity down", GD_TAB_PORTAL, 6, 1,   3,   .25f,.55f,1.0f, "" },
  { GD_P_GRAVU,   "Гравитация ↑","Gravity up",   GD_TAB_PORTAL, 6, 1,   3,   1.0f,.85f,.20f, "" },
  { GD_P_S0,      "Скорость .5x","Speed 0.5x",   GD_TAB_PORTAL, 6, 1,   3,   .40f,.80f,1.0f, "" },
  { GD_P_S1,      "Скорость 1x", "Speed 1x",     GD_TAB_PORTAL, 6, 1,   3,   .30f,1.0f,.45f, "" },
  { GD_P_S2,      "Скорость 2x", "Speed 2x",     GD_TAB_PORTAL, 6, 1,   3,   1.0f,.85f,.20f, "" },
  { GD_P_S3,      "Скорость 3x", "Speed 3x",     GD_TAB_PORTAL, 6, 1,   3,   1.0f,.50f,.20f, "" },
  { GD_P_S4,      "Скорость 4x", "Speed 4x",     GD_TAB_PORTAL, 6, 1,   3,   1.0f,.25f,.35f, "" },
  { GD_P_S5,      "Скорость 5x", "Speed 5x",     GD_TAB_PORTAL, 6, 1,   3,   .70f,.20f,1.0f, "" },
  { GD_P_MINI,    "Мини-портал", "Mini portal",  GD_TAB_PORTAL, 6, 1,   2,   .30f,1.0f,.45f, "" },
  { GD_P_BIG,     "Обычный разм.","Normal size",  GD_TAB_PORTAL, 6, 1,   2,   .30f,.60f,1.0f, "" },
  { GD_P_DUAL,    "Дуал вкл",    "Dual on",      GD_TAB_PORTAL, 6, 1,   3,   .50f,.90f,1.0f, "" },
  { GD_P_NODUAL,  "Дуал выкл",   "Dual off",     GD_TAB_PORTAL, 6, 1,   3,   .30f,.45f,.70f, "" },
  { GD_P_MIRROR,  "Зеркало вкл", "Mirror on",    GD_TAB_PORTAL, 6, 1,   3,   1.0f,.60f,.20f, "" },
  { GD_P_NOMIRROR,"Зеркало выкл","Mirror off",   GD_TAB_PORTAL, 6, 1,   3,   .30f,.60f,1.0f, "" },
  { GD_P_TPA,     "Телепорт А",  "Teleport A",   GD_TAB_PORTAL, 6, 1,   2,   .30f,.60f,1.0f, "a0=канал" },
  { GD_P_TPB,     "Телепорт Б",  "Teleport B",   GD_TAB_PORTAL, 6, 1,   2,   1.0f,.60f,.20f, "a0=канал" },
  { GD_SAW_S,     "Пила малая",  "Small saw",    GD_TAB_SAW,    3, 1,   1,   .90f,.93f,1.0f, "" },
  { GD_SAW_M,     "Пила средняя","Medium saw",   GD_TAB_SAW,    3, 1.5f,1.5f,.90f,.93f,1.0f, "" },
  { GD_SAW_L,     "Пила большая","Big saw",      GD_TAB_SAW,    3, 2,   2,   .90f,.93f,1.0f, "" },
  { GD_COIN,      "Монета",      "Coin",         GD_TAB_SPECIAL,7, 1,   1,   1.0f,.85f,.20f, "" },
  { GD_STARTPOS,  "Старт",       "Start pos",    GD_TAB_SPECIAL,10,1,   1,   .30f,1.0f,.45f, "a0=режим a1=скорость" },
  { GD_CHECKPOINT,"Чекпоинт",    "Checkpoint",   GD_TAB_SPECIAL,10,1,   1,   .30f,.90f,1.0f, "" },
  { GD_LBL_D,     "Блок D",      "D block",      GD_TAB_SPECIAL,11,1,   1,   .50f,.55f,.80f, "волна не бьётся о платформы" },
  { GD_LBL_J,     "Блок J",      "J block",      GD_TAB_SPECIAL,11,1,   1,   .50f,.55f,.80f, "нет авто-прыжка при удержании" },
  { GD_LBL_S,     "Блок S",      "S block",      GD_TAB_SPECIAL,11,1,   1,   .50f,.55f,.80f, "гасит дэш-орбы" },
  { GD_LBL_H,     "Блок H",      "H block",      GD_TAB_SPECIAL,11,1,   1,   .50f,.55f,.80f, "нет урона сверху" },
  { GD_LBL_F,     "Блок F",      "F block",      GD_TAB_SPECIAL,11,1,   1,   .50f,.55f,.80f, "переворот гравитации о потолок" },
  /* триггеры */
  { GD_T_MOVE,    "Move",     "Move",     GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=X a2=Y a3=время a4=easing" },
  { GD_T_ROTATE,  "Rotate",   "Rotate",   GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=градус a2=время" },
  { GD_T_SCALE,   "Scale",    "Scale",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=sx a2=sy a3=время" },
  { GD_T_ALPHA,   "Alpha",    "Alpha",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=alpha a2=время" },
  { GD_T_COLOR,   "Color",    "Color",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=канал a1..a3=RGB a4=время" },
  { GD_T_PULSE,   "Pulse",    "Pulse",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=канал a1=время a2..a3=fade a4=сила" },
  { GD_T_SPAWN,   "Spawn",    "Spawn",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=задержка" },
  { GD_T_TOUCH,   "Touch",    "Touch",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=hold" },
  { GD_T_COUNT,   "Count",    "Count",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=item a1=цель a2=группа" },
  { GD_T_INSTCOUNT,"Inst.Count","Inst.Count",GD_TAB_TRIGGER,9, 1, 1, 1.0f,.30f,.80f, "a0=item a1=цель a2=группа" },
  { GD_T_COLLIDE, "Collision","Collision",GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=cid a1=группа a2=on-exit" },
  { GD_T_TOGGLE,  "Toggle",   "Toggle",   GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=вкл" },
  { GD_T_STOP,    "Stop",     "Stop",     GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа" },
  { GD_T_FOLLOW,  "Follow",   "Follow",   GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=лидер a2=скорость" },
  { GD_T_FOLLOWY, "Follow Y", "Follow Y", GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=скорость a2=смещение" },
  { GD_T_KILL,    "Kill",     "Kill",     GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "" },
  { GD_T_END,     "End",      "End",      GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "" },
  { GD_T_SHAKE,   "Shake",    "Shake",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=сила a1=интервал" },
  { GD_T_FLASH,   "Flash",    "Flash",    GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0..a2=RGB a3=время" },
  { GD_T_CAMOFF,  "Cam Offset","Cam Offset",GD_TAB_TRIGGER,9, 1, 1, 1.0f,.30f,.80f, "a0=X a1=Y a2=время" },
  { GD_T_ZOOM,    "Zoom",     "Zoom",     GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=zoom a1=время" },
  { GD_T_PARTICLE,"Particle", "Particle", GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=кол-во a1=скорость a2=жизнь" },
  { GD_T_PICKUP,  "Pickup",   "Pickup",   GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=item a1=кол-во" },
  { GD_T_MOVETGT, "Move Target","Move Target",GD_TAB_TRIGGER,9, 1, 1, 1.0f,.30f,.80f, "a0=группа a1=цель a2=скорость" },
  { GD_T_SHOW,    "Show",     "Show",     GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа" },
  { GD_T_HIDE,    "Hide",     "Hide",     GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0=группа" },
  { GD_T_SEQ,     "Sequence", "Sequence", GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0..a3=группы a4=шаг" },
  { GD_T_RANDOM,  "Random",   "Random",   GD_TAB_TRIGGER, 9, 1, 1, 1.0f,.30f,.80f, "a0..a3=группы" }
};
const int GD_CAT_N = (int)(sizeof GD_CAT / sizeof *GD_CAT);

const GDInfo *gd_info(int id)
{
  int i;
  for (i = 0; i < GD_CAT_N; i++)
    if (GD_CAT[i].id == (unsigned short)id) return &GD_CAT[i];
  return 0;
}

static const char *MODE_NAME[GD_MODE_N] =
  { "Куб", "Корабль", "Шар", "UFO", "Волна", "Робот", "Паук", "Свинг" };
static const char *MODE_EN[GD_MODE_N] =
  { "CUBE", "SHIP", "BALL", "UFO", "WAVE", "ROBOT", "SPIDER", "SWING" };
const char *gd_mode_name(int m) { return (m >= 0 && m < GD_MODE_N) ? MODE_NAME[m] : "?"; }
const char *gd_mode_en(int m)   { return (m >= 0 && m < GD_MODE_N) ? MODE_EN[m]   : "?"; }

const char *gd_trig_name(int id)
{
  const GDInfo *i = gd_info(id);
  return (i && i->kind == 9) ? i->name : "Триггер";
}

float gd_speed_mul(int idx)
{
  static const float M[6] = { 0.815f, 1.0f, 1.263f, 1.574f, 1.944f, 2.357f };
  return M[idx < 0 ? 1 : (idx > 5 ? 5 : idx)];
}

/* easing: список как в настройках Move-триггера GD */
float gd_ease(int kind, float x, float rate)
{
  float r = rate < 0.2f ? 0.2f : rate;
  if (x <= 0.0f) return 0.0f;
  if (x >= 1.0f) return 1.0f;
  switch (kind) {
    case GD_EASE_INOUT:  return x * x * (3.0f - 2.0f * x);
    case GD_EASE_IN:     return powf(x, r);
    case GD_EASE_OUT:    return 1.0f - powf(1.0f - x, r);
    case GD_EASE_ELASTIC: {
      float p = 0.35f;
      return powf(2.0f, -10.0f * x) * sinf((x - p / 4.0f) * 6.28318f / p) + 1.0f;
    }
    case GD_EASE_BOUNCE: {
      float n = 7.5625f, d = 2.75f;
      x = 1.0f - x;
      if (x < 1.0f / d)      return 1.0f - n * x * x;
      else if (x < 2.0f / d) { x -= 1.5f / d;  return 1.0f - (n * x * x + 0.75f); }
      else if (x < 2.5f / d) { x -= 2.25f / d; return 1.0f - (n * x * x + 0.9375f); }
      x -= 2.625f / d;       return 1.0f - (n * x * x + 0.984375f);
    }
    case GD_EASE_EXP:    return powf(2.0f, 10.0f * (x - 1.0f));
    case GD_EASE_SINE:   return 1.0f - cosf(x * 1.5707963f);
    case GD_EASE_BACK: {
      float s = 1.70158f;
      x -= 1.0f;
      return x * x * ((s + 1.0f) * x + s) + 1.0f;
    }
    default: return x;
  }
}

static unsigned rnd(GDGame *g)
{
  unsigned x = g->rng;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  return g->rng = x;
}

static float rf(GDGame *g, float a, float b)
{
  return a + (float)(rnd(g) % 1000) / 1000.0f * (b - a);
}

static int in_grp(const GDObj *o, int grp)
{
  int i;
  if (grp <= 0) return 0;
  for (i = 0; i < o->ngrp; i++) if (o->groups[i] == (unsigned char)grp) return 1;
  return 0;
}

/* ================================================================ уровни */

static GDLevel LV[GD_MAX_LVL];
static GDObj   UNDO[GD_MAX_UNDO][GD_MAX_OBJ];   /* снимки редактора */

int gd_lvl_new(GDGame *g, const char *name, const char *author)
{
  GDLevel *L;
  int i, c;
  if (g->nlv >= GD_MAX_LVL) return -1;
  L = &LV[g->nlv];
  for (i = 0; i < GD_STR - 1 && name[i]; i++)   L->name[i] = name[i];
  L->name[i] = 0;
  for (i = 0; i < GD_STR - 1 && author[i]; i++)  L->author[i] = author[i];
  L->author[i] = 0;
  L->nobj = 0; L->len = 0; L->best = 0; L->plays = 0;
  L->diff = 0; L->stars = 0; L->coins = 0; L->official = 0;
  L->startMode = GD_CUBE; L->startSpeed = 1; L->startMini = 0;
  L->startDual = 0; L->startMirror = 0;
  for (c = 0; c < GD_CH; c++) {
    L->col[c][0] = 0.06f; L->col[c][1] = 0.09f; L->col[c][2] = 0.24f;
  }
  L->col[0][0] = 0.05f; L->col[0][1] = 0.08f; L->col[0][2] = 0.22f;   /* BG  */
  L->col[1][0] = 0.10f; L->col[1][1] = 0.16f; L->col[1][2] = 0.40f;   /* G1  */
  L->col[2][0] = 0.08f; L->col[2][1] = 0.12f; L->col[2][2] = 0.32f;   /* G2  */
  L->col[3][0] = 0.35f; L->col[3][1] = 0.85f; L->col[3][2] = 1.00f;   /* LINE*/
  L->col[4][0] = 0.30f; L->col[4][1] = 0.40f; L->col[4][2] = 0.75f;   /* OBJ */
  L->col[5][0] = 0.20f; L->col[5][1] = 0.90f; L->col[5][2] = 0.40f;   /* P1  */
  L->col[6][0] = 0.30f; L->col[6][1] = 0.60f; L->col[6][2] = 1.00f;   /* P2  */
  L->col[7][0] = 0.15f; L->col[7][1] = 0.20f; L->col[7][2] = 0.45f;   /* 3DL */
  g->nlv++;
  return g->nlv - 1;
}

GDLevel *gd_level_ptr(GDGame *g, int i)
{
  return (i >= 0 && i < g->nlv) ? &LV[i] : 0;
}

void gd_lvl_delete(GDGame *g, int i)
{
  int k;
  if (i < 0 || i >= g->nlv) return;
  for (k = i; k + 1 < g->nlv; k++) LV[k] = LV[k + 1];
  g->nlv--;
  if (g->cur >= g->nlv) g->cur = g->nlv - 1;
  g->L = g->cur >= 0 ? &LV[g->cur] : 0;
}

void gd_lvl_select(GDGame *g, int i)
{
  if (i < 0 || i >= g->nlv) return;
  g->cur = i;
  g->L = &LV[i];
}

int gd_obj_add(GDGame *g, int id, float x, float y)
{
  const GDInfo *inf = gd_info(id);
  GDObj *o;
  int i;
  if (!inf || !g->L || g->L->nobj >= GD_MAX_OBJ) return -1;
  o = &g->L->o[g->L->nobj];
  o->x = x; o->y = y;
  o->w = inf->w; o->h = inf->h;
  o->rot = 0; o->sx = o->sy = 1.0f; o->alpha = 1.0f;
  o->ox = o->oy = 0;
  for (i = 0; i < 8; i++) o->a[i] = 0;
  o->id = (unsigned short)id;
  o->tab = inf->tab;
  o->ngrp = 0; o->visible = 1; o->used = 0; o->layer = 0;
  return g->L->nobj++;
}

void gd_obj_del(GDGame *g, int i)
{
  GDLevel *L = g->L;
  if (!L || i < 0 || i >= L->nobj) return;
  L->o[i] = L->o[L->nobj - 1];
  L->nobj--;
}

void gd_obj_group(GDGame *g, int i, int grp)
{
  GDObj *o;
  int k;
  if (!g->L || i < 0 || i >= g->L->nobj) return;
  o = &g->L->o[i];
  if (grp <= 0) { o->ngrp = 0; return; }
  for (k = 0; k < o->ngrp; k++) if (o->groups[k] == (unsigned char)grp) return;
  if (o->ngrp < 4) o->groups[o->ngrp++] = (unsigned char)grp;
}

void gd_obj_set(GDGame *g, int i, int slot, float v)
{
  if (!g->L || i < 0 || i >= g->L->nobj || slot < 0 || slot > 7) return;
  g->L->o[i].a[slot] = v;
}

int gd_obj_at(GDGame *g, float x, float y)
{
  GDLevel *L = g->L;
  int i, best = -1;
  if (!L) return -1;
  for (i = 0; i < L->nobj; i++) {
    GDObj *o = &L->o[i];
    if (x >= o->x && x <= o->x + o->w * o->sx &&
        y >= o->y && y <= o->y + o->h * o->sy) best = i;   /* верхний по списку */
  }
  return best;
}

void gd_lvl_finish(GDGame *g)
{
  GDLevel *L = g->L;
  int i, j;
  float maxx = 0;
  if (!L) return;
  /* сортировка вставками по X: физика обрывает перебор объектов за игроком */
  for (i = 1; i < L->nobj; i++) {
    GDObj key = L->o[i];
    for (j = i - 1; j >= 0 && L->o[j].x > key.x; j--) L->o[j + 1] = L->o[j];
    L->o[j + 1] = key;
  }
  for (i = 0; i < L->nobj; i++) {
    GDObj *o = &L->o[i];
    float r = o->x + o->w * o->sx;
    if (r > maxx) maxx = r;
    if (o->id == GD_COIN) L->coins++;
  }
  L->len = maxx + 16.0f;
}

/* ----------------------------------------------------------- строка уровня */
/* Свой компактный текстовый формат: ключи 1/2/3/6/57 совпадают с GD,
 * остальные — наши. Объекты через ';', поля через ','. */

static char *pf(char *b, const char *end, float v)
{
  char tmp[32];
  int n = 0, i;
  if (b >= end - 2) return b;
  if (v == (float)(int)v) {
    int iv = (int)v, k = 0;
    char d[16];
    if (iv < 0) { *b++ = '-'; iv = -iv; }
    do { d[k++] = (char)('0' + iv % 10); iv /= 10; } while (iv && k < 15);
    while (k) { tmp[n++] = d[--k]; }
  } else {
    int iv = (int)(v * 100.0f + (v >= 0 ? 0.5f : -0.5f));
    int k = 0, dot = 0;
    char d[24];
    if (iv < 0) { *b++ = '-'; iv = -iv; }
    do { d[k++] = (char)('0' + iv % 10); iv /= 10; if (k == 2 && !dot) { d[k++] = '.'; dot = 1; } }
    while (iv && k < 22);
    if (!dot) { d[k++] = '0'; d[k++] = '.'; }
    while (k) { tmp[n++] = d[--k]; }
  }
  tmp[n] = 0;
  for (i = 0; tmp[i] && b < end - 1; i++) *b++ = tmp[i];
  return b;
}

static char *ps(char *b, const char *end, const char *s)
{
  while (*s && b < end - 1) *b++ = *s++;
  return b;
}

int gd_lvl_to_string(GDGame *g, int li, char *buf, int cap)
{
  GDLevel *L;
  char *b = buf, *end = buf + cap;
  int i, k;
  if (li < 0 || li >= g->nlv || cap < 64) return 0;
  L = &LV[li];
  b = ps(b, end, "GD1;");
  b = ps(b, end, L->name); b = ps(b, end, ";");
  b = ps(b, end, L->author); b = ps(b, end, ";");
  b = pf(b, end, (float)L->diff);   b = ps(b, end, ";");
  b = pf(b, end, (float)L->stars);  b = ps(b, end, ";");
  b = pf(b, end, (float)L->startMode);  b = ps(b, end, ",");
  b = pf(b, end, (float)L->startSpeed); b = ps(b, end, ",");
  b = pf(b, end, (float)L->startMini);  b = ps(b, end, ",");
  b = pf(b, end, (float)L->startDual);  b = ps(b, end, ",");
  b = pf(b, end, (float)L->startMirror);b = ps(b, end, ";");
  for (k = 0; k < GD_CH; k++) {
    if (k) b = ps(b, end, ",");
    b = pf(b, end, (float)(int)(L->col[k][0] * 255.0f)); b = ps(b, end, ".");
    b = pf(b, end, (float)(int)(L->col[k][1] * 255.0f)); b = ps(b, end, ".");
    b = pf(b, end, (float)(int)(L->col[k][2] * 255.0f));
  }
  b = ps(b, end, ";");
  for (i = 0; i < L->nobj; i++) {
    GDObj *o = &L->o[i];
    if (i) b = ps(b, end, ";");
    b = ps(b, end, "1,"); b = pf(b, end, (float)o->id);
    b = ps(b, end, ",2,"); b = pf(b, end, o->x);
    b = ps(b, end, ",3,"); b = pf(b, end, o->y);
    if (o->rot)  { b = ps(b, end, ",6,");  b = pf(b, end, o->rot); }
    if (o->sx != 1.0f || o->sy != 1.0f) {
      b = ps(b, end, ",32,"); b = pf(b, end, o->sx);
      b = ps(b, end, ",33,"); b = pf(b, end, o->sy);
    }
    if (o->ngrp) {
      int j;
      b = ps(b, end, ",57,");
      for (j = 0; j < o->ngrp; j++) {
        if (j) b = ps(b, end, ".");
        b = pf(b, end, (float)o->groups[j]);
      }
    }
    for (k = 0; k < 8; k++) {
      if (o->a[k] != 0.0f) {
        b = ps(b, end, ",a");
        b = pf(b, end, (float)k);
        b = ps(b, end, ",");
        b = pf(b, end, o->a[k]);
      }
    }
  }
  *b = 0;
  return (int)(b - buf);
}

static float num(const char **s)
{
  const char *p = *s;
  float v = 0, sign = 1.0f;
  if (*p == '-') { sign = -1.0f; p++; }
  while (*p >= '0' && *p <= '9') v = v * 10.0f + (float)(*p++ - '0');
  if (*p == '.') {
    float d = 0.1f;
    p++;
    while (*p >= '0' && *p <= '9') { v += (float)(*p++ - '0') * d; d *= 0.1f; }
  }
  *s = p;
  return v * sign;
}

int gd_lvl_from_string(GDGame *g, int li, const char *s)
{
  const char *p = s;
  GDLevel *L;
  int i, k;
  if (li < 0 || li >= g->nlv) return 0;
  if (p[0] != 'G' || p[1] != 'D' || p[2] != '1') return 0;
  L = &LV[li];
  L->nobj = 0;
  p += 4;
  for (i = 0; i < GD_STR - 1 && *p && *p != ';'; i++) L->name[i] = *p++;
  L->name[i] = 0; if (*p == ';') p++;
  for (i = 0; i < GD_STR - 1 && *p && *p != ';'; i++) L->author[i] = *p++;
  L->author[i] = 0; if (*p == ';') p++;
  L->diff  = (int)num(&p); if (*p == ';') p++;
  L->stars = (int)num(&p); if (*p == ';') p++;
  L->startMode   = (int)num(&p); if (*p == ',') p++;
  L->startSpeed  = (int)num(&p); if (*p == ',') p++;
  L->startMini   = (int)num(&p); if (*p == ',') p++;
  L->startDual   = (int)num(&p); if (*p == ',') p++;
  L->startMirror = (int)num(&p); if (*p == ';') p++;
  for (k = 0; k < GD_CH; k++) {
    L->col[k][0] = num(&p) / 255.0f; if (*p == '.') p++;
    L->col[k][1] = num(&p) / 255.0f; if (*p == '.') p++;
    L->col[k][2] = num(&p) / 255.0f; if (*p == ',') p++;
  }
  if (*p == ';') p++;

  while (*p) {
    int id = -1, idx = -1, ngrp = 0, aslot = -1;
    float x = 0, y = 0, rot = 0, sx = 1, sy = 1;
    float a[8];
    unsigned char grp[4];
    for (k = 0; k < 8; k++) a[k] = 0;
    for (k = 0; k < 4; k++) grp[k] = 0;
    while (*p && *p != ';') {
      const char *key = p;
      int kn = 0;
      float v;
      while (*p && *p != ',' && *p != ';') { p++; kn++; }
      if (*p == ',') p++;
      if (kn == 2 && key[0] == 'a' && key[1] >= '0' && key[1] <= '7') {
        aslot = key[1] - '0';
        v = num(&p);
        a[aslot] = v;
      } else {
        v = num(&p);
        if (kn == 1 && key[0] == '1') id = (int)v;
        else if (kn == 1 && key[0] == '2') x = v;
        else if (kn == 1 && key[0] == '3') y = v;
        else if (kn == 1 && key[0] == '6') rot = v;
        else if (kn == 2 && key[0] == '3' && key[1] == '2') sx = v;
        else if (kn == 2 && key[0] == '3' && key[1] == '3') sy = v;
        else if (kn == 2 && key[0] == '5' && key[1] == '7') {
          /* группы через точку — разбираем отдельно */
          const char *q = p - 1;
          (void)q;
        }
      }
      if (*p == '.') {           /* список групп: 57,1.2.3 */
        if (ngrp < 4) grp[ngrp++] = (unsigned char)(int)v;
        p++;
        continue;
      }
      if (*p == ',') p++;
    }
    if (id > 0) {
      idx = gd_obj_add(g, id, x, y);
      if (idx >= 0) {
        GDObj *o = &L->o[idx];
        o->rot = rot; o->sx = sx; o->sy = sy;
        for (k = 0; k < 8; k++) o->a[k] = a[k];
        for (k = 0; k < ngrp; k++) gd_obj_group(g, idx, grp[k]);
      }
    }
    if (*p == ';') p++;
  }
  gd_lvl_finish(g);
  return 1;
}

/* ================================================================ редактор */

static GDObj CLIP[GD_MAX_OBJ];
static int   CLIP_N;

void gd_ed_init(GDGame *g, int li, int tool, int tab)
{
  gd_lvl_select(g, li);
  g->tool = tool; g->tab = tab;
  g->nsel = 0; g->selId = -1; g->editIdx = -1;
  g->un = 0;
}

void gd_ed_push(GDGame *g)
{
  int i;
  GDLevel *L = g->L;
  if (!L) return;
  g->utop = (g->utop + 1) % GD_MAX_UNDO;
  for (i = 0; i < L->nobj; i++) UNDO[g->utop][i] = L->o[i];
  g->undo_n[g->utop] = L->nobj;
  g->undo_len[g->utop] = L->len;
  if (g->un < GD_MAX_UNDO) g->un++;
}

void gd_ed_undo(GDGame *g)
{
  int i;
  GDLevel *L = g->L;
  if (!L || g->un <= 0) return;
  for (i = 0; i < g->undo_n[g->utop]; i++) L->o[i] = UNDO[g->utop][i];
  L->nobj = g->undo_n[g->utop];
  L->len = g->undo_len[g->utop];
  g->utop = (g->utop + GD_MAX_UNDO - 1) % GD_MAX_UNDO;
  g->un--;
  g->nsel = 0;
}

void gd_ed_place(GDGame *g, float x, float y)
{
  const GDInfo *inf = gd_info(g->curObj);
  int i;
  if (!inf || inf->kind == 9) return;          /* триггеры ставятся отдельно */
  gd_ed_push(g);
  /* привязка к сетке 1x1, как в GD при включённом snap */
  i = gd_obj_add(g, g->curObj, (float)(int)x, (float)(int)y);
  if (i >= 0 && g->curGroup > 0) gd_obj_group(g, i, g->curGroup);
  gd_lvl_finish(g);
}

void gd_ed_pick(GDGame *g, float x, float y, int add)
{
  int i = gd_obj_at(g, x, y);
  if (!add) g->nsel = 0;
  if (i < 0) return;
  if (g->nsel < GD_MAX_SEL) g->selected[g->nsel++] = i;
  g->selId = i;
  g->editIdx = i;
}

void gd_ed_box(GDGame *g, float x0, float y0, float x1, float y1)
{
  GDLevel *L = g->L;
  int i;
  float t;
  if (!L) return;
  if (x0 > x1) { t = x0; x0 = x1; x1 = t; }
  if (y0 > y1) { t = y0; y0 = y1; y1 = t; }
  g->nsel = 0;
  for (i = 0; i < L->nobj; i++) {
    GDObj *o = &L->o[i];
    if (o->x + o->w * o->sx >= x0 && o->x <= x1 &&
        o->y + o->h * o->sy >= y0 && o->y <= y1)
      if (g->nsel < GD_MAX_SEL) g->selected[g->nsel++] = i;
  }
}

void gd_ed_clear_sel(GDGame *g) { g->nsel = 0; g->selId = -1; }

void gd_ed_move_sel(GDGame *g, float dx, float dy)
{
  int k;
  if (!g->L || !g->nsel) return;
  gd_ed_push(g);
  for (k = 0; k < g->nsel; k++) {
    int i = g->selected[k];
    if (i >= 0 && i < g->L->nobj) { g->L->o[i].x += dx; g->L->o[i].y += dy; }
  }
  gd_lvl_finish(g);
}

void gd_ed_rot_sel(GDGame *g, float deg)
{
  int k;
  if (!g->L || !g->nsel) return;
  gd_ed_push(g);
  for (k = 0; k < g->nsel; k++) {
    int i = g->selected[k];
    if (i >= 0 && i < g->L->nobj) {
      g->L->o[i].rot += deg;
      while (g->L->o[i].rot >= 360.0f) g->L->o[i].rot -= 360.0f;
      while (g->L->o[i].rot < 0.0f)    g->L->o[i].rot += 360.0f;
    }
  }
}

void gd_ed_scale_sel(GDGame *g, float f)
{
  int k;
  if (!g->L || !g->nsel || f < 0.1f || f > 8.0f) return;
  gd_ed_push(g);
  for (k = 0; k < g->nsel; k++) {
    int i = g->selected[k];
    if (i >= 0 && i < g->L->nobj) {
      g->L->o[i].sx *= f;
      g->L->o[i].sy *= f;
    }
  }
}

void gd_ed_flip_sel(GDGame *g, int vertical)
{
  int k;
  if (!g->L || !g->nsel) return;
  gd_ed_push(g);
  for (k = 0; k < g->nsel; k++) {
    int i = g->selected[k];
    if (i >= 0 && i < g->L->nobj)
      g->L->o[i].rot = vertical ? (360.0f - g->L->o[i].rot)
                                : (180.0f - g->L->o[i].rot);
  }
}

void gd_ed_group_sel(GDGame *g, int grp)
{
  int k;
  if (!g->L || !g->nsel) return;
  gd_ed_push(g);
  for (k = 0; k < g->nsel; k++) {
    int i = g->selected[k];
    if (i >= 0 && i < g->L->nobj) { g->L->o[i].ngrp = 0; gd_obj_group(g, i, grp); }
  }
}

void gd_ed_del_sel(GDGame *g)
{
  int k;
  if (!g->L || !g->nsel) return;
  gd_ed_push(g);
  for (k = g->nsel - 1; k >= 0; k--) gd_obj_del(g, g->selected[k]);
  g->nsel = 0;
  gd_lvl_finish(g);
}

void gd_ed_copy_sel(GDGame *g)
{
  int k;
  if (!g->L) return;
  CLIP_N = 0;
  for (k = 0; k < g->nsel && k < GD_MAX_OBJ; k++) {
    int i = g->selected[k];
    if (i >= 0 && i < g->L->nobj) CLIP[CLIP_N++] = g->L->o[i];
  }
}

void gd_ed_paste(GDGame *g, float x, float y)
{
  int k;
  float cx = 1e9f, cy = 1e9f;
  if (!g->L || !CLIP_N) return;
  gd_ed_push(g);
  for (k = 0; k < CLIP_N; k++) {
    if (CLIP[k].x < cx) cx = CLIP[k].x;
    if (CLIP[k].y < cy) cy = CLIP[k].y;
  }
  g->nsel = 0;
  for (k = 0; k < CLIP_N; k++) {
    int i = gd_obj_add(g, CLIP[k].id, CLIP[k].x - cx + x, CLIP[k].y - cy + y);
    if (i >= 0) {
      g->L->o[i] = CLIP[k];
      g->L->o[i].x = CLIP[k].x - cx + x;
      g->L->o[i].y = CLIP[k].y - cy + y;
      if (g->nsel < GD_MAX_SEL) g->selected[g->nsel++] = i;
    }
  }
  gd_lvl_finish(g);
}

void gd_place_trigger(GDGame *g, int id)
{
  GDLevel *L = g->L;
  int i;
  if (!L) return;
  for (i = 0; i < L->nobj; i++)
    if (L->o[i].id == (unsigned short)id) { /* активация делается в activate() */ }
}

/* ============================================================== аккаунты */

static unsigned long long fnv(const char *s, unsigned long long seed)
{
  unsigned long long h = 1469598103934665603ULL ^ seed;
  while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211ULL; }
  return h;
}

static int name_ok(const char *n)
{
  int i, len = 0;
  for (i = 0; n[i]; i++) len++;
  if (len < 3 || len >= GD_STR) return 0;
  for (i = 0; n[i]; i++) {
    char c = n[i];
    if (c == ' ' || c == ';' || c == ',' || c == '|' || c == ':') return 0;
  }
  return 1;
}

int gd_acc_register(GDGame *g, const char *name, const char *pass, char *err)
{
  int i, pw = 0;
  GDAcc *a;
  for (i = 0; pass[i]; i++) pw++;
  if (!name_ok(name)) { if (err) err[0] = 0; return -1; }
  if (pw < 6) { if (err) err[0] = 0; return -2; }
  for (i = 0; i < g->nacc; i++)
    if (fnv(g->acc[i].name, 0) == fnv(name, 0)) { if (err) err[0] = 0; return -3; }
  if (g->nacc >= GD_MAX_ACC) return -4;
  a = &g->acc[g->nacc];
  for (i = 0; i < GD_STR - 1 && name[i]; i++) a->name[i] = name[i];
  a->name[i] = 0;
  a->hash = fnv(pass, fnv(name, 7));
  a->stars = 0; a->orbs = 500; a->demons = 0; a->coins = 0; a->diamonds = 0;
  a->c1 = 0; a->c2 = 5; a->glow = 0;
  for (i = 0; i < GD_MODE_N; i++) a->icon[i] = 0;
  g->curAcc = g->nacc;
  g->loggedIn = 1;
  g->nacc++;
  return 0;
}

int gd_acc_login(GDGame *g, const char *name, const char *pass, char *err)
{
  int i;
  unsigned long long h = fnv(pass, fnv(name, 7));
  if (err) err[0] = 0;
  for (i = 0; i < g->nacc; i++) {
    if (fnv(g->acc[i].name, 0) != fnv(name, 0)) continue;
    if (g->acc[i].hash != h) return -2;         /* неверный пароль */
    g->curAcc = i; g->loggedIn = 1;
    return 0;
  }
  return -1;                                    /* нет такого аккаунта */
}

void gd_acc_logout(GDGame *g) { g->loggedIn = 0; g->curAcc = -1; }

int gd_store_save(GDGame *g, char *buf, int cap)
{
  char *b = buf, *end = buf + cap;
  int i;
  for (i = 0; i < g->nacc; i++) {
    GDAcc *a = &g->acc[i];
    b = ps(b, end, "ACC;");
    b = ps(b, end, a->name); b = ps(b, end, ";");
    {
      char t[17];
      int n = 0, j;
      unsigned long long h = a->hash;
      do { t[n++] = "0123456789abcdef"[h & 15]; h >>= 4; } while (h && n < 16);
      while (n && b < end - 1) *b++ = t[--n];
      for (j = 0; j < 0; j++) { }
    }
    b = ps(b, end, ";");
    b = pf(b, end, (float)a->stars); b = ps(b, end, ",");
    b = pf(b, end, (float)a->orbs);  b = ps(b, end, ",");
    b = pf(b, end, (float)a->demons);b = ps(b, end, ",");
    b = pf(b, end, (float)a->coins); b = ps(b, end, ",");
    b = pf(b, end, (float)a->diamonds); b = ps(b, end, ",");
    b = pf(b, end, (float)a->c1); b = ps(b, end, ",");
    b = pf(b, end, (float)a->c2); b = ps(b, end, ",");
    b = pf(b, end, (float)a->glow); b = ps(b, end, "\n");
  }
  for (i = 0; i < g->nlv; i++) {
    int n;
    b = ps(b, end, "LVL;");
    n = gd_lvl_to_string(g, i, b, (int)(end - b));
    b += n;
    b = ps(b, end, "\n");
  }
  *b = 0;
  return (int)(b - buf);
}

void gd_store_load(GDGame *g, const char *buf)
{
  const char *p = buf;
  if (!buf) return;
  while (*p) {
    const char *e = p;
    while (*e && *e != '\n') e++;
    if (e - p > 4 && p[0] == 'A' && p[1] == 'C' && p[2] == 'C' && g->nacc < GD_MAX_ACC) {
      GDAcc *a = &g->acc[g->nacc];
      const char *q = p + 4;
      int i;
      unsigned long long h = 0;
      for (i = 0; i < GD_STR - 1 && q < e && *q != ';'; i++) a->name[i] = *q++;
      a->name[i] = 0;
      if (*q == ';') q++;
      while (q < e && *q != ';') {
        char c = *q++;
        int v = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : 0;
        h = (h << 4) | (unsigned)v;
      }
      a->hash = h;
      if (*q == ';') q++;
      a->stars    = (int)num(&q); if (*q == ',') q++;
      a->orbs     = (int)num(&q); if (*q == ',') q++;
      a->demons   = (int)num(&q); if (*q == ',') q++;
      a->coins    = (int)num(&q); if (*q == ',') q++;
      a->diamonds = (int)num(&q); if (*q == ',') q++;
      a->c1       = (int)num(&q); if (*q == ',') q++;
      a->c2       = (int)num(&q); if (*q == ',') q++;
      a->glow     = (int)num(&q);
      for (i = 0; i < GD_MODE_N; i++) a->icon[i] = 0;
      g->nacc++;
    } else if (e - p > 4 && p[0] == 'L' && p[1] == 'V' && p[2] == 'L') {
      int li = gd_lvl_new(g, "level", "");
      char tmp[GD_STORE];
      int n = (int)(e - (p + 4));
      if (n > GD_STORE - 1) n = GD_STORE - 1;
      {
        int i;
        for (i = 0; i < n; i++) tmp[i] = p[4 + i];
        tmp[n] = 0;
      }
      gd_lvl_select(g, li);
      gd_lvl_from_string(g, li, tmp);
    }
    p = (*e == '\n') ? e + 1 : e;
  }
}

/* ================================================================= игра */

static void die_now(GDGame *g);
static void activate_obj(GDGame *g, int idx);
static void act_group(GDGame *g, int grp);

static void burst(GDGame *g, float x, float y, float cr, float cg, float cb, int n, float spd)
{
  int i;
  for (i = 0; i < n && g->npart < GD_MAX_PART; i++) {
    GDPart *p = &g->part[g->npart++];
    float a = rf(g, 0.0f, 6.283f), s = rf(g, spd * 0.3f, spd);
    p->x = x; p->y = y;
    p->vx = cosf(a) * s; p->vy = sinf(a) * s + 3.0f;
    p->r = rf(g, 0.05f, 0.18f); p->life = rf(g, 0.3f, 0.8f);
    p->cr = cr; p->cg = cg; p->cb = cb;
  }
}

void gd_init(GDGame *g)
{
  int i;
  g->nlv = 0; g->cur = 0; g->L = 0;
  g->npart = 0; g->nact = 0; g->ncp = 0;
  g->rng = 0x5DEECEu;
  g->screen = GD_SCR_MENU;
  g->tool = GD_TOOL_BUILD; g->tab = 0;
  g->curObj = GD_BLOCK; g->curGroup = 0; g->selId = -1; g->editIdx = -1;
  g->nacc = 0; g->loggedIn = 0; g->curAcc = -1;
  g->attempt = 0;
  g->store[0] = 0;
  g->nsel = 0;
  for (i = 0; i < GD_MAX_ITEM; i++) g->items[i] = 0;
  gd_build_demo(g);
}

void gd_scr(GDGame *g, int scr) { if (scr >= 0 && scr < GD_SCR_N) g->screen = scr; }

void gd_start(GDGame *g, int li, int practice)
{
  gd_lvl_select(g, li);
  g->practice = practice;
  g->attempt = 0;
  g->screen = GD_SCR_GAME;
  g->L->plays++;
  gd_respawn(g);
}

void gd_respawn(GDGame *g)
{
  GDLevel *L = g->L;
  int i, k;
  float sx = 0.0f, sy = 0.0f;
  if (!L) return;
  g->phase = GD_PLAY;
  g->deadt = 0; g->npart = 0; g->nact = 0; g->shake = 0; g->flash = 0;
  g->camx = -4.5f; g->camy = 0; g->zoom = 1.0f; g->camrot = 0;
  g->speed = L->startSpeed; g->mode = L->startMode;
  g->mini = L->startMini; g->dual = L->startDual; g->mirror = L->startMirror;
  g->t = 0; g->jumps = 0; g->clicks = 0; g->coinsGot = 0; g->hold = 0;
  g->np = g->dual ? 2 : 1;
  for (k = 0; k < 2; k++) {
    GDPlayer *p = &g->p[k];
    p->x = 0; p->y = 0; p->vx = 0; p->vy = 0; p->rot = 0; p->holdT = 0;
    p->mode = g->mode; p->grav = 1; p->mini = g->mini;
    p->grounded = 1; p->dashT = 0; p->dead = 0; p->jumps = 0;
  }
  for (i = 0; i < L->nobj; i++) {
    GDObj *o = &L->o[i];
    o->ox = 0; o->oy = 0; o->used = 0; o->alpha = 1.0f;
    if (o->id == GD_STARTPOS && sx == 0.0f) {
      sx = o->x + 1.0f; sy = o->y;
      if (o->a[0] >= 0 && o->a[0] < GD_MODE_N) g->mode = (int)o->a[0];
      if (o->a[1] >= 0 && o->a[1] < 6) g->speed = (int)o->a[1];
    }
  }
  g->p[0].x = sx; g->p[0].y = sy; g->p[0].mode = g->mode;
  if (g->np > 1) {
    g->p[1] = g->p[0];
    g->p[1].grav = -1;
  }
  for (i = 0; i < GD_MAX_ITEM; i++) g->items[i] = 0;
  g->ncp = 0;
}

void gd_press(GDGame *g)
{
  int i;
  if (!g->hold) { g->clickEdge = 1; g->clicks++; }
  g->hold = 1;
  for (i = 0; g->L && i < g->L->nobj; i++)
    if (g->L->o[i].id >= GD_ORB_Y && g->L->o[i].id <= GD_ORB_TOGGLE) g->L->o[i].used = 0;
}

void gd_release(GDGame *g) { g->hold = 0; }

float gd_prog(const GDGame *g)
{
  float p;
  if (!g->L || g->L->len <= 0) return 0;
  p = g->p[0].x / g->L->len;
  return p < 0.0f ? 0.0f : (p > 1.0f ? 1.0f : p);
}

/* ------------------------------------------------------------- триггеры */

static void act_new(GDGame *g, int id, int grp, float dur, int ease, const float *v)
{
  GDAct *a;
  int i;
  if (g->nact >= GD_MAX_ACT) {                     /* вытесняем старое */
    for (i = 0; i < g->nact - 1; i++) g->act[i] = g->act[i + 1];
    g->nact--;
  }
  a = &g->act[g->nact++];
  a->id = (unsigned short)id;
  a->grp = (unsigned short)grp;
  a->t = 0; a->dur = dur > 0 ? dur : 0.0001f; a->prev = 0;
  a->ease = (unsigned char)ease;
  a->alive = 1;
  for (i = 0; i < 8; i++) a->v[i] = v ? v[i] : 0.0f;
}

static void set_visible(GDGame *g, int grp, int vis)
{
  int i;
  for (i = 0; g->L && i < g->L->nobj; i++)
    if (in_grp(&g->L->o[i], grp)) g->L->o[i].visible = (unsigned char)vis;
}

/* активация всех триггеров группы */
static void act_group(GDGame *g, int grp)
{
  int i;
  for (i = 0; g->L && i < g->L->nobj; i++)
    if (in_grp(&g->L->o[i], grp)) activate_obj(g, i);
}

static void activate_obj(GDGame *g, int idx)
{
  GDObj *o;
  float v[8];
  int i;
  if (!g->L || idx < 0 || idx >= g->L->nobj) return;
  o = &g->L->o[idx];
  for (i = 0; i < 8; i++) v[i] = 0;
  switch (o->id) {
    case GD_T_MOVE:
      v[0] = o->a[1] / UNITS; v[1] = o->a[2] / UNITS;
      act_new(g, GD_T_MOVE, (int)o->a[0], o->a[3], (int)o->a[4], v);
      break;
    case GD_T_ROTATE:
      v[0] = o->a[1];
      act_new(g, GD_T_ROTATE, (int)o->a[0], o->a[2], (int)o->a[3], v);
      break;
    case GD_T_SCALE:
      v[0] = o->a[1] - 1.0f; v[1] = o->a[2] - 1.0f;
      act_new(g, GD_T_SCALE, (int)o->a[0], o->a[3], 0, v);
      break;
    case GD_T_ALPHA:
      v[0] = o->a[1]; v[1] = 1.0f;
      act_new(g, GD_T_ALPHA, (int)o->a[0], o->a[2], 0, v);
      break;
    case GD_T_COLOR: {
      int ch = (int)o->a[0];
      if (ch < 0 || ch >= GD_CH) break;
      v[0] = o->a[1] / 255.0f; v[1] = o->a[2] / 255.0f; v[2] = o->a[3] / 255.0f;
      v[3] = (float)ch;
      v[4] = g->L->col[ch][0]; v[5] = g->L->col[ch][1]; v[6] = g->L->col[ch][2];
      act_new(g, GD_T_COLOR, 0, o->a[4], 0, v);
      break;
    }
    case GD_T_PULSE:
      v[0] = o->a[1]; v[1] = o->a[2]; v[2] = o->a[3]; v[3] = o->a[4]; v[4] = o->a[0];
      act_new(g, GD_T_PULSE, (int)o->a[0], o->a[1] + o->a[2] + o->a[3], 0, v);
      break;
    case GD_T_SPAWN:
      v[0] = o->a[0];
      act_new(g, GD_T_SPAWN, (int)o->a[0], o->a[1], 0, v);
      break;
    case GD_T_TOGGLE: set_visible(g, (int)o->a[0], (int)o->a[1]); break;
    case GD_T_SHOW:   set_visible(g, (int)o->a[0], 1); break;
    case GD_T_HIDE:   set_visible(g, (int)o->a[0], 0); break;
    case GD_T_STOP: {
      int k;
      for (k = 0; k < g->nact; k++)
        if (g->act[k].grp == (int)o->a[0]) g->act[k].alive = 0;
      break;
    }
    case GD_T_FOLLOW:
      v[0] = o->a[1]; v[1] = o->a[2];
      act_new(g, GD_T_FOLLOW, (int)o->a[0], 9999.0f, 0, v);
      break;
    case GD_T_FOLLOWY:
      v[0] = o->a[1]; v[1] = o->a[2];
      act_new(g, GD_T_FOLLOWY, (int)o->a[0], 9999.0f, 0, v);
      break;
    case GD_T_MOVETGT:
      v[0] = o->a[1]; v[1] = o->a[2];
      act_new(g, GD_T_MOVETGT, (int)o->a[0], 9999.0f, 0, v);
      break;
    case GD_T_KILL: die_now(g); break;
    case GD_T_END:
      g->phase = GD_WIN;
      break;
    case GD_T_SHAKE:
      if (o->a[0] > g->shake) g->shake = o->a[0];
      break;
    case GD_T_FLASH:
      g->fr = o->a[0]; g->fg = o->a[1]; g->fb = o->a[2];
      g->flash = o->a[3] > 0 ? o->a[3] : 0.4f;
      break;
    case GD_T_CAMOFF:
      v[0] = o->a[0]; v[1] = o->a[1];
      act_new(g, GD_T_CAMOFF, 0, o->a[2], 0, v);
      break;
    case GD_T_ZOOM:
      v[0] = o->a[0] > 0 ? o->a[0] : 1.0f;
      act_new(g, GD_T_ZOOM, 0, o->a[1], 0, v);
      break;
    case GD_T_PARTICLE:
      burst(g, o->x + 0.5f, o->y + 0.5f, 1.0f, 0.8f, 0.3f,
            (int)(o->a[0] > 0 ? o->a[0] : 8), o->a[1] > 0 ? o->a[1] : 6.0f);
      break;
    case GD_T_PICKUP: {
      int it = (int)o->a[0];
      if (it >= 0 && it < GD_MAX_ITEM) {
        g->items[it] += (int)(o->a[1] != 0 ? o->a[1] : 1);
        /* проверяем Count-триггеры */
        for (i = 0; i < g->L->nobj; i++) {
          GDObj *c = &g->L->o[i];
          if ((c->id == GD_T_COUNT || c->id == GD_T_INSTCOUNT) &&
              (int)c->a[0] == it && g->items[it] >= (int)c->a[1] && !c->used) {
            c->used = 1;
            act_group(g, (int)c->a[2]);
          }
        }
      }
      break;
    }
    case GD_T_SEQ:
      for (i = 0; i < 4; i++) {
        if (o->a[i] > 0) {
          float vv[8];
          int k;
          for (k = 0; k < 8; k++) vv[k] = 0;
          vv[0] = o->a[i];
          act_new(g, GD_T_SPAWN, (int)o->a[i], o->a[4] * (float)i, 0, vv);
        }
      }
      break;
    case GD_T_RANDOM: {
      int cnt = 0, pick, k;
      float vv[8];
      for (i = 0; i < 8; i++) vv[i] = 0;
      for (i = 0; i < 4; i++) if (o->a[i] > 0) cnt++;
      if (!cnt) break;
      pick = (int)(rnd(g) % (unsigned)cnt);
      for (i = 0, k = 0; i < 4; i++)
        if (o->a[i] > 0) { if (k++ == pick) { vv[0] = o->a[i];
          act_new(g, GD_T_SPAWN, (int)o->a[i], 0.0f, 0, vv); } }
      break;
    }
    default: break;
  }
}

static void step_actions(GDGame *g, float dt)
{
  int k, i;
  for (k = 0; k < g->nact; k++) {
    GDAct *a = &g->act[k];
    float e, d;
    if (!a->alive) continue;
    a->t += dt;
    switch (a->id) {
      case GD_T_MOVE:
      case GD_T_ROTATE:
      case GD_T_SCALE:
        e = gd_ease(a->ease, a->t / a->dur, 2.0f);
        d = e - a->prev;
        a->prev = e;
        for (i = 0; g->L && i < g->L->nobj; i++) {
          GDObj *o = &g->L->o[i];
          if (!in_grp(o, a->grp)) continue;
          if (a->id == GD_T_MOVE)        { o->ox += a->v[0] * d; o->oy += a->v[1] * d; }
          else if (a->id == GD_T_ROTATE)   o->rot += a->v[0] * d;
          else                           { o->sx += a->v[0] * d; o->sy += a->v[1] * d; }
        }
        break;
      case GD_T_ALPHA:
        e = a->t / a->dur; if (e > 1.0f) e = 1.0f;
        for (i = 0; g->L && i < g->L->nobj; i++)
          if (in_grp(&g->L->o[i], a->grp))
            g->L->o[i].alpha = a->v[1] + (a->v[0] - a->v[1]) * e;
        break;
      case GD_T_COLOR: {
        int ch = (int)a->v[3];
        e = a->t / a->dur; if (e > 1.0f) e = 1.0f;
        if (ch >= 0 && ch < GD_CH) {
          g->L->col[ch][0] = a->v[4] + (a->v[0] - a->v[4]) * e;
          g->L->col[ch][1] = a->v[5] + (a->v[1] - a->v[5]) * e;
          g->L->col[ch][2] = a->v[6] + (a->v[2] - a->v[6]) * e;
        }
        break;
      }
      case GD_T_FOLLOW:
      case GD_T_MOVETGT: {
        float lx = 0, ly = 0, cx, cy;
        int n = 0;
        for (i = 0; i < g->L->nobj; i++)
          if (in_grp(&g->L->o[i], (int)a->v[0])) {
            lx += g->L->o[i].x + g->L->o[i].ox; ly += g->L->o[i].y + g->L->o[i].oy; n++;
          }
        if (!n) break;
        lx /= n; ly /= n;
        for (i = 0; i < g->L->nobj; i++) {
          GDObj *o = &g->L->o[i];
          float sp = a->v[1] > 0 ? a->v[1] : 1.0f;
          if (!in_grp(o, a->grp)) continue;
          cx = o->x + o->ox; cy = o->y + o->oy;
          o->ox += (lx - cx) * sp * dt;
          o->oy += (ly - cy) * sp * dt;
        }
        break;
      }
      case GD_T_FOLLOWY:
        for (i = 0; i < g->L->nobj; i++) {
          GDObj *o = &g->L->o[i];
          float sp = a->v[0] > 0 ? a->v[0] : 1.0f;
          float want = g->p[0].y + a->v[1];
          if (!in_grp(o, a->grp)) continue;
          o->oy += (want - (o->y + o->oy)) * sp * dt;
        }
        break;
      case GD_T_CAMOFF:
        e = a->t / a->dur; if (e > 1.0f) e = 1.0f;
        g->camx = g->p[0].x - 4.5f + a->v[0] * e;
        break;
      case GD_T_ZOOM:
        e = a->t / a->dur; if (e > 1.0f) e = 1.0f;
        g->zoom = 1.0f + (a->v[0] - 1.0f) * e;
        break;
      case GD_T_SPAWN:
        if (a->t >= a->dur) { act_group(g, (int)a->v[0]); a->alive = 0; }
        break;
      case GD_T_PULSE:
        break;    /* интенсивность читается рендером через gd_pulse() */
      default: break;
    }
    if (a->t >= a->dur && a->id != GD_T_FOLLOW && a->id != GD_T_FOLLOWY &&
        a->id != GD_T_MOVETGT && a->id != GD_T_SPAWN) a->alive = 0;
  }
  /* уплотняем */
  for (k = 0, i = 0; k < g->nact; k++)
    if (g->act[k].alive) { if (i != k) g->act[i] = g->act[k]; i++; }
  g->nact = i;
}

float gd_pulse(const GDGame *g, int ch)
{
  int k;
  float best = 0;
  for (k = 0; k < g->nact; k++) {
    const GDAct *a = &g->act[k];
    float fin, fout, total, amp;
    if (a->id != GD_T_PULSE || !a->alive) continue;
    if ((int)a->v[4] != ch) continue;
    fin = a->v[1]; fout = a->v[2]; total = a->dur;
    if (a->t < fin)               amp = fin > 0 ? a->t / fin : 1.0f;
    else if (a->t < total - fout) amp = 1.0f;
    else                          amp = fout > 0 ? (total - a->t) / fout : 0.0f;
    if (amp < 0) amp = 0;
    amp *= a->v[3];
    if (amp > best) best = amp;
  }
  return best;
}

/* ============================================================== физика */

static float psz(const GDPlayer *p) { return p->mini ? 0.62f : 1.0f; }

static void die_now(GDGame *g)
{
  if (g->phase != GD_PLAY) return;
  g->phase = GD_DEAD;
  g->deadt = 0;
  g->shake = 1.0f;
  burst(g, g->p[0].x, g->p[0].y + 0.5f, 1.0f, 1.0f, 1.0f, 24, 14.0f);
}

static void spider_teleport(GDGame *g, GDPlayer *p);

static void activate_orb(GDGame *g, GDPlayer *p, GDObj *o)
{
  float v = JUMPV;
  o->used = 1;
  burst(g, o->x + 0.5f, o->y + 0.5f,
        gd_info(o->id)->cr, gd_info(o->id)->cg, gd_info(o->id)->cb, 8, 6.0f);
  switch (o->id) {
    case GD_ORB_Y: p->vy = v * p->grav; break;
    case GD_ORB_P: p->vy = v * 0.62f * p->grav; break;
    case GD_ORB_R: p->vy = v * 1.45f * p->grav; break;
    case GD_ORB_B: p->grav = -p->grav; p->vy = v * p->grav; break;
    case GD_ORB_G: p->grav = -p->grav; p->vy = 0; break;
    case GD_ORB_K: p->grav = -p->grav; p->vy = v * 0.9f * p->grav; break;
    case GD_ORB_DASH: case GD_ORB_DASHP:
      p->dashT = 0.26f; p->dashA = o->a[0] * 3.14159265f / 180.0f;
      if (o->id == GD_ORB_DASHP) p->grav = -p->grav;
      break;
    case GD_ORB_SPIDER: spider_teleport(g, p); break;
    case GD_ORB_TOGGLE: act_group(g, (int)o->a[0]); break;
    default: break;
  }
  if (p->vy * p->grav > 0) p->grounded = 0;
  p->jumps++;
  g->jumps++;
}

static void activate_pad(GDGame *g, GDPlayer *p, GDObj *o)
{
  float v = JUMPV;
  o->used = 1;
  burst(g, o->x + 0.5f, o->y + 0.3f,
        gd_info(o->id)->cr, gd_info(o->id)->cg, gd_info(o->id)->cb, 6, 5.0f);
  switch (o->id) {
    case GD_PAD_Y: p->vy = v * 1.35f * p->grav; break;
    case GD_PAD_P: p->vy = v * 0.75f * p->grav; break;
    case GD_PAD_R: p->vy = v * 1.90f * p->grav; break;
    case GD_PAD_B: p->grav = -p->grav; p->vy = v * p->grav; break;
    case GD_PAD_DASH:
      p->dashT = 0.30f; p->dashA = o->a[0] * 3.14159265f / 180.0f;
      break;
    default: break;
  }
  p->grounded = 0;
  p->jumps++;
  g->jumps++;
}

static void spider_teleport(GDGame *g, GDPlayer *p)
{
  int i;
  float s = psz(p), dir = (float)-p->grav, best = -1, y = p->y;
  for (i = 0; g->L && i < g->L->nobj; i++) {
    GDObj *o = &g->L->o[i];
    float top, bot;
    if (!o->visible || gd_info(o->id)->kind != 0) continue;
    if (p->x + s * 0.3f < o->x + o->ox || p->x - s * 0.3f > o->x + o->ox + o->w * o->sx) continue;
    bot = o->y + o->oy; top = bot + o->h * o->sy;
    if (dir > 0 && bot > y + s * 0.4f && (best < 0 || bot < best)) best = bot;
    if (dir < 0 && top < y + s * 0.6f && (best < 0 || top > best)) best = top;
  }
  if (dir > 0 && (best < 0 || best > CEIL - s)) best = CEIL - s;
  if (dir < 0 && best < 0) best = 0;
  p->y = (dir > 0) ? best : best - s;
  p->grav = -p->grav;
  p->vy = 0;
  p->grounded = 1;
  burst(g, p->x, p->y + s * 0.5f, 0.7f, 0.4f, 1.0f, 10, 7.0f);
}

static int overlap(float ax, float ay, float aw, float ah,
                   float bx, float by, float bw, float bh)
{
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void step_player(GDGame *g, GDPlayer *p)
{
  GDLevel *L = g->L;
  float s = psz(p), hw = s * 0.30f, prev = p->y, sp;
  int i, blockJ = 0, blockH = 0, blockS = 0, blockF = 0, blockD = 0;
  float px0, px1;

  sp = NORMAL * gd_speed_mul(g->speed) * (p->mini ? 0.90f : 1.0f);
  p->vx = sp;
  p->x += sp * STEP;

  /* letter-блоки: D/J/S/H/F из Geometry Dash (вкладка Special) */
  for (i = 0; i < L->nobj; i++) {
    GDObj *o = &L->o[i];
    if (o->id < GD_LBL_D || o->id > GD_LBL_F) continue;
    if (!overlap(p->x - hw, p->y, hw * 2, s, o->x + o->ox, o->y + o->oy, o->w, o->h)) continue;
    if (o->id == GD_LBL_D) blockD = 1;
    if (o->id == GD_LBL_J) blockJ = 1;
    if (o->id == GD_LBL_S) blockS = 1;
    if (o->id == GD_LBL_H) blockH = 1;
    if (o->id == GD_LBL_F) blockF = 1;
  }
  if (blockS) p->dashT = 0;

  /* вертикальное движение по режиму */
  if (p->dashT > 0) {
    p->dashT -= STEP;
    p->x += cosf(p->dashA) * 12.0f * STEP;
    p->y += sinf(p->dashA) * 12.0f * STEP;
  } else switch (p->mode) {
    case GD_SHIP: case GD_SWING: {
      /* у корабля тяга чуть сильнее гравитации, у свинга — вдвое (резкий свинг) */
      float thr = (p->mode == GD_SWING) ? GRAV * 2.0f : GRAV * 1.15f;
      float grv = (p->mode == GD_SWING) ? GRAV : GRAV * 0.55f;
      p->vy -= grv * (float)p->grav * STEP;
      if (g->hold) p->vy += thr * (float)p->grav * STEP;
      if (p->vy >  10.0f) p->vy =  10.0f;
      if (p->vy < -10.0f) p->vy = -10.0f;
      p->y += p->vy * STEP;
      break;
    }
    case GD_WAVE:
      p->vy = (g->hold ? 1.0f : -1.0f) * sp;
      p->y += p->vy * STEP;
      break;
    case GD_UFO:
      if (g->clickEdge && !blockJ) p->vy = 11.5f * (float)p->grav;
      p->vy -= GRAV * 0.9f * (float)p->grav * STEP;
      p->y += p->vy * STEP;
      break;
    case GD_BALL:
      p->vy -= GRAV * (float)p->grav * STEP;
      if (g->clickEdge && p->grounded && !blockJ) { p->grav = -p->grav; p->vy = 0; g->jumps++; }
      p->y += p->vy * STEP;
      break;
    case GD_SPIDER:
      p->vy -= GRAV * (float)p->grav * STEP;
      if (g->clickEdge && p->grounded && !blockJ) spider_teleport(g, p);
      p->y += p->vy * STEP;
      break;
    case GD_ROBOT:
      if (g->clickEdge && p->grounded && !blockJ) {
        p->vy = JUMPV * (float)p->grav; p->holdT = 0; p->grounded = 0;
        p->jumps++; g->jumps++;
      }
      p->vy -= GRAV * (float)p->grav * STEP;
      if (g->hold && p->holdT < 0.13f && !p->grounded) {   /* робот тянет прыжок */
        p->holdT += STEP;
        p->vy += GRAV * 0.45f * (float)p->grav * STEP;
      }
      p->y += p->vy * STEP;
      break;
    default:   /* куб: сначала прыжок, потом гравитация и перемещение */
      if (g->hold && p->grounded && !blockJ) {
        p->vy = JUMPV * (float)p->grav; p->grounded = 0;
        p->jumps++; g->jumps++;
      }
      p->vy -= GRAV * (float)p->grav * STEP;
      p->y += p->vy * STEP;
      break;
  }

  /* пол и потолок */
  p->grounded = 0;
  if (p->grav > 0) {
    if (p->y <= 0.0f) { p->y = 0.0f; if (p->vy < 0) p->vy = 0; p->grounded = 1; }
    if (p->y + s >= CEIL) {
      p->y = CEIL - s;
      if (blockF) { p->grav = -1; p->vy = 0; }
      else if (p->mode == GD_SHIP || p->mode == GD_WAVE || p->mode == GD_SWING) { g->killId = -2; die_now(g); return; }
      else if (p->vy > 0) p->vy = 0;
    }
  } else {
    if (p->y + s >= CEIL) { p->y = CEIL - s; if (p->vy > 0) p->vy = 0; p->grounded = 1; }
    if (p->y <= 0.0f) {
      p->y = 0.0f;
      if (blockF) { p->grav = 1; p->vy = 0; }
      else if (p->mode == GD_SHIP || p->mode == GD_WAVE || p->mode == GD_SWING) { g->killId = -2; die_now(g); return; }
      else if (p->vy < 0) p->vy = 0;
    }
  }

  px0 = p->x - hw; px1 = p->x + hw;

  /* объекты */
  for (i = 0; i < L->nobj; i++) {
    GDObj *o = &L->o[i];
    const GDInfo *inf;
    float ox, oy, ow, oh;
    if (!o->visible) continue;
    ox = o->x + o->ox; oy = o->y + o->oy;
    if (ox > p->x + 3.0f) break;                 /* объекты отсортированы по X */
    ow = o->w * o->sx; oh = o->h * o->sy;
    if (ox + ow < p->x - 2.0f) continue;
    inf = gd_info(o->id);
    if (!inf) continue;

    switch (inf->kind) {
      case 0: {  /* твёрдый блок: сверху можно стоять, снизу бьёмся, сбоку гибнем */
        float top;
        if (!overlap(px0, p->y, hw * 2, s, ox, oy, ow, oh)) break;
        top = oy + oh;
        if (prev >= top - EPS) {                 /* игрок был над блоком */
          if (p->grav > 0 && p->vy <= 0.0f) { p->y = top; p->vy = 0; p->grounded = 1; }
          break;                                 /* прыжок с верха блока не убивает */
        }
        if (prev + s <= oy + EPS) {              /* игрок был под блоком */
          if (p->grav < 0 && p->vy >= 0.0f) { p->y = oy - s; p->vy = 0; p->grounded = 1; break; }
          if (blockH) { p->vy = 0; break; }      /* H-блок: нет урона сверху */
          if (blockF) { p->grav = -p->grav; p->vy = 0; break; }
          g->killId = o->id; die_now(g); return;
        }
        if (p->mode == GD_WAVE && blockD) break; /* D-блок: волна не бьётся о платформы */
        g->killId = o->id; die_now(g); return;
      }

      case 2: { /* склон: проверяем только поверхность, а не весь AABB */
        float rel = (p->x - ox) / (ow > 0 ? ow : 1.0f), surf;
        if (rel < 0.0f || rel > 1.0f) break;
        surf = oy + ((o->id == GD_SLOPE_UP) ? rel : (1.0f - rel)) * oh;
        if (p->grav > 0) {
          if (p->y <= surf + EPS) {              /* ноги на поверхности или ниже */
            if (p->y >= surf - 0.60f) {
              p->y = surf; if (p->vy < 0.0f) p->vy = 0; p->grounded = 1; break;
            }
            g->killId = o->id; die_now(g); return;   /* вошли в склон сбоку */
          }
          break;                                 /* выше поверхности — свободно */
        }
        if (p->y + s >= surf - EPS) {            /* перевёрнутая гравитация */
          if (p->y + s <= surf + 0.60f) {
            p->y = surf - s; if (p->vy > 0.0f) p->vy = 0; p->grounded = 1; break;
          }
          g->killId = o->id; die_now(g); return;
        }
        break;
      }

      case 1: { /* шип: хитбокс заметно меньше картинки */
        float cx = ox + ow * 0.5f, base = oy, h = oh * 0.45f;
        int flip = (o->rot > 90.0f && o->rot < 270.0f);
        float lo = flip ? oy + oh - h : base, hi = flip ? oy + oh : base + h;
        if (px1 > cx - 0.16f && px0 < cx + 0.16f &&
            p->y + 0.06f < hi && p->y + s > lo) { die_now(g); return; }
        break;
      }

      case 3: { /* пила: круг, вращается только визуально */
        float cx = ox + ow * 0.5f, cy = oy + oh * 0.5f;
        float dx = p->x - cx, dy = (p->y + s * 0.5f) - cy;
        float r = ow * 0.5f * 0.92f + s * 0.28f;
        if (dx * dx + dy * dy < r * r) { g->killId = o->id; die_now(g); return; }
        break;
      }

      case 4:   /* орб: срабатывает по нажатию */
        if (!overlap(px0, p->y, hw * 2, s, ox, oy, ow, oh)) break;
        if (g->clickEdge && !o->used) activate_orb(g, p, o);
        break;

      case 5:   /* пад: срабатывает от касания */
        if (overlap(px0, p->y, hw * 2, s, ox, oy, ow, oh)) {
          if (!o->used) activate_pad(g, p, o);
        } else o->used = 0;
        break;

      case 6:   /* портал */
        if (!overlap(px0, p->y, hw * 2, s, ox, oy, ow, oh)) break;
        switch (o->id) {
          case GD_P_CUBE:   p->mode = GD_CUBE;   break;
          case GD_P_SHIP:   p->mode = GD_SHIP;   break;
          case GD_P_BALL:   p->mode = GD_BALL;   break;
          case GD_P_UFO:    p->mode = GD_UFO;    break;
          case GD_P_WAVE:   p->mode = GD_WAVE;   break;
          case GD_P_ROBOT:  p->mode = GD_ROBOT;  break;
          case GD_P_SPIDER: p->mode = GD_SPIDER; break;
          case GD_P_SWING:  p->mode = GD_SWING;  break;
          case GD_P_GRAVD:  p->grav = 1;  break;
          case GD_P_GRAVU:  p->grav = -1; break;
          case GD_P_MINI:   p->mini = 1; break;
          case GD_P_BIG:    p->mini = 0; break;
          case GD_P_DUAL:
            if (g->np < 2) {
              g->np = 2; g->p[1] = *p; g->p[1].grav = -p->grav;
            }
            break;
          case GD_P_NODUAL: g->np = 1; break;
          case GD_P_MIRROR:   g->mirror = 1; break;
          case GD_P_NOMIRROR: g->mirror = 0; break;
          case GD_P_TPA: case GD_P_TPB: {
            int j;
            for (j = 0; j < L->nobj; j++) {
              GDObj *t = &L->o[j];
              if (t == o || t->id != (o->id == GD_P_TPA ? GD_P_TPB : GD_P_TPA)) continue;
              if ((int)t->a[0] != (int)o->a[0]) continue;
              p->x = t->x + t->ox + 1.0f;
              p->y = t->y + t->oy;
              burst(g, p->x, p->y + 0.5f, 1.0f, 0.7f, 0.3f, 12, 8.0f);
              break;
            }
            break;
          }
          default: break;
        }
        if (o->id >= GD_P_S0 && o->id <= GD_P_S5) g->speed = o->id - GD_P_S0;
        g->mode = p->mode; g->mini = p->mini;
        break;

      case 7:   /* монета */
        if (!o->used && overlap(px0, p->y, hw * 2, s, ox, oy, ow, oh)) {
          o->used = 1; g->coinsGot++;
          burst(g, ox + 0.5f, oy + 0.5f, 1.0f, 0.85f, 0.2f, 10, 6.0f);
        }
        break;

      case 10:  /* чекпоинт практики */
        if (g->practice && !o->used &&
            overlap(px0, p->y, hw * 2, s, ox, oy, ow, oh)) {
          o->used = 1;
          if (g->ncp < GD_MAX_CP) {
            GDCp *c = &g->cp[g->ncp++];
            c->x = p->x; c->y = p->y; c->vy = 0;
            c->mode = p->mode; c->grav = p->grav;
          }
        }
        break;
      default: break;
    }
  }

  /* вращение иконки */
  if (p->mode == GD_CUBE || p->mode == GD_ROBOT || p->mode == GD_SPIDER) {
    if (!p->grounded) p->rot += 834.0f * STEP * (float)p->grav;
    else {
      float q = p->rot / 90.0f, snap;
      q = (float)(int)(q + (q >= 0.0f ? 0.5f : -0.5f));
      snap = q * 90.0f;
      p->rot += (snap - p->rot) * (STEP * 30.0f);
    }
  } else if (p->mode == GD_SHIP || p->mode == GD_SWING) {
    p->rot = -p->vy * 1.6f * (float)p->grav;
  } else if (p->mode == GD_WAVE) {
    p->rot = (g->hold ? 45.0f : -45.0f) * (float)p->grav;
  }
}

void gd_update(GDGame *g, float dt)
{
  int i;
  if (dt > 0.1f) dt = 0.1f;
  g->accum += dt;
  while (g->accum >= STEP) {
    g->accum -= STEP;
    g->t += STEP;
    if (g->shake > 0.0f) g->shake -= STEP * 3.0f;
    if (g->flash > 0.0f) g->flash -= STEP;

    for (i = 0; i < g->npart; i++) {
      GDPart *p = &g->part[i];
      p->life -= STEP;
      p->vy -= 60.0f * STEP;
      p->x += p->vx * STEP;
      p->y += p->vy * STEP;
      if (p->y < 0.0f) { p->y = 0.0f; p->vy *= -0.35f; p->vx *= 0.8f; }
    }
    for (i = 0; i < g->npart; i++)
      if (g->part[i].life <= 0.0f) {
        g->part[i] = g->part[g->npart - 1];
        g->npart--; i--;
      }

    if (g->screen != GD_SCR_GAME || g->phase != GD_PLAY) {
      if (g->screen == GD_SCR_GAME && g->phase == GD_DEAD) {
        g->deadt += STEP;
        if (g->deadt > 0.75f) {
          if (g->practice && g->ncp > 0) {
            GDCp *c = &g->cp[g->ncp - 1];
            g->phase = GD_PLAY; g->deadt = 0; g->npart = 0;
            g->p[0].x = c->x; g->p[0].y = c->y; g->p[0].vy = 0;
            g->p[0].mode = c->mode; g->p[0].grav = c->grav;
            g->mode = c->mode; g->attempt++;
            if (g->np > 1) { g->p[1] = g->p[0]; g->p[1].grav = -c->grav; }
          } else {
            gd_respawn(g);                 /* попытки не сбрасываем, как в GD */
          }
        }
      }
      g->clickEdge = 0;
      continue;
    }

    /* триггеры без группы срабатывают, когда игрок доходит до них */
    for (i = 0; i < g->L->nobj; i++) {
      GDObj *o = &g->L->o[i];
      const GDInfo *inf = gd_info(o->id);
      if (!inf || inf->kind != 9 || o->ngrp) continue;
      if (!o->used && g->p[0].x >= o->x + o->ox) { o->used = 1; activate_obj(g, i); }
    }
    /* Touch-триггеры */
    for (i = 0; i < g->L->nobj; i++) {
      GDObj *o = &g->L->o[i];
      float s = psz(&g->p[0]);
      if (o->id != GD_T_TOUCH) continue;
      if (overlap(g->p[0].x - s * 0.3f, g->p[0].y, s * 0.6f, s,
                  o->x + o->ox, o->y + o->oy, o->w, o->h)) {
        if (!o->used || o->a[1]) { o->used = 1; act_group(g, (int)o->a[0]); }
      } else o->used = 0;
    }
    /* Collision-триггеры: по касанию блоков с нужным cid */
    for (i = 0; i < g->L->nobj; i++) {
      GDObj *o = &g->L->o[i];
      int hit = 0, j;
      float s;
      if (o->id != GD_T_COLLIDE) continue;
      s = psz(&g->p[0]);
      for (j = 0; j < g->L->nobj; j++) {
        GDObj *b = &g->L->o[j];
        const GDInfo *bi = gd_info(b->id);
        if (!bi || (bi->kind != 0 && bi->kind != 2)) continue;
        if ((int)b->a[0] != (int)o->a[0]) continue;
        if (overlap(g->p[0].x - s * 0.3f, g->p[0].y, s * 0.6f, s,
                    b->x + b->ox, b->y + b->oy, b->w * b->sx, b->h * b->sy)) { hit = 1; break; }
      }
      if (hit && !o->used) { o->used = 1; act_group(g, (int)o->a[1]); }
      else if (!hit && o->a[2] && o->used) { o->used = 0; act_group(g, (int)o->a[1]); }
    }

    step_actions(g, STEP);

    for (i = 0; i < g->np && g->phase == GD_PLAY; i++) step_player(g, &g->p[i]);

    g->mode = g->p[0].mode;
    g->mini = g->p[0].mini;
    g->camx = g->p[0].x - 4.5f;
    {
      float want = g->p[0].y - 4.0f;
      if (want < -1.0f) want = -1.0f;
      if (want > CEIL - 9.0f) want = CEIL - 9.0f;
      g->camy += (want - g->camy) * (STEP * 6.0f);
    }

    if (g->phase == GD_PLAY && g->p[0].x >= g->L->len) {
      int pc = (int)(gd_prog(g) * 100.0f);
      g->phase = GD_WIN;
      if (pc > g->L->best) g->L->best = pc;
      if (g->loggedIn && g->curAcc >= 0 && g->curAcc < g->nacc) {
        GDAcc *a = &g->acc[g->curAcc];
        if (!g->practice) {
          a->stars += g->L->stars;
          a->orbs += g->L->stars * 10;
          a->coins += g->coinsGot;
          a->demons += (g->L->diff >= 6) ? 1 : 0;
        }
      }
      g->screen = GD_SCR_DONE;
    }
    if (g->phase == GD_DEAD) {
      int pc = (int)(gd_prog(g) * 100.0f);
      if (pc > g->L->best) g->L->best = pc;
      g->attempt++;
    }
    g->clickEdge = 0;
  }
}

/* ====================================================== демо-уровни */

static int add_o(GDGame *g, int id, float x, float y) { return gd_obj_add(g, id, x, y); }

static int trg(GDGame *g, int id, float x, float y, int grp)
{
  int i = gd_obj_add(g, id, x, y);
  if (i >= 0 && grp > 0) gd_obj_group(g, i, grp);
  return i;
}

void gd_build_demo(GDGame *g)
{
  float x;
  int li, i;

  g->nlv = 0;

  /* --- 1. Stereo Bounce: куб, орбы, пады, шипы, пилы, порталы скорости */
  li = gd_lvl_new(g, "Stereo Bounce", "RobTop");
  gd_lvl_select(g, li);
  LV[li].official = 1; LV[li].diff = 2; LV[li].stars = 3;
  LV[li].startMode = GD_CUBE; LV[li].startSpeed = 1;
  for (x = 8; x < 30; x += 1) add_o(g, GD_DECO_COL, x, 4.0f);
  add_o(g, GD_SPIKE, 26, 0); add_o(g, GD_SPIKE, 34, 0);
  add_o(g, GD_BLOCK, 40, 0); add_o(g, GD_SPIKE, 41, 0);
  add_o(g, GD_ORB_Y, 46, 2); add_o(g, GD_SPIKE, 49, 0); add_o(g, GD_SPIKE, 50, 0);
  add_o(g, GD_PAD_Y, 55, 0); add_o(g, GD_SAW_S, 58, 0);
  add_o(g, GD_P_S2, 64, 1);
  add_o(g, GD_SPIKE, 70, 0); add_o(g, GD_SPIKE, 71, 0);
  add_o(g, GD_ORB_P, 76, 2); add_o(g, GD_SPIKE, 79, 0);
  add_o(g, GD_BLOCK, 84, 0); add_o(g, GD_BLOCK, 85, 0); add_o(g, GD_SPIKE, 87, 0);
  add_o(g, GD_PAD_R, 92, 0); add_o(g, GD_SAW_M, 95, 2);
  add_o(g, GD_COIN, 99, 4);
  add_o(g, GD_SPIKE, 104, 0); add_o(g, GD_SPIKE, 105, 0); add_o(g, GD_SPIKE, 106, 0);
  add_o(g, GD_ORB_R, 110, 2);
  add_o(g, GD_P_S1, 118, 1);
  add_o(g, GD_SPIKE, 124, 0); add_o(g, GD_BLOCK, 130, 0); add_o(g, GD_SLOPE_UP, 132, 0);
  add_o(g, GD_SPIKE, 136, 0); add_o(g, GD_SPIKE, 137, 0);
  add_o(g, GD_ORB_Y, 142, 2); add_o(g, GD_SAW_S, 145, 0);
  add_o(g, GD_PAD_Y, 150, 0); add_o(g, GD_SPIKE, 153, 0); add_o(g, GD_SPIKE, 154, 0);
  add_o(g, GD_COIN, 158, 3);
  add_o(g, GD_SPIKE, 162, 0); add_o(g, GD_SPIKE, 163, 0);
  gd_lvl_finish(g);

  /* --- 2. Polargeist: корабль и волна, гравитация, мини, дуал */
  li = gd_lvl_new(g, "Polargeist", "RobTop");
  gd_lvl_select(g, li);
  LV[li].official = 1; LV[li].diff = 3; LV[li].stars = 4;
  for (x = 6; x < 200; x += 1)                   /* потолок коридора */
    if (x > 20 && x < 172) add_o(g, GD_BLOCK, x, 8.0f);
  add_o(g, GD_SPIKE, 24, 0); add_o(g, GD_SPIKE, 30, 0);
  add_o(g, GD_P_SHIP, 40, 0);
  add_o(g, GD_COIN, 60, 3);
  add_o(g, GD_P_GRAVU, 80, 0);
  add_o(g, GD_P_GRAVD, 92, 0);
  add_o(g, GD_P_MINI, 100, 0);
  add_o(g, GD_P_BIG, 108, 0);
  add_o(g, GD_P_WAVE, 124, 0);
  add_o(g, GD_P_DUAL, 140, 0);
  add_o(g, GD_P_NODUAL, 160, 0);
  add_o(g, GD_P_CUBE, 170, 0);
  add_o(g, GD_SPIKE, 180, 0); add_o(g, GD_SPIKE, 181, 0);
  add_o(g, GD_COIN, 186, 0);
  gd_lvl_finish(g);

  /* --- 3. Trigger Lab: витрина триггеров (движение, цвет, пульс, счётчики) */
  li = gd_lvl_new(g, "Trigger Lab", "Derka");
  gd_lvl_select(g, li);
  LV[li].diff = 1; LV[li].stars = 2;
  /* платформа на группе 1 уезжает вверх */
  for (i = 0; i < 4; i++) { int o = add_o(g, GD_BLOCK, 30.0f + i, 4.0f); gd_obj_group(g, o, 1); }
  {
    int t = trg(g, GD_T_MOVE, 26, 3, 0);
    gd_obj_set(g, t, 0, 1);          /* группа 1 */
    gd_obj_set(g, t, 1, 0);          /* X = 0 юнитов */
    gd_obj_set(g, t, 2, 90);         /* Y = 90 юнитов = 3 блока */
    gd_obj_set(g, t, 3, 1.0f);       /* 1 секунда */
    gd_obj_set(g, t, 4, GD_EASE_INOUT);
  }
  /* вращение группы 2 */
  { int o = add_o(g, GD_OUTLINE, 50, 3); gd_obj_group(g, o, 2);
    int t = trg(g, GD_T_ROTATE, 46, 3, 0);
    gd_obj_set(g, t, 0, 2); gd_obj_set(g, t, 1, 180); gd_obj_set(g, t, 2, 1.5f); }
  /* прозрачность группы 3 */
  { int o = add_o(g, GD_BLOCK, 66, 4); gd_obj_group(g, o, 3);
    int t = trg(g, GD_T_ALPHA, 62, 3, 0);
    gd_obj_set(g, t, 0, 3); gd_obj_set(g, t, 1, 0.15f); gd_obj_set(g, t, 2, 1.0f); }
  /* цвет фона */
  { int t = trg(g, GD_T_COLOR, 78, 3, 0);
    gd_obj_set(g, t, 0, 0); gd_obj_set(g, t, 1, 90);
    gd_obj_set(g, t, 2, 20); gd_obj_set(g, t, 3, 120); gd_obj_set(g, t, 4, 1.0f); }
  /* пульс канала LINE */
  { int t = trg(g, GD_T_PULSE, 88, 3, 0);
    gd_obj_set(g, t, 0, 3); gd_obj_set(g, t, 1, 1.0f);
    gd_obj_set(g, t, 2, 0.2f); gd_obj_set(g, t, 3, 0.3f); gd_obj_set(g, t, 4, 1.0f); }
  /* pickup -> count -> spawn группы 4 */
  { int t = trg(g, GD_T_PICKUP, 100, 3, 0);
    gd_obj_set(g, t, 0, 1); gd_obj_set(g, t, 1, 3);
    t = trg(g, GD_T_COUNT, 102, 3, 0);
    gd_obj_set(g, t, 0, 1); gd_obj_set(g, t, 1, 3); gd_obj_set(g, t, 2, 4);
    /* группа 4 — триггер Show, группа 5 — спрятанная монета */
    t = trg(g, GD_T_SHOW, 104, 3, 4);
    gd_obj_set(g, t, 0, 5);
    { int o = add_o(g, GD_COIN, 110, 2); gd_obj_group(g, o, 5); LV[li].o[o].visible = 0; } }
  /* touch-триггер: тряска камеры */
  { int t = trg(g, GD_T_TOUCH, 120, 1, 0);
    gd_obj_set(g, t, 0, 5);
    t = trg(g, GD_T_SHAKE, 124, 3, 5);
    gd_obj_set(g, t, 0, 0.8f); }
  /* follow: группа 6 тянется за группой 7 */
  { int o = add_o(g, GD_DECO_GLOW, 140, 4); gd_obj_group(g, o, 7);
    o = add_o(g, GD_DECO_GLOW, 140, 1); gd_obj_group(g, o, 6);
    { int t = trg(g, GD_T_FOLLOW, 136, 3, 0);
      gd_obj_set(g, t, 0, 6); gd_obj_set(g, t, 1, 7); gd_obj_set(g, t, 2, 2.0f); } }
  /* toggle: прячем шипы группы 8 */
  { int o = add_o(g, GD_SPIKE, 160, 0); gd_obj_group(g, o, 8);
    int t = trg(g, GD_T_TOGGLE, 156, 3, 0);
    gd_obj_set(g, t, 0, 8); gd_obj_set(g, t, 1, 0); }
  add_o(g, GD_SPIKE, 170, 0);
  add_o(g, GD_ORB_Y, 175, 2);
  gd_lvl_finish(g);

  gd_lvl_select(g, 0);
}

/* ==================================================== симулятор и тесты */
#ifdef GD_SIM
#include <stdio.h>
#include <stdlib.h>

int in_grp_pub(const GDObj *o, int grp);

static int find_obj(const GDGame *g, float x, float y)
{
  int i;
  for (i = 0; i < g->L->nobj; i++)
    if (g->L->o[i].x == x && g->L->o[i].y == y) return i;
  return -1;
}

static float SLACK;
static int FAILS;

static void ck(int cond, const char *what)
{
  printf("  %s %s\n", cond ? "OK  " : "FAIL", what);
  if (!cond) FAILS++;
}

/* автопилот: куб/робот/паук прыгают перед препятствием,
 * корабль/волна/UFO/свинг держат середину коридора */
static int ai(const GDGame *g)
{
  const GDPlayer *p = &g->p[0];
  int i;
  if (p->mode == GD_SHIP || p->mode == GD_WAVE || p->mode == GD_UFO ||
      p->mode == GD_SWING)
    return p->y + p->vy * 0.20f < 4.0f;   /* упреждение, иначе влетаем в потолок */

  if (p->mode == GD_SPIDER) return 0;
  if (!p->grounded) return 0;      /* в воздухе решение уже принято */

  for (i = 0; i < g->L->nobj; i++) {
    const GDObj *o = &g->L->o[i];
    const GDInfo *inf = gd_info(o->id);
    float d, lead = 2.30f;
    if (o->x + o->ox > p->x + 9.0f) break;
    if (!inf || !o->visible) continue;
    if (inf->kind != 0 && inf->kind != 1 && inf->kind != 3) continue;  /* склоны проезжаем */
    if (o->x + o->ox + o->w * o->sx <= p->x + 0.3f) continue;
    if (o->y + o->oy >= 0.9f && p->y < 0.9f && inf->kind != 3) continue;
    d = (o->x + o->ox) - (p->x + 0.3f);
    if (inf->kind == 1) {
      /* считаем длину очереди шипов: чем длиннее, тем позже прыжок,
       * иначе приземлимся ровно в последний шип (окно прыжка 4.24 блока) */
      int n = 1, k;
      for (k = 1; k <= 4; k++)
        if (i + k < g->L->nobj && g->L->o[i + k].id == o->id &&
            g->L->o[i + k].y == o->y &&
            g->L->o[i + k].x <= o->x + o->w * k + 0.01f) n++;
        else break;
      lead = 2.60f - (float)(n - 1);
      if (lead < 0.30f) lead = 0.30f;
    } else if (inf->kind == 3) lead = 2.20f;
    else {
      int k;
      lead = (o->y + o->oy + o->h * o->sy <= 1.01f) ? 2.00f : 1.90f;
      for (k = 1; k <= 2; k++)
        if (i + k < g->L->nobj && g->L->o[i + k].id == o->id &&
            g->L->o[i + k].y == o->y &&
            g->L->o[i + k].x <= o->x + o->w * k + 0.01f) lead += 0.30f;
    }
    if (p->y > 0.5f) lead += 0.30f;
    return d < lead + SLACK;
  }
  return 0;
}

static int run_level(GDGame *g, int li, int maxframes, int *deaths)
{
  int f = 0, last = GD_PLAY, prev_hold;
  *deaths = 0;
  gd_start(g, li, 0);
  prev_hold = 0;
  while (g->phase != GD_WIN && f < maxframes) {
    int want = ai(g);
    if (want != prev_hold) { if (want) gd_press(g); else gd_release(g); prev_hold = want; }
    gd_update(g, 1.0f / 60.0f);
    if (g->phase == GD_DEAD && last != GD_DEAD) {
      (*deaths)++;
      prev_hold = 0;              /* gd_respawn сбрасывает hold */
      if (*deaths <= 3)
        printf("  смерть %d: x=%.2f y=%.2f режим=%s (%d%%) убийца=%d\n", *deaths,
               g->p[0].x, g->p[0].y, gd_mode_name(g->p[0].mode),
               (int)(gd_prog(g) * 100), g->killId);
    }
    last = g->phase;
    if (g->screen == GD_SCR_DONE) break;
    f++;
  }
  return g->screen == GD_SCR_DONE;
}

static void unit_tests(GDGame *g)
{
  GDLevel *L;
  int i, li, deaths = 0;
  char err[8], buf[GD_STORE], buf2[GD_STORE];
  float h;

  printf("— физика —\n");
  gd_start(g, 0, 0);
  gd_press(g);
  h = 0;
  for (i = 0; i < 240; i++) { gd_update(g, 1.0f / 60.0f); if (g->p[0].y > h) h = g->p[0].y; }
  ck(h > 2.0f && h < 2.5f, "высота прыжка куба ~2.2 блока");
  ck(g->p[0].vx > 10.3f && g->p[0].vx < 10.5f, "normal speed = 10.386 блока/с");

  printf("— орбы и пады —\n");
  gd_start(g, 0, 0);
  g->p[0].x = 46.4f; g->p[0].y = 1.5f; g->p[0].vy = 0; g->phase = GD_PLAY;
  gd_press(g); gd_update(g, 1.0f / 60.0f);
  ck(g->p[0].vy > 15.0f, "жёлтый орб даёт прыжок");
  gd_start(g, 0, 0);
  g->p[0].x = 76.4f; g->p[0].y = 1.5f; g->p[0].vy = 0;
  gd_press(g); gd_update(g, 1.0f / 60.0f);
  ck(g->p[0].vy > 8.0f && g->p[0].vy < 15.0f, "розовый орб — слабый прыжок");
  gd_start(g, 0, 0);
  g->p[0].x = 55.4f; g->p[0].y = 0.1f; g->p[0].vy = 0;
  gd_release(g); gd_update(g, 1.0f / 60.0f);
  ck(g->p[0].vy > 20.0f, "жёлтый пад срабатывает без нажатия");

  printf("— порталы —\n");
  gd_start(g, 1, 0);
  g->p[0].x = 40.4f; gd_update(g, 1.0f / 60.0f);
  ck(g->p[0].mode == GD_SHIP, "портал корабля меняет режим");
  g->p[0].x = 80.4f; gd_update(g, 1.0f / 60.0f);
  ck(g->p[0].grav == -1, "жёлтый портал переворачивает гравитацию");
  g->p[0].x = 92.4f; gd_update(g, 1.0f / 60.0f);
  ck(g->p[0].grav == 1, "синий портал возвращает гравитацию");
  g->p[0].x = 100.4f; gd_update(g, 1.0f / 60.0f);
  ck(g->p[0].mini == 1, "мини-портал уменьшает иконку");
  g->p[0].x = 124.4f; gd_update(g, 1.0f / 60.0f);
  ck(g->p[0].mode == GD_WAVE, "портал волны");
  g->p[0].x = 140.4f; gd_update(g, 1.0f / 60.0f);
  ck(g->np == 2 && g->p[1].grav == -g->p[0].grav, "дуал: два игрока с обратной гравитацией");

  printf("— триггеры —\n");
  gd_start(g, 2, 0);
  g->p[0].x = 26.5f; gd_update(g, 1.0f / 240.0f);
  for (i = 0; i < 240; i++) gd_update(g, 1.0f / 240.0f);
  {
    int found = 0;
    for (i = 0; i < g->L->nobj; i++)
      if (in_grp_pub(&g->L->o[i], 1) && g->L->o[i].oy > 2.5f && g->L->o[i].oy < 3.5f) found++;
    ck(found == 4, "Move: все 4 блока группы уехали на 90 юнитов = 3 блока");
  }
  gd_start(g, 2, 0);
  g->p[0].x = 46.5f;
  for (i = 0; i < 360; i++) gd_update(g, 1.0f / 240.0f);
  {
    int found = 0;
    for (i = 0; i < g->L->nobj; i++)
      if (in_grp_pub(&g->L->o[i], 2) && g->L->o[i].rot > 170.0f) found = 1;
    ck(found, "Rotate: группа повернулась на 180°");
  }
  gd_start(g, 2, 0);
  g->p[0].x = 62.5f;
  for (i = 0; i < 240; i++) gd_update(g, 1.0f / 240.0f);
  {
    int found = 0;
    for (i = 0; i < g->L->nobj; i++)
      if (in_grp_pub(&g->L->o[i], 3) && g->L->o[i].alpha < 0.3f) found = 1;
    ck(found, "Alpha: группа стала прозрачной");
  }
  gd_start(g, 2, 0);
  g->p[0].x = 78.5f;
  for (i = 0; i < 240; i++) gd_update(g, 1.0f / 240.0f);
  ck(g->L->col[0][2] > 0.35f, "Color: фон посинел");
  gd_start(g, 2, 0);
  g->p[0].x = 88.5f;
  for (i = 0; i < 60; i++) gd_update(g, 1.0f / 240.0f);
  ck(gd_pulse(g, 3) > 0.2f, "Pulse: канал LINE пульсирует");
  gd_start(g, 2, 0);
  g->p[0].x = 100.5f; gd_update(g, 1.0f / 240.0f);
  ck(g->items[1] == 3, "Pickup: счётчик item=1 стал 3");
  {
    int vis = 0;
    for (i = 0; i < g->L->nobj; i++)
      if (in_grp_pub(&g->L->o[i], 5) && g->L->o[i].visible) vis = 1;
    ck(vis, "Count -> Spawn -> Show: монета появилась после 3 пикапов");
  }
  gd_start(g, 2, 0);
  g->p[0].x = 156.5f; gd_update(g, 1.0f / 240.0f);
  {
    int hidden = 1;
    for (i = 0; i < g->L->nobj; i++)
      if (in_grp_pub(&g->L->o[i], 8) && g->L->o[i].visible) hidden = 0;
    ck(hidden, "Toggle: шипы группы 8 спрятаны");
  }

  printf("— редактор —\n");
  li = gd_lvl_new(g, "Editor Test", "Derka");
  gd_ed_init(g, li, GD_TOOL_BUILD, 0);
  g->curObj = GD_BLOCK;
  gd_ed_place(g, 10.4f, 0.4f);
  gd_ed_place(g, 11.6f, 0.6f);
  ck(g->L->nobj == 2, "постановка двух блоков");
  ck(g->L->o[0].x == 10.0f && g->L->o[1].x == 11.0f, "привязка к сетке");
  gd_ed_pick(g, 10.5f, 0.5f, 0);
  ck(g->nsel == 1, "выбор объекта");
  gd_ed_move_sel(g, 3.0f, 2.0f);
  ck(find_obj(g, 13.0f, 2.0f) >= 0, "перемещение выделения");
  gd_ed_pick(g, 13.5f, 2.5f, 0);
  gd_ed_rot_sel(g, 90.0f);
  ck(g->L->o[g->selected[0]].rot == 90.0f, "поворот на 90°");
  gd_ed_scale_sel(g, 2.0f);
  ck(g->L->o[g->selected[0]].sx == 2.0f, "масштаб x2");
  gd_ed_group_sel(g, 7);
  ck(g->L->o[g->selected[0]].ngrp == 1 && g->L->o[g->selected[0]].groups[0] == 7,
     "назначение группы");
  gd_ed_undo(g);
  ck(g->L->o[0].ngrp == 0, "undo откатил группу");
  gd_ed_undo(g);
  ck(g->L->o[0].sx == 1.0f, "undo откатил масштаб");
  gd_ed_pick(g, 13.5f, 2.5f, 0);
  gd_ed_copy_sel(g);
  gd_ed_paste(g, 20.0f, 0.0f);
  ck(g->L->nobj == 3, "copy/paste добавил объект");
  gd_ed_del_sel(g);
  ck(g->L->nobj == 2, "удаление выделения");
  gd_ed_box(g, 0, 0, 30, 5);
  ck(g->nsel == 2, "выделение рамкой");

  printf("— строка уровня —\n");
  {
    int n = gd_lvl_to_string(g, 0, buf, sizeof buf);
    int li2 = gd_lvl_new(g, "tmp", "tmp");
    int before = g->nlv;
    gd_lvl_select(g, li2);
    ck(n > 100 && gd_lvl_from_string(g, li2, buf), "уровень сохранился в строку");
    ck(g->L->nobj == LV[0].nobj, "число объектов совпало после загрузки");
    ck(before == g->nlv, "загрузка не плодит уровни");
  }

  printf("— аккаунты —\n");
  ck(gd_acc_register(g, "Derka", "secret1", err) == 0, "регистрация");
  ck(gd_acc_register(g, "Derka", "secret1", err) == -3, "повторный ник отклонён");
  ck(gd_acc_register(g, "ab", "secret1", err) == -1, "короткий ник отклонён");
  ck(gd_acc_register(g, "Vasya", "123", err) == -2, "короткий пароль отклонён");
  ck(g->loggedIn == 1 && g->curAcc == 0, "после регистрации вошли");
  gd_acc_logout(g);
  ck(gd_acc_login(g, "Derka", "wrong", err) == -2, "неверный пароль отклонён");
  ck(gd_acc_login(g, "Derka", "secret1", err) == 0, "вход по паролю");
  ck(g->loggedIn == 1, "сессия активна");

  printf("— сохранения —\n");
  {
    int n = gd_store_save(g, buf, sizeof buf);
    GDGame g2;
    ck(n > 50, "сохранение непустое");
    gd_init(&g2);
    g2.nlv = 0; g2.nacc = 0;
    gd_store_load(&g2, buf);
    ck(g2.nacc == g->nacc, "аккаунты восстановлены");
    ck(g2.nacc > 0 && g2.acc[0].hash == g->acc[0].hash, "хэш пароля совпал");
  }
  (void)L; (void)buf2; (void)deaths;
}

int in_grp_pub(const GDObj *o, int grp)
{
  int i;
  for (i = 0; i < o->ngrp; i++) if (o->groups[i] == (unsigned char)grp) return 1;
  return 0;
}

int main(int argc, char **argv)
{
  static GDGame g;
  int ok, li;

  SLACK = argc > 1 ? (float)atof(argv[1]) : 0.0f;
  gd_init(&g);
  printf("уровней: %d, объектов в первом: %d, %.0f блоков, %.0f сек, сдвиг тайминга %+.2f\n",
         g.nlv, LV[0].nobj, LV[0].len, LV[0].len / NORMAL, SLACK);

  for (li = 0; li < g.nlv; li++) {
    int d = 0;
    printf("— прохождение уровня %d (%s) —\n", li, LV[li].name);
    ok = run_level(&g, li, 60 * 400, &d);
    printf("  %s, попыток %d, прыжков %d, лучший %d%%, время %.1f c, монет %d/%d\n",
           ok ? "ПРОЙДЕН" : "НЕ ПРОЙДЕН", g.attempt, g.jumps,
           LV[li].best, g.t, g.coinsGot, LV[li].coins);
    if (!ok) { printf("  FAIL уровень %d не пройден автопилотом\n", li); FAILS++; }
  }

  unit_tests(&g);

  printf("итог: %s (%d проверок провалено)\n", FAILS ? "ПРОВАЛ" : "ВСЁ ОК", FAILS);
  return FAILS ? 1 : 0;
}
#endif
