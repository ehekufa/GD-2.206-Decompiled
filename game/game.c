/*
 * Geometry Dash for Derka — оболочка под Android: EGL + OpenGL ES 2 + native_app_glue.
 * Ассетов нет: вся картинка рисуется процедурно, кадр уходит одним glDrawArrays.
 *
 * Здесь только оболочка: меню с кнопками, список уровней, страница уровня, гараж,
 * аккаунты (регистрация/логин с экранной клавиатурой), настройки, редактор уровней,
 * игра и экран прохождения. Вся логика — в gd_core.c, она проверяется на хосте.
 */
#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "gd_core.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "gdderka", __VA_ARGS__))
#define MAXV    20000     /* вершин на кадр: сцена + интерфейс */
#define VIEW_H  10.0f     /* блоков по вертикали при zoom=1 */
#define PI      3.14159265f
#define NBTN     160      /* кнопок immediate-mode интерфейса */

typedef struct { float r, g, b, a; } C;
typedef struct { float x, y, r, g, b, a; } V;
typedef struct { int id; float x, y, w, h; } Btn;

typedef struct {
  struct android_app *app;
  EGLDisplay dpy;
  EGLSurface surf;
  EGLContext ctx;
  GLuint prog, vbo;
  int ready, anim, W, H, logged;
  double last;
  GDGame g;
  /* ввод и интерфейс */
  float tx, ty, sx, sy;         /* текущая и стартовая точка касания */
  int tdown, drag, moved;
  Btn btn[NBTN]; int nbtn;
  int popup, popupSlot;         /* открытый попап и редактируемый параметр */
  int field;                    /* 0 = ник, 1 = пароль */
  char nm[GD_STR], pw[GD_STR];
  char msg[48];
  float msgT;
  int scroll;                   /* прокрутка списка уровней */
  int page;                     /* выбранный уровень */
  int garageMode;
} Eng;

static V vb[MAXV];
static int vn;
static int gW, gH;
static float SC, GY, CX, CY;    /* пикселей в блоке, Y земли, камера */

/* ------------------------------------------------------- шрифт 3x5 (A-Z 0-9 и знаки) */
static const char *FONT[] = {
  ".#." "#.#" "#.#" "###" "#.#",  /* A */  "###" "#.#" "##." "#.#" "##.",  /* B */
  "###" "#.." "#.." "#.." "###",  /* C */  "##." "#.#" "#.#" "#.#" "##.",  /* D */
  "###" "#.." "##." "#.." "###",  /* E */  "###" "#.." "##." "#.." "#..",  /* F */
  "###" "#.." "#.#" "#.#" "###",  /* G */  "#.#" "#.#" "###" "#.#" "#.#",  /* H */
  "###" ".#." ".#." ".#." "###",  /* I */  "..#" "..#" "..#" "#.#" "###",  /* J */
  "#.#" "#.#" "##." "#.#" "#.#",  /* K */  "#.." "#.." "#.." "#.." "###",  /* L */
  "#.#" "###" "#.#" "#.#" "#.#",  /* M */  "##." "#.#" "#.#" "#.#" "#.#",  /* N */
  "###" "#.#" "#.#" "#.#" "###",  /* O */  "###" "#.#" "###" "#.." "#..",  /* P */
  "###" "#.#" "#.#" "###" "..#",  /* Q */  "###" "#.#" "###" "#.#" "#.#",  /* R */
  "###" "#.." "###" "..#" "###",  /* S */  "###" ".#." ".#." ".#." ".#.",  /* T */
  "#.#" "#.#" "#.#" "#.#" "###",  /* U */  "#.#" "#.#" "#.#" "#.#" ".#.",  /* V */
  "#.#" "#.#" "###" "###" "#.#",  /* W */  "#.#" "#.#" ".#." "#.#" "#.#",  /* X */
  "#.#" "#.#" ".#." ".#." ".#.",  /* Y */  "###" "..#" ".#." "#.." "###",  /* Z */
  "###" "#.#" "#.#" "#.#" "###",  /* 0 */  ".#." "##." ".#." ".#." "###",  /* 1 */
  "###" "..#" "###" "#.." "###",  /* 2 */  "###" "..#" "###" "..#" "###",  /* 3 */
  "#.#" "#.#" "###" "..#" "..#",  /* 4 */  "###" "#.." "###" "..#" "###",  /* 5 */
  "###" "#.." "###" "#.#" "###",  /* 6 */  "###" "..#" "..#" "..#" "..#",  /* 7 */
  "###" "#.#" "###" "#.#" "###",  /* 8 */  "###" "#.#" "###" "..#" "###",  /* 9 */
  "..." "..." "..." "..." "...",  /* пробел */
  "#.#" "..#" ".#." "#.." "#.#",  /* % */
  ".#." ".#." ".#." "..." ".#.",  /* ! */
  "..." "..." "..." "..." ".#.",  /* . */
  "..." ".#." "..." ".#." "...",  /* : */
  "..." "..." "###" "..." "...",  /* - */
  "..." ".#." "###" ".#." "...",  /* + */
  "..#" ".#." "..#" ".#." "..#",  /* < */  /* стрелка влево */
  "#.." ".#." "#.." ".#." "#..",  /* > */  /* стрелка вправо */
  "###" "..#" ".#." "..." ".#.",  /* ? */
  "###" "#.#" "###" "#.#" "#.#",  /* = (решётка) */
  "#.#" ".#." ".#." ".#." "#.#",  /* * (звёздочка) */
  "..#" ".#." ".#." ".#." "..#",  /* ( */
  "#.." ".#." ".#." ".#." "#..",  /* ) */
  "..." "..." "..." "..." ".#.",  /* , */
  "..#" "..#" "###" "#.." "#..",  /* / */
  ".#." "#.#" "#.#" "#.#" ".#.",  /* o в кружке — для орбов */
  "###" "#.#" "#.#" "###" "#.#",  /* R для Robot */
};

static int fidx(int c)
{
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= '0' && c <= '9') return 26 + c - '0';
  switch (c) {
    case '%': return 37; case '!': return 38; case '.': return 39;
    case ':': return 40; case '-': return 41; case '+': return 42;
    case '<': return 43; case '>': return 44; case '?': return 45;
    case '#': return 46; case '*': return 47; case '(': return 48;
    case ')': return 49; case ',': return 50; case '/': return 51;
  }
  return 36;
}

/* ------------------------------------------------------------- примитивы */

static C C4(float r, float g, float b, float a) { C c; c.r = r; c.g = g; c.b = b; c.a = a; return c; }

static C mix(C a, C b, float t)
{
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  return C4(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
}

static C ch(const GDLevel *L, int i, float a)
{
  return C4(L->col[i][0], L->col[i][1], L->col[i][2], a);
}

/* координаты в пикселях, начало в левом нижнем углу */
static void vp(float x, float y, C c)
{
  if (vn >= MAXV) return;
  vb[vn].x = x / (0.5f * gW) - 1.0f;
  vb[vn].y = y / (0.5f * gH) - 1.0f;
  vb[vn].r = c.r; vb[vn].g = c.g; vb[vn].b = c.b; vb[vn].a = c.a;
  vn++;
}

static void tri(float x0, float y0, float x1, float y1, float x2, float y2, C c)
{
  vp(x0, y0, c); vp(x1, y1, c); vp(x2, y2, c);
}

static void quad(float x, float y, float w, float h, C c)
{
  tri(x, y, x + w, y, x + w, y + h, c);
  tri(x, y, x + w, y + h, x, y + h, c);
}

static void q4(float x0, float y0, float x1, float y1,
               float x2, float y2, float x3, float y3, C c)
{
  tri(x0, y0, x1, y1, x2, y2, c);
  tri(x0, y0, x2, y2, x3, y3, c);
}

static void vgrad(float x, float y, float w, float h, C bot, C top)
{
  vp(x, y, bot); vp(x + w, y, bot); vp(x + w, y + h, top);
  vp(x, y, bot); vp(x + w, y + h, top); vp(x, y + h, top);
}

static void disc(float cx, float cy, float r, C c, int seg)
{
  int i;
  for (i = 0; i < seg; i++) {
    float a0 = (float)i / seg * 6.28318f, a1 = (float)(i + 1) / seg * 6.28318f;
    tri(cx, cy, cx + cosf(a0) * r, cy + sinf(a0) * r,
        cx + cosf(a1) * r, cy + sinf(a1) * r, c);
  }
}

static void ring(float cx, float cy, float r0, float r1, C c, int seg)
{
  int i;
  for (i = 0; i < seg; i++) {
    float a0 = (float)i / seg * 6.28318f, a1 = (float)(i + 1) / seg * 6.28318f;
    float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
    q4(cx + c0 * r0, cy + s0 * r0, cx + c1 * r0, cy + s1 * r0,
       cx + c1 * r1, cy + s1 * r1, cx + c0 * r1, cy + s0 * r1, c);
  }
}

/* мировые координаты: X и Y в блоках от камеры */
static float PXf(float wx) { return (wx - CX) * SC; }
static float PYf(float wy) { return GY + (wy - CY) * SC; }

static void wquad(float wx, float wy, float ww, float wh, C c)
{
  quad(PXf(wx), PYf(wy), ww * SC, wh * SC, c);
}

static void wtri(float x0, float y0, float x1, float y1, float x2, float y2, C c)
{
  tri(PXf(x0), PYf(y0), PXf(x1), PYf(y1), PXf(x2), PYf(y2), c);
}

static void wdisc(float wx, float wy, float wr, C c)
{
  disc(PXf(wx), PYf(wy), wr * SC, c, 18);
}

/* повёрнутый квадрат в мире: центр, размер, угол в градусах */
static void rq(float cx, float cy, float s, float ang, C c)
{
  float r = s * SC * 0.5f, a = ang * PI / 180.0f;
  float co = cosf(a) * r, si = sinf(a) * r;
  float x = PXf(cx), y = PYf(cy);
  q4(x - co + si, y - si - co, x + co + si, y + si - co,
     x + co - si, y + si + co, x - co - si, y - si + co, c);
}

static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

static float tadv(float sz) { return sz * 0.8f; }

/* строка по центру cx, низ букв на y, высота буквы sz */
static void text(const char *s, float cx, float y, float sz, C c)
{
  float u = sz / 5.0f, adv = tadv(sz);
  float x = cx - adv * slen(s) * 0.5f;
  int i, r, k;

  for (i = 0; s[i]; i++, x += adv) {
    const char *g = FONT[fidx((unsigned char)s[i])];
    for (r = 0; r < 5; r++)
      for (k = 0; k < 3; k++)
        if (g[r * 3 + k] == '#') quad(x + k * u, y + (4 - r) * u, u + 0.5f, u + 0.5f, c);
  }
}

/* строка от левого края */
static void textL(const char *s, float x, float y, float sz, C c)
{
  float u = sz / 5.0f, adv = tadv(sz);
  int i, r, k;

  for (i = 0; s[i]; i++, x += adv) {
    const char *g = FONT[fidx((unsigned char)s[i])];
    for (r = 0; r < 5; r++)
      for (k = 0; k < 3; k++)
        if (g[r * 3 + k] == '#') quad(x + k * u, y + (4 - r) * u, u + 0.5f, u + 0.5f, c);
  }
}

static void textsh(const char *s, float cx, float y, float sz, C c)
{
  text(s, cx + sz * 0.12f, y - sz * 0.12f, sz, C4(0.0f, 0.0f, 0.0f, 0.45f));
  text(s, cx, y, sz, c);
}

static void fmt(char *b, const char *pre, float v, int dec, const char *post)
{
  char t[24];
  int n = 0, i, ip, k;
  float av = v < 0 ? -v : v;
  int mult = 1;
  for (i = 0; i < dec; i++) mult *= 10;
  ip = (int)(av * mult + 0.5f);
  if (v < 0) t[n++] = '-';
  {
    char d[16];
    int m = 0;
    do { d[m++] = (char)('0' + ip % 10); ip /= 10; } while (ip && m < 14);
    for (k = m - 1; k >= 0; k--) {
      t[n++] = d[k];
      if (dec && k == dec) t[n++] = '.';
    }
  }
  t[n] = 0;
  b[0] = 0;
  if (pre) strcat(b, pre);
  strcat(b, t);
  if (post) strcat(b, post);
}

static void fmti(char *b, const char *pre, int v, const char *post)
{
  fmt(b, pre, (float)v, 0, post);
}

/* --------------------------------------------------------- кнопки (immediate mode) */

static void save_store(Eng *e);
static void load_store(Eng *e);

/* зарегистрировать область нажатия без отрисовки (строки списков) */
static void breg(Eng *e, int id, float x, float y, float w, float h)
{
  if (e->nbtn >= NBTN) return;
  e->btn[e->nbtn].id = id; e->btn[e->nbtn].x = x; e->btn[e->nbtn].y = y;
  e->btn[e->nbtn].w = w; e->btn[e->nbtn].h = h;
  e->nbtn++;
}

static void scat(char *b, const char *a, const char *c)
{
  b[0] = 0;
  if (a) strcat(b, a);
  if (c) strcat(b, c);
}

static int hit(int id, Eng *e, float x, float y)
{
  int i;
  for (i = 0; i < e->nbtn; i++)
    if (e->btn[i].id == id)
      return x >= e->btn[i].x && x <= e->btn[i].x + e->btn[i].w &&
             y >= e->btn[i].y && y <= e->btn[i].y + e->btn[i].h;
  return 0;
}

/* кнопка в экранных координатах (Y снизу), возвращает 1 если нажата сейчас */
static int button(Eng *e, int id, float x, float y, float w, float h,
                  const char *label, C bg, C fg, float tsz)
{
  int pressed = 0;
  if (e->nbtn < NBTN) {
    e->btn[e->nbtn].id = id; e->btn[e->nbtn].x = x; e->btn[e->nbtn].y = y;
    e->btn[e->nbtn].w = w; e->btn[e->nbtn].h = h;
    e->nbtn++;
  }
  quad(x, y, w, h, C4(0, 0, 0, 0.35f));
  quad(x + 2, y + 2, w - 4, h - 4, bg);
  quad(x + 2, y + h * 0.55f, w - 4, h * 0.45f - 2, C4(1, 1, 1, 0.12f));
  if (label && label[0]) textsh(label, x + w * 0.5f, y + h * 0.5f - tsz * 0.5f, tsz, fg);
  if (e->tdown && e->tx >= x && e->tx <= x + w && e->ty >= y && e->ty <= y + h) pressed = 1;
  return pressed;
}

static void say(Eng *e, const char *s)
{
  int i;
  for (i = 0; i < 47 && s[i]; i++) e->msg[i] = s[i];
  e->msg[i] = 0;
  e->msgT = 2.5f;
}

/* ------------------------------------------------------------- фон и земля */

static void draw_world(Eng *e)
{
  GDGame *g = &e->g;
  GDLevel *L = g->L;
  float x0, x1, wx, u, pulse;
  int i;
  C bg0 = ch(L, 0, 1), bg1 = mix(ch(L, 0, 1), ch(L, 1, 1), 0.75f);
  C line = ch(L, 3, 1);
  C obj = ch(L, 4, 1);

  pulse = gd_pulse(g, 3);
  if (pulse > 0.0f) line = mix(line, C4(1, 1, 1, 1), pulse);
  if (gd_pulse(g, 0) > 0.0f) bg0 = mix(bg0, C4(1, 1, 1, 1), gd_pulse(g, 0));

  vgrad(0, 0, (float)e->W, (float)e->H, bg1, bg0);

  /* диагональные полосы фона */
  for (i = 0; i < 9; i++) {
    float p = (float)i / 9.0f;
    float off = fmodf(g->t * 12.0f + p * 400.0f, (float)e->H + 300.0f);
    C c = mix(bg1, C4(1, 1, 1, 0.06f), 0.5f);
    q4(-100, off, (float)e->W + 100, off + 120,
       (float)e->W + 100, off + 180, -100, off + 60, c);
  }

  x0 = CX - 1.0f;
  x1 = CX + (float)e->W / SC + 1.0f;

  /* сетка земли */
  for (wx = (float)(int)x0; wx < x1; wx += 1.0f) {
    float px = PXf(wx);
    quad(px, 0, 1.2f, PYf(0.0f), C4(0, 0, 0, 0.18f));
  }
  /* земля */
  vgrad(0, PYf(0.0f) - 400, (float)e->W, 400, mix(ch(L, 2, 1), C4(0, 0, 0, 1), 0.5f), ch(L, 2, 1));
  quad(0, PYf(0.0f) - 3, (float)e->W, 3, line);
  for (wx = (float)(int)x0; wx < x1; wx += 1.0f)
    quad(PXf(wx), PYf(0.0f) - 40, 1.2f, 40, C4(0, 0, 0, 0.12f));
  /* потолок уровня */
  quad(0, PYf(12.0f), (float)e->W, 3, line);

  (void)obj; (void)u;
}

/* ------------------------------------------------------------- объекты */

static void draw_obj(Eng *e, const GDObj *o, float t)
{
  GDLevel *L = e->g.L;
  const GDInfo *inf = gd_info(o->id);
  float x = o->x + o->ox, y = o->y + o->oy;
  float w = o->w * o->sx, h = o->h * o->sy;
  C body, edge;
  float a = o->alpha;
  if (!inf || a <= 0.01f) return;

  body = C4(inf->cr, inf->cg, inf->cb, a);
  edge = C4(0.05f, 0.06f, 0.12f, a);

  switch (inf->kind) {
    case 0:   /* блок */
      wquad(x, y, w, h, body);
      wquad(x, y + h - 0.08f, w, 0.08f, mix(body, C4(1, 1, 1, 1), 0.35f));
      wquad(x + 0.06f, y + 0.06f, w - 0.12f, h - 0.12f, mix(body, C4(0, 0, 0, 1), 0.35f));
      break;
    case 2:   /* склон */
      if (o->id == GD_SLOPE_UP) {
        wtri(x, y, x + w, y, x + w, y + h, body);
        wtri(x, y, x + w, y + h, x + w, y + h - 0.08f, mix(body, C4(1, 1, 1, 1), 0.3f));
      } else {
        wtri(x, y, x + w, y, x, y + h, body);
        wtri(x, y + h, x + w, y, x, y + h - 0.08f, mix(body, C4(1, 1, 1, 1), 0.3f));
      }
      break;
    case 1: { /* шип */
      int flip = (o->rot > 90.0f && o->rot < 270.0f);
      C c = mix(body, ch(L, 4, 1), 0.25f);
      if (flip) {
        wtri(x + 0.08f, y + h, x + w - 0.08f, y + h, x + w * 0.5f, y + h * 0.1f, c);
        wtri(x + 0.2f, y + h - 0.06f, x + w - 0.2f, y + h - 0.06f, x + w * 0.5f, y + h * 0.2f, edge);
      } else {
        wtri(x + 0.08f, y, x + w - 0.08f, y, x + w * 0.5f, y + h * 0.9f, c);
        wtri(x + 0.22f, y + 0.04f, x + w - 0.22f, y + 0.04f, x + w * 0.5f, y + h * 0.6f, edge);
      }
      break;
    }
    case 3: { /* пила */
      float cx = x + w * 0.5f, cy = y + h * 0.5f, r = w * 0.5f;
      int k;
      wdisc(cx, cy, r * 0.95f, C4(0.85f, 0.88f, 1.0f, a));
      wdisc(cx, cy, r * 0.55f, C4(0.25f, 0.28f, 0.45f, a));
      for (k = 0; k < 8; k++) {
        float ang = t * 540.0f + k * 45.0f;
        rq(cx + cosf(ang * PI / 180.0f) * r * 0.72f,
           cy + sinf(ang * PI / 180.0f) * r * 0.72f, r * 0.34f, ang, C4(0.9f, 0.93f, 1.0f, a));
      }
      break;
    }
    case 4: { /* орб */
      float cx = x + w * 0.5f, cy = y + h * 0.5f;
      float pr = 0.30f + 0.03f * sinf(t * 6.0f);
      wdisc(cx, cy, pr + 0.10f, C4(body.r, body.g, body.b, a * 0.30f));
      ring(PXf(cx), PYf(cy), pr * SC * 0.72f, pr * SC, C4(1, 1, 1, a * 0.9f), 18);
      wdisc(cx, cy, pr * 0.72f, body);
      break;
    }
    case 5:   /* пад */
      wtri(x + 0.1f, y, x + w - 0.1f, y, x + w * 0.5f, y + h * 0.8f, body);
      wquad(x, y, w, 0.08f, C4(1, 1, 1, a * 0.7f));
      break;
    case 6: { /* портал */
      float cx = x + w * 0.5f;
      int k;
      wquad(x + 0.25f, y, 0.5f, h, C4(body.r, body.g, body.b, a * 0.28f));
      for (k = 0; k < 2; k++) {
        float yy = y + (k ? h * 0.78f : h * 0.22f);
        ring(PXf(cx), PYf(yy), 0.10f * SC, 0.26f * SC, C4(body.r, body.g, body.b, a), 14);
        wdisc(cx, yy, 0.10f, C4(1, 1, 1, a * 0.85f));
      }
      quad(PXf(x + 0.30f), PYf(y), 0.40f * SC, h * SC, C4(body.r, body.g, body.b, a * 0.5f));
      break;
    }
    case 7: { /* монета */
      float cx = x + w * 0.5f, cy = y + h * 0.5f;
      wdisc(cx, cy, 0.32f, C4(1.0f, 0.80f, 0.15f, a));
      ring(PXf(cx), PYf(cy), 0.20f * SC, 0.32f * SC, C4(1.0f, 0.95f, 0.5f, a), 16);
      wdisc(cx, cy, 0.14f, C4(0.85f, 0.60f, 0.10f, a));
      break;
    }
    case 8:   /* декор */
      wquad(x + 0.1f, y, w - 0.2f, h, C4(body.r, body.g, body.b, a * 0.5f));
      break;
    case 10:  /* старт / чекпоинт */
      wquad(x + 0.42f, y, 0.16f, h, C4(body.r, body.g, body.b, a * 0.9f));
      wtri(x + 0.42f, y + h * 0.6f, x + 0.42f, y + h, x + 0.9f, y + h * 0.8f, body);
      break;
    case 11: { /* letter-блок D/J/S/H/F */
      const char *lt = "DJS HF";
      char b[2] = { 'D', 0 };
      int k = o->id - GD_LBL_D;
      wquad(x, y, w, h, C4(body.r, body.g, body.b, a * 0.35f));
      ring(PXf(x + w * 0.5f), PYf(y + h * 0.5f), 0.30f * SC, 0.36f * SC, C4(1, 1, 1, a * 0.8f), 14);
      if (k >= 0 && k < 5) b[0] = lt[k];
      text(b, PXf(x + w * 0.5f), PYf(y + h * 0.5f) - 0.14f * SC, 0.28f * SC, C4(1, 1, 1, a));
      break;
    }
    case 9: { /* триггер: виден только в редакторе */
      if (e->g.screen != GD_SCR_EDITOR) return;
      wquad(x, y, w, h, C4(body.r, body.g, body.b, 0.35f));
      ring(PXf(x + w * 0.5f), PYf(y + h * 0.5f), 0.18f * SC, 0.26f * SC, C4(1, 1, 1, 0.8f), 12);
      text(inf->en, PXf(x + w * 0.5f), PYf(y) - 0.22f * SC, 0.18f * SC, C4(1, 1, 1, 0.85f));
      break;
    }
    default: break;
  }
}

/* ------------------------------------------------------------- игрок */

static void draw_player(Eng *e, const GDPlayer *p, C c1, C c2, int glow)
{
  float s = p->mini ? 0.62f : 1.0f;
  float cx = p->x, cy = p->y + s * 0.5f;
  C dark = C4(0.05f, 0.06f, 0.12f, 1);

  if (glow) wdisc(cx, cy, s * 0.75f, C4(c1.r, c1.g, c1.b, 0.18f));

  switch (p->mode) {
    case GD_CUBE: case GD_ROBOT: case GD_SPIDER:
      rq(cx, cy, s * 0.95f, p->rot, dark);
      rq(cx, cy, s * 0.82f, p->rot, c1);
      rq(cx, cy, s * 0.50f, p->rot, c2);
      {
        float a = p->rot * PI / 180.0f;
        float ex = cosf(a), ey = sinf(a);
        wdisc(cx + ex * s * 0.18f + ey * s * 0.16f, cy + ey * s * 0.18f - ex * s * 0.16f,
              s * 0.09f, dark);
        wdisc(cx + ex * s * 0.18f - ey * s * 0.16f, cy + ey * s * 0.18f + ex * s * 0.16f,
              s * 0.09f, dark);
      }
      if (p->mode == GD_ROBOT || p->mode == GD_SPIDER) {
        wquad(cx - s * 0.34f, p->y - s * 0.22f, s * 0.18f, s * 0.24f, dark);
        wquad(cx + s * 0.16f, p->y - s * 0.22f, s * 0.18f, s * 0.24f, dark);
      }
      break;
    case GD_SHIP: case GD_SWING:
      rq(cx, cy, s * 0.55f, p->rot, c1);
      {
        float a = p->rot * PI / 180.0f;
        float dx = cosf(a), dy = sinf(a);
        wtri(cx - dx * s * 0.5f - dy * s * 0.35f, cy - dy * s * 0.5f + dx * s * 0.35f,
             cx + dx * s * 0.55f, cy + dy * s * 0.55f,
             cx - dx * s * 0.5f + dy * s * 0.35f, cy - dy * s * 0.5f - dx * s * 0.35f, c2);
        wdisc(cx + dx * s * 0.5f, cy + dy * s * 0.5f, s * 0.16f, C4(1, 1, 1, 0.9f));
        if (e->g.hold)
          wdisc(cx - dx * s * 0.55f, cy - dy * s * 0.55f, s * 0.16f, C4(1.0f, 0.7f, 0.2f, 0.85f));
      }
      break;
    case GD_BALL:
      wdisc(cx, cy, s * 0.46f, dark);
      wdisc(cx, cy, s * 0.38f, c1);
      {
        float a = p->rot * PI / 180.0f;
        quad(PXf(cx) - cosf(a) * s * SC * 0.38f, PYf(cy) - sinf(a) * s * SC * 0.38f,
             s * SC * 0.76f, 2.0f, c2);
      }
      break;
    case GD_UFO:
      wdisc(cx, cy + s * 0.1f, s * 0.42f, C4(c2.r, c2.g, c2.b, 0.85f));
      wquad(cx - s * 0.45f, cy - s * 0.30f, s * 0.9f, s * 0.22f, c1);
      wdisc(cx, cy + s * 0.16f, s * 0.16f, C4(1, 1, 1, 0.9f));
      break;
    case GD_WAVE:
      {
        float a = p->rot * PI / 180.0f, dx = cosf(a), dy = sinf(a);
        wtri(cx + dx * s * 0.5f, cy + dy * s * 0.5f,
             cx - dx * s * 0.3f + dy * s * 0.28f, cy - dy * s * 0.3f - dx * s * 0.28f,
             cx - dx * s * 0.3f - dy * s * 0.28f, cy - dy * s * 0.3f + dx * s * 0.28f, c1);
        wtri(cx - dx * s * 0.3f, cy - dy * s * 0.3f,
             cx - dx * s * 1.6f, cy - dy * s * 1.6f + s * 0.06f,
             cx - dx * s * 1.6f, cy - dy * s * 1.6f - s * 0.06f, C4(c2.r, c2.g, c2.b, 0.5f));
      }
      break;
    default: break;
  }
}

/* ------------------------------------------------------------- игра */

static void draw_game(Eng *e)
{
  GDGame *g = &e->g;
  GDLevel *L = g->L;
  char b[64];
  float prog, bx, by, bw, bh;
  int i;
  C c1 = ch(L, 5, 1), c2 = ch(L, 6, 1);
  int acc = g->loggedIn ? g->curAcc : -1;

  if (g->shake > 0.0f) {
    CX += sinf(g->t * 97.0f) * 0.20f * g->shake;
    GY += cosf(g->t * 83.0f) * 11.0f * g->shake;
  }
  draw_world(e);

  for (i = 0; i < L->nobj; i++) {
    const GDObj *o = &L->o[i];
    if (!o->visible) continue;
    if (o->x + o->ox > CX + (float)e->W / SC + 1.0f) break;
    if (o->x + o->ox + o->w * o->sx < CX - 1.0f) continue;
    draw_obj(e, o, g->t);
  }

  for (i = 0; i < g->npart; i++) {
    GDPart *p = &g->part[i];
    float a = p->life > 1.0f ? 1.0f : p->life;
    wquad(p->x - p->r, p->y - p->r, p->r * 2, p->r * 2, C4(p->cr, p->cg, p->cb, a));
  }

  if (acc >= 0 && acc < g->nacc) {
    c1 = C4(0.2f + 0.13f * g->acc[acc].c1, 0.5f, 1.0f - 0.11f * g->acc[acc].c1, 1);
    c2 = C4(1.0f - 0.11f * g->acc[acc].c2, 0.5f, 0.2f + 0.13f * g->acc[acc].c2, 1);
  }
  for (i = 0; i < g->np; i++) draw_player(e, &g->p[i], c1, c2, acc >= 0 ? g->acc[acc].glow : 0);

  /* HUD: прогресс-бар */
  prog = gd_prog(g);
  bw = (float)e->W * 0.42f; bh = 14.0f;
  bx = ((float)e->W - bw) * 0.5f; by = (float)e->H - 46.0f;
  quad(bx - 2, by - 2, bw + 4, bh + 4, C4(0, 0, 0, 0.5f));
  quad(bx, by, bw, bh, C4(1, 1, 1, 0.18f));
  quad(bx, by, bw * prog, bh, C4(0.35f, 1.0f, 0.45f, 1));
  fmt(b, "", prog * 100.0f, 0, "%");
  textsh(b, bx + bw * 0.5f, by + 3, 12, C4(1, 1, 1, 1));

  fmti(b, "ATTEMPT ", g->attempt + 1, "");
  textsh(b, 90.0f, (float)e->H - 60.0f, 13, C4(1, 1, 1, 0.85f));
  fmti(b, "BEST ", L->best, "%");
  textsh(b, (float)e->W - 90.0f, (float)e->H - 60.0f, 13, C4(1, 1, 1, 0.85f));
  fmti(b, "* ", g->coinsGot, "");
  textsh(b, 60.0f, (float)e->H - 84.0f, 13, C4(1.0f, 0.85f, 0.2f, 1));
  if (g->practice) textsh("PRACTICE", (float)e->W * 0.5f, 30.0f, 13, C4(0.4f, 1.0f, 0.6f, 0.9f));

  /* кнопка паузы */
  if (button(e, 900, (float)e->W - 74.0f, 24.0f, 52.0f, 52.0f, "", C4(0.2f, 0.3f, 0.6f, 0.85f),
             C4(1, 1, 1, 1), 14))
    e->popup = 2;
  quad((float)e->W - 60.0f, 40.0f, 8, 20, C4(1, 1, 1, 1));
  quad((float)e->W - 46.0f, 40.0f, 8, 20, C4(1, 1, 1, 1));

  if (g->flash > 0.0f)
    quad(0, 0, (float)e->W, (float)e->H, C4(g->fr, g->fg, g->fb, g->flash * 0.6f));
  if (g->phase == GD_DEAD && g->deadt < 0.25f)
    quad(0, 0, (float)e->W, (float)e->H, C4(1, 1, 1, 0.5f * (1.0f - g->deadt / 0.25f)));
}

/* ------------------------------------------------------------- экраны меню */

static const char *DIFF[] = { "NA", "EASY", "NORMAL", "HARD", "HARDER", "INSANE", "DEMON", "DEMON" };

static void head(Eng *e, const char *title)
{
  quad(0, (float)e->H - 70, (float)e->W, 70, C4(0, 0, 0, 0.35f));
  textsh(title, (float)e->W * 0.5f, (float)e->H - 46, 26, C4(1, 1, 1, 1));
}

static void draw_menu(Eng *e)
{
  GDGame *g = &e->g;
  GDLevel *L = g->L;
  float cx = (float)e->W * 0.5f, w = 300, h = 64, y;
  char b[64];

  vgrad(0, 0, (float)e->W, (float)e->H, C4(0.06f, 0.09f, 0.28f, 1), C4(0.16f, 0.05f, 0.30f, 1));
  {
    int i;
    for (i = 0; i < 12; i++) {
      float off = fmodf(g->t * 18.0f + i * 90.0f, (float)e->H + 200.0f);
      q4(-80, off, (float)e->W + 80, off + 60, (float)e->W + 80, off + 110, -80, off + 50,
         C4(1, 1, 1, 0.045f));
    }
  }

  textsh("GEOMETRY DASH", cx, (float)e->H - 110, 40, C4(1, 1, 1, 1));
  textsh("FOR DERKA", cx, (float)e->H - 142, 16, C4(0.6f, 0.9f, 1.0f, 1));

  y = (float)e->H - 230;
  if (button(e, 1, cx - w * 0.5f, y, w, h, "PLAY", C4(0.20f, 0.55f, 0.95f, 1), C4(1, 1, 1, 1), 22))
    gd_scr(g, GD_SCR_LEVELS);
  y -= h + 14;
  if (button(e, 2, cx - w * 0.5f, y, w, h, "CREATOR", C4(0.30f, 0.75f, 0.35f, 1), C4(1, 1, 1, 1), 22)) {
    if (g->nlv < GD_MAX_LVL) {
      int li = gd_lvl_new(g, "New Level", g->loggedIn ? g->acc[g->curAcc].name : "Derka");
      gd_lvl_select(g, li);
      gd_ed_init(g, li, GD_TOOL_BUILD, 0);
      g->curObj = GD_BLOCK;
      gd_scr(g, GD_SCR_EDITOR);
    }
  }
  y -= h + 14;
  if (button(e, 3, cx - w * 0.5f, y, w * 0.5f - 7, h, "GARAGE", C4(0.85f, 0.55f, 0.20f, 1),
             C4(1, 1, 1, 1), 18))
    gd_scr(g, GD_SCR_GARAGE);
  if (button(e, 4, cx + 7, y, w * 0.5f - 7, h, "MORE", C4(0.55f, 0.35f, 0.85f, 1),
             C4(1, 1, 1, 1), 18))
    gd_scr(g, GD_SCR_SETTINGS);

  /* панель аккаунта */
  y -= h + 22;
  quad(cx - w * 0.5f, y - 6, w, 54, C4(0, 0, 0, 0.3f));
  if (g->loggedIn && g->curAcc >= 0) {
    GDAcc *a = &g->acc[g->curAcc];
    textL(a->name, cx - w * 0.5f + 14, y + 30, 16, C4(1, 1, 1, 1));
    fmti(b, "STARS ", a->stars, "");
    textL(b, cx - w * 0.5f + 14, y + 8, 12, C4(0.8f, 0.9f, 1.0f, 1));
    fmti(b, "ORBS ", a->orbs, "");
    textL(b, cx - w * 0.5f + 130, y + 8, 12, C4(0.8f, 0.9f, 1.0f, 1));
    if (button(e, 5, cx + w * 0.5f - 92, y + 8, 80, 38, "PROFILE", C4(0.2f, 0.4f, 0.8f, 1),
               C4(1, 1, 1, 1), 12))
      gd_scr(g, GD_SCR_ACCOUNT);
  } else {
    textL("NOT LOGGED IN", cx - w * 0.5f + 14, y + 20, 14, C4(0.8f, 0.8f, 0.8f, 1));
    if (button(e, 5, cx + w * 0.5f - 92, y + 8, 80, 38, "ACCOUNT", C4(0.2f, 0.4f, 0.8f, 1),
               C4(1, 1, 1, 1), 12))
      gd_scr(g, GD_SCR_ACCOUNT);
  }

  fmti(b, "LEVELS ", g->nlv, "");
  textsh(b, 90.0f, 24.0f, 13, C4(1, 1, 1, 0.7f));
  if (L) textsh(L->name, (float)e->W - 140.0f, 24.0f, 13, C4(1, 1, 1, 0.7f));
}

static void draw_levels(Eng *e)
{
  GDGame *g = &e->g;
  float w = (float)e->W - 120, h = 84;
  int i;
  char b[80];

  vgrad(0, 0, (float)e->W, (float)e->H, C4(0.05f, 0.08f, 0.24f, 1), C4(0.12f, 0.06f, 0.26f, 1));
  head(e, "LEVELS");
  if (button(e, 10, 16.0f, (float)e->H - 62, 84, 48, "< BACK", C4(0.25f, 0.35f, 0.65f, 1),
             C4(1, 1, 1, 1), 14))
    gd_scr(g, GD_SCR_MENU);

  for (i = 0; i < g->nlv; i++) {
    GDLevel *lv = gd_level_ptr(g, i);
    float y = (float)e->H - 110 - (float)e->scroll - (float)i * (h + 12);
    C bg;
    if (!lv || y < -h || y > (float)e->H) continue;
    bg = lv->official ? C4(0.20f, 0.45f, 0.85f, 1) : C4(0.30f, 0.60f, 0.35f, 1);
    quad(58, y, w, h, C4(0, 0, 0, 0.35f));
    quad(60, y + 2, w - 4, h - 4, bg);
    quad(60, y + h * 0.6f, w - 4, h * 0.4f - 2, C4(1, 1, 1, 0.10f));
    textL(lv->name, 76, y + h - 34, 18, C4(1, 1, 1, 1));
    scat(b, "BY ", lv->author);
    textL(b, 76, y + h - 56, 12, C4(0.85f, 0.9f, 1.0f, 1));
    textL(DIFF[lv->diff < 7 ? lv->diff : 6], 76, y + 12, 13, C4(1, 1, 0.6f, 1));
    fmti(b, "BEST ", lv->best, "%");
    textL(b, 240, y + 12, 13, C4(1, 1, 1, 0.9f));
    fmti(b, "* ", lv->stars, "");
    textL(b, 380, y + 12, 13, C4(1.0f, 0.85f, 0.2f, 1));
    fmti(b, "OBJ ", lv->nobj, "");
    textL(b, 500, y + 12, 13, C4(1, 1, 1, 0.75f));
    breg(e, 100 + i, 58, y, w, h);
    if (hit(100 + i, e, e->tx, e->ty) && e->tdown && !e->moved) {
      e->page = i;
      gd_scr(g, GD_SCR_PAGE);
    }
  }
}

static void draw_page(Eng *e)
{
  GDGame *g = &e->g;
  GDLevel *L = gd_level_ptr(g, e->page);
  float cx = (float)e->W * 0.5f, w = 320, h = 60, y;
  char b[80];

  if (!L) { gd_scr(g, GD_SCR_LEVELS); return; }
  vgrad(0, 0, (float)e->W, (float)e->H, C4(0.06f, 0.10f, 0.28f, 1), C4(0.16f, 0.06f, 0.28f, 1));

  textsh(L->name, cx, (float)e->H - 110, 30, C4(1, 1, 1, 1));
  scat(b, "BY ", L->author);
  textsh(b, cx, (float)e->H - 142, 15, C4(0.75f, 0.85f, 1.0f, 1));
  scat(b, "", DIFF[L->diff < 7 ? L->diff : 6]);
  textsh(b, cx - 130, (float)e->H - 190, 18, C4(1, 1, 0.55f, 1));
  fmti(b, "* ", L->stars, "");
  textsh(b, cx, (float)e->H - 190, 18, C4(1.0f, 0.85f, 0.2f, 1));
  fmti(b, "BEST ", L->best, "%");
  textsh(b, cx + 130, (float)e->H - 190, 18, C4(0.6f, 1.0f, 0.7f, 1));
  fmti(b, "OBJECTS ", L->nobj, "");
  textsh(b, cx, (float)e->H - 220, 13, C4(1, 1, 1, 0.75f));

  y = (float)e->H - 320;
  if (button(e, 20, cx - w * 0.5f, y, w, h, "PLAY", C4(0.20f, 0.55f, 0.95f, 1), C4(1, 1, 1, 1), 20)) {
    gd_start(g, e->page, 0);
    e->popup = 0;
  }
  y -= h + 12;
  if (button(e, 21, cx - w * 0.5f, y, w, h, "PRACTICE", C4(0.25f, 0.70f, 0.40f, 1),
             C4(1, 1, 1, 1), 20))
    gd_start(g, e->page, 1);
  y -= h + 12;
  if (!L->official) {
    if (button(e, 22, cx - w * 0.5f, y, w * 0.5f - 6, h, "EDIT", C4(0.85f, 0.55f, 0.20f, 1),
               C4(1, 1, 1, 1), 18)) {
      gd_ed_init(g, e->page, GD_TOOL_BUILD, 0);
      gd_scr(g, GD_SCR_EDITOR);
    }
    if (button(e, 23, cx + 6, y, w * 0.5f - 6, h, "DELETE", C4(0.75f, 0.25f, 0.25f, 1),
               C4(1, 1, 1, 1), 18)) {
      gd_lvl_delete(g, e->page);
      gd_scr(g, GD_SCR_LEVELS);
    }
    y -= h + 12;
  }
  if (button(e, 24, cx - w * 0.5f, y, w, h, "< BACK", C4(0.25f, 0.35f, 0.65f, 1),
             C4(1, 1, 1, 1), 18))
    gd_scr(g, GD_SCR_LEVELS);
}

/* ------------------------------------------------------------- гараж */

static const C PALETTE[12] = {
  { 0.20f, 0.60f, 1.00f, 1 }, { 0.30f, 0.90f, 0.40f, 1 }, { 1.00f, 0.80f, 0.20f, 1 },
  { 1.00f, 0.35f, 0.35f, 1 }, { 0.80f, 0.40f, 1.00f, 1 }, { 1.00f, 0.50f, 0.20f, 1 },
  { 0.30f, 0.90f, 0.90f, 1 }, { 1.00f, 0.45f, 0.80f, 1 }, { 0.95f, 0.95f, 0.95f, 1 },
  { 0.45f, 0.50f, 0.65f, 1 }, { 0.15f, 0.18f, 0.30f, 1 }, { 0.60f, 0.35f, 0.20f, 1 }
};

static void draw_garage(Eng *e)
{
  GDGame *g = &e->g;
  int acc = g->loggedIn ? g->curAcc : -1;
  float cx = (float)e->W * 0.5f;
  char b[48];
  int i, c1i = 0, c2i = 5, glow = 0, icon = 0;
  C c1 = C4(0.2f, 0.6f, 1.0f, 1), c2 = C4(1.0f, 0.5f, 0.2f, 1);
  GDPlayer pv;

  vgrad(0, 0, (float)e->W, (float)e->H, C4(0.08f, 0.10f, 0.26f, 1), C4(0.18f, 0.08f, 0.30f, 1));
  head(e, "ICON KIT");
  if (button(e, 30, 16.0f, (float)e->H - 62, 84, 48, "< BACK", C4(0.25f, 0.35f, 0.65f, 1),
             C4(1, 1, 1, 1), 14))
    gd_scr(g, GD_SCR_MENU);

  if (acc >= 0) {
    c1i = g->acc[acc].c1; c2i = g->acc[acc].c2; glow = g->acc[acc].glow;
    icon = g->acc[acc].icon[e->garageMode];
  }
  c1 = PALETTE[c1i % 12]; c2 = PALETTE[c2i % 12];

  /* вкладки режимов */
  for (i = 0; i < GD_MODE_N; i++) {
    float x = cx - (GD_MODE_N * 62) * 0.5f + i * 62;
    if (button(e, 200 + i, x, (float)e->H - 130, 56, 40, gd_mode_en(i),
               e->garageMode == i ? C4(0.30f, 0.60f, 1.0f, 1) : C4(0.20f, 0.25f, 0.45f, 1),
               C4(1, 1, 1, 1), 10))
      e->garageMode = i;
  }

  /* превью */
  memset(&pv, 0, sizeof pv);
  pv.x = CX + 4.5f; pv.y = CY + 3.0f; pv.mode = e->garageMode; pv.rot = 0;
  pv.mini = 0;
  quad(PXf(pv.x) - 90, PYf(pv.y) - 90, 180, 180, C4(0, 0, 0, 0.25f));
  draw_player(e, &pv, c1, c2, glow);

  fmti(b, "ICON ", icon + 1, "");
  textsh(b, cx, (float)e->H - 300, 16, C4(1, 1, 1, 0.9f));
  if (button(e, 31, cx - 150, (float)e->H - 320, 60, 40, "<", C4(0.25f, 0.35f, 0.65f, 1),
             C4(1, 1, 1, 1), 16) && acc >= 0)
    g->acc[acc].icon[e->garageMode] = (icon + 11) % 12;
  if (button(e, 32, cx + 90, (float)e->H - 320, 60, 40, ">", C4(0.25f, 0.35f, 0.65f, 1),
             C4(1, 1, 1, 1), 16) && acc >= 0)
    g->acc[acc].icon[e->garageMode] = (icon + 1) % 12;

  /* палитры */
  for (i = 0; i < 12; i++) {
    float x = cx - 12 * 34 * 0.5f + i * 34;
    if (button(e, 300 + i, x, (float)e->H - 400, 30, 30, "", PALETTE[i], C4(1, 1, 1, 1), 10) &&
        acc >= 0)
      g->acc[acc].c1 = i;
    if (button(e, 340 + i, x, (float)e->H - 440, 30, 30, "", PALETTE[i], C4(1, 1, 1, 1), 10) &&
        acc >= 0)
      g->acc[acc].c2 = i;
  }
  textL("PRIMARY", cx - 12 * 34 * 0.5f, (float)e->H - 372, 12, C4(1, 1, 1, 0.8f));
  textL("SECONDARY", cx - 12 * 34 * 0.5f, (float)e->H - 412, 12, C4(1, 1, 1, 0.8f));

  if (acc >= 0) {
    if (button(e, 33, cx - 90, 60, 180, 46, glow ? "GLOW: ON" : "GLOW: OFF",
               C4(0.35f, 0.45f, 0.80f, 1), C4(1, 1, 1, 1), 14))
      g->acc[acc].glow = !g->acc[acc].glow;
  } else {
    textsh("LOG IN TO SAVE ICONS", cx, 70, 14, C4(1, 0.8f, 0.4f, 1));
  }
}

/* ------------------------------------------------------------- аккаунты */

static const char *KEYS[3] = { "1234567890", "qwertyuiop", "asdfghjklzxcvbnm" };

static void draw_account(Eng *e)
{
  GDGame *g = &e->g;
  float cx = (float)e->W * 0.5f, kw = 52, kh = 56;
  char b[64];
  int r, i;

  vgrad(0, 0, (float)e->W, (float)e->H, C4(0.06f, 0.09f, 0.26f, 1), C4(0.14f, 0.06f, 0.28f, 1));
  head(e, "ACCOUNT");
  if (button(e, 40, 16.0f, (float)e->H - 62, 84, 48, "< BACK", C4(0.25f, 0.35f, 0.65f, 1),
             C4(1, 1, 1, 1), 14))
    gd_scr(g, GD_SCR_MENU);

  if (g->loggedIn && g->curAcc >= 0) {
    GDAcc *a = &g->acc[g->curAcc];
    textsh(a->name, cx, (float)e->H - 130, 26, C4(1, 1, 1, 1));
    fmti(b, "STARS ", a->stars, "");   textL(b, cx - 200, (float)e->H - 180, 15, C4(0.85f, 0.9f, 1, 1));
    fmti(b, "ORBS ", a->orbs, "");     textL(b, cx - 200, (float)e->H - 206, 15, C4(0.85f, 0.9f, 1, 1));
    fmti(b, "DEMONS ", a->demons, ""); textL(b, cx + 40, (float)e->H - 180, 15, C4(0.85f, 0.9f, 1, 1));
    fmti(b, "COINS ", a->coins, "");   textL(b, cx + 40, (float)e->H - 206, 15, C4(0.85f, 0.9f, 1, 1));
    if (button(e, 41, cx - 110, (float)e->H - 280, 220, 54, "LOG OUT",
               C4(0.70f, 0.30f, 0.30f, 1), C4(1, 1, 1, 1), 18)) {
      gd_acc_logout(g);
      say(e, "LOGGED OUT");
    }
    return;
  }

  /* поля ввода */
  quad(cx - 220, (float)e->H - 190, 440, 46, C4(0, 0, 0, 0.4f));
  quad(cx - 218, (float)e->H - 188, 436, 42, C4(0.12f, 0.16f, 0.32f, 1));
  textL(e->nm[0] ? e->nm : "USERNAME", cx - 206, (float)e->H - 172, 16,
        e->nm[0] ? C4(1, 1, 1, 1) : C4(0.6f, 0.65f, 0.8f, 1));
  if (button(e, 42, cx + 230, (float)e->H - 190, 60, 46, e->field == 0 ? "*" : "",
             C4(0.2f, 0.4f, 0.8f, 1), C4(1, 1, 1, 1), 14))
    e->field = 0;

  quad(cx - 220, (float)e->H - 250, 440, 46, C4(0, 0, 0, 0.4f));
  quad(cx - 218, (float)e->H - 248, 436, 42, C4(0.12f, 0.16f, 0.32f, 1));
  {
    char stars[GD_STR];
    int n = slen(e->pw), k;
    for (k = 0; k < n && k < GD_STR - 1; k++) stars[k] = '*';
    stars[k] = 0;
    textL(stars[0] ? stars : "PASSWORD", cx - 206, (float)e->H - 232, 16,
          stars[0] ? C4(1, 1, 1, 1) : C4(0.6f, 0.65f, 0.8f, 1));
  }
  if (button(e, 43, cx + 230, (float)e->H - 250, 60, 46, e->field == 1 ? "*" : "",
             C4(0.2f, 0.4f, 0.8f, 1), C4(1, 1, 1, 1), 14))
    e->field = 1;

  /* экранная клавиатура */
  for (r = 0; r < 3; r++) {
    int n = slen(KEYS[r]);
    float x0 = cx - n * kw * 0.5f;
    for (i = 0; i < n; i++) {
      char lb[2];
      lb[0] = KEYS[r][i]; lb[1] = 0;
      if (button(e, 500 + r * 16 + i, x0 + i * kw, (float)e->H - 330 - r * (kh + 8),
                 kw - 6, kh, lb, C4(0.22f, 0.28f, 0.50f, 1), C4(1, 1, 1, 1), 16)) {
        char *dst = e->field ? e->pw : e->nm;
        int l = slen(dst);
        if (l < GD_STR - 1) { dst[l] = KEYS[r][i]; dst[l + 1] = 0; }
      }
    }
  }
  if (button(e, 44, cx - 170, (float)e->H - 330 - 3 * (kh + 8), 160, kh, "BACK",
             C4(0.60f, 0.30f, 0.30f, 1), C4(1, 1, 1, 1), 14)) {
    char *dst = e->field ? e->pw : e->nm;
    int l = slen(dst);
    if (l > 0) dst[l - 1] = 0;
  }
  if (button(e, 45, cx + 10, (float)e->H - 330 - 3 * (kh + 8), 160, kh, "REGISTER",
             C4(0.25f, 0.65f, 0.35f, 1), C4(1, 1, 1, 1), 14)) {
    char err[8];
    int rc = gd_acc_register(g, e->nm, e->pw, err);
    if (rc == 0) { say(e, "ACCOUNT CREATED"); save_store(e); }
    else if (rc == -1) say(e, "BAD USERNAME (3-19, NO SPACES)");
    else if (rc == -2) say(e, "PASSWORD TOO SHORT (6+)");
    else if (rc == -3) say(e, "USERNAME TAKEN");
    else say(e, "TOO MANY ACCOUNTS");
  }
  if (button(e, 46, cx - 170, (float)e->H - 330 - 4 * (kh + 8) - 8, 340, kh, "LOG IN",
             C4(0.20f, 0.45f, 0.90f, 1), C4(1, 1, 1, 1), 16)) {
    char err[8];
    int rc = gd_acc_login(g, e->nm, e->pw, err);
    if (rc == 0) { say(e, "LOGIN OK"); save_store(e); }
    else if (rc == -1) say(e, "NO SUCH ACCOUNT");
    else say(e, "WRONG PASSWORD");
  }
}

/* ------------------------------------------------------------- настройки */

static void draw_settings(Eng *e)
{
  GDGame *g = &e->g;
  float cx = (float)e->W * 0.5f, w = 340, h = 56, y = (float)e->H - 150;
  char b[64];

  vgrad(0, 0, (float)e->W, (float)e->H, C4(0.07f, 0.09f, 0.26f, 1), C4(0.16f, 0.06f, 0.28f, 1));
  head(e, "MORE");
  if (button(e, 50, 16.0f, (float)e->H - 62, 84, 48, "< BACK", C4(0.25f, 0.35f, 0.65f, 1),
             C4(1, 1, 1, 1), 14))
    gd_scr(g, GD_SCR_MENU);

  fmti(b, "PHYSICS ", 240, " HZ");
  if (button(e, 51, cx - w * 0.5f, y, w, h, b, C4(0.22f, 0.30f, 0.55f, 1), C4(1, 1, 1, 1), 15)) { }
  y -= h + 12;
  fmt(b, "SPEED NORMAL ", 10.386f, 3, " B/S");
  if (button(e, 52, cx - w * 0.5f, y, w, h, b, C4(0.22f, 0.30f, 0.55f, 1), C4(1, 1, 1, 1), 15)) { }
  y -= h + 12;
  fmti(b, "OBJECTS IN CATALOG ", GD_CAT_N, "");
  if (button(e, 53, cx - w * 0.5f, y, w, h, b, C4(0.22f, 0.30f, 0.55f, 1), C4(1, 1, 1, 1), 15)) { }
  y -= h + 12;
  if (button(e, 54, cx - w * 0.5f, y, w, h, "SAVE PROGRESS", C4(0.25f, 0.60f, 0.35f, 1),
             C4(1, 1, 1, 1), 16)) {
    save_store(e);
    say(e, "SAVED");
  }
  y -= h + 12;
  if (button(e, 55, cx - w * 0.5f, y, w, h, "RESET DEMO LEVELS", C4(0.65f, 0.30f, 0.30f, 1),
             C4(1, 1, 1, 1), 16)) {
    int keep = g->nacc;
    g->nlv = 0;
    gd_build_demo(g);
    g->nacc = keep;
    gd_lvl_select(g, 0);
    say(e, "LEVELS REBUILT");
  }
  textsh("GEOMETRY DASH FOR DERKA - C99 + OPENGL ES 2", cx, 30, 12, C4(1, 1, 1, 0.55f));
}

/* ------------------------------------------------------------- редактор */

static void draw_editor(Eng *e)
{
  GDGame *g = &e->g;
  GDLevel *L = g->L;
  float w = (float)e->W, h = (float)e->H;
  float palH = 150, tabH = 44;
  int i, n, cols;
  char b[48];

  if (!L) { gd_scr(g, GD_SCR_MENU); return; }

  CX = g->camx; CY = g->camy;
  SC = h / VIEW_H;
  GY = h * 0.30f;
  draw_world(e);

  for (i = 0; i < L->nobj; i++) {
    const GDObj *o = &L->o[i];
    if (o->x + o->ox > CX + w / SC + 1.0f) break;
    if (o->x + o->ox + o->w * o->sx < CX - 1.0f) continue;
    draw_obj(e, o, g->t);
  }
  /* сетка */
  {
    float x0 = (float)(int)CX - 1, x1 = CX + w / SC + 1;
    float wx, wy;
    for (wx = x0; wx < x1; wx += 1.0f)
      quad(PXf(wx), 0, 1.0f, h, C4(1, 1, 1, 0.05f));
    for (wy = -2; wy < 14; wy += 1.0f)
      quad(0, PYf(wy), w, 1.0f, C4(1, 1, 1, 0.05f));
  }
  /* выделение */
  for (i = 0; i < g->nsel; i++) {
    int k = g->selected[i];
    if (k < 0 || k >= L->nobj) continue;
    {
      const GDObj *o = &L->o[k];
      float x = PXf(o->x + o->ox), y = PYf(o->y + o->oy);
      float ow = o->w * o->sx * SC, oh = o->h * o->sy * SC;
      quad(x - 2, y - 2, ow + 4, 3, C4(0.3f, 1.0f, 0.5f, 0.9f));
      quad(x - 2, y + oh - 1, ow + 4, 3, C4(0.3f, 1.0f, 0.5f, 0.9f));
      quad(x - 2, y, 3, oh, C4(0.3f, 1.0f, 0.5f, 0.9f));
      quad(x + ow - 1, y, 3, oh, C4(0.3f, 1.0f, 0.5f, 0.9f));
    }
  }

  /* верхняя панель */
  quad(0, h - 56, w, 56, C4(0, 0, 0, 0.45f));
  if (button(e, 60, 8, h - 50, 92, 42, "< EXIT", C4(0.65f, 0.30f, 0.30f, 1), C4(1, 1, 1, 1), 13))
    gd_scr(g, GD_SCR_MENU);
  if (button(e, 61, 108, h - 50, 92, 42, "PLAY", C4(0.25f, 0.60f, 0.95f, 1), C4(1, 1, 1, 1), 13)) {
    gd_start(g, g->cur, 0);
  }
  if (button(e, 62, 208, h - 50, 92, 42, "SAVE", C4(0.25f, 0.65f, 0.35f, 1), C4(1, 1, 1, 1), 13)) {
    save_store(e);
    say(e, "LEVEL SAVED");
  }
  if (button(e, 63, 308, h - 50, 76, 42, "UNDO", C4(0.35f, 0.40f, 0.65f, 1), C4(1, 1, 1, 1), 13))
    gd_ed_undo(g);
  if (button(e, 64, 392, h - 50, 76, 42, "DEL", C4(0.70f, 0.30f, 0.30f, 1), C4(1, 1, 1, 1), 13))
    gd_ed_del_sel(g);
  if (button(e, 65, 476, h - 50, 76, 42, "ROT", C4(0.40f, 0.45f, 0.70f, 1), C4(1, 1, 1, 1), 13))
    gd_ed_rot_sel(g, 45.0f);
  if (button(e, 66, 560, h - 50, 76, 42, "COPY", C4(0.40f, 0.45f, 0.70f, 1), C4(1, 1, 1, 1), 13))
    gd_ed_copy_sel(g);
  if (button(e, 67, 644, h - 50, 84, 42, "PASTE", C4(0.40f, 0.45f, 0.70f, 1), C4(1, 1, 1, 1), 13))
    gd_ed_paste(g, CX + 4.0f, 2.0f);
  fmti(b, "GRP ", g->curGroup, "");
  if (button(e, 68, 736, h - 50, 92, 42, b, C4(0.45f, 0.35f, 0.70f, 1), C4(1, 1, 1, 1), 12))
    g->curGroup = (g->curGroup + 1) % 100;
  if (g->nsel && button(e, 69, 836, h - 50, 110, 42, "ASSIGN", C4(0.55f, 0.40f, 0.80f, 1),
                        C4(1, 1, 1, 1), 12))
    gd_ed_group_sel(g, g->curGroup);
  fmti(b, "OBJ ", L->nobj, "");
  textsh(b, w - 90, h - 34, 14, C4(1, 1, 1, 0.9f));

  /* инструменты */
  quad(0, palH + tabH, w, 40, C4(0, 0, 0, 0.40f));
  {
    const char *tn[4] = { "BUILD", "EDIT", "DELETE", "COLOR" };
    for (i = 0; i < 4; i++)
      if (button(e, 70 + i, 8 + i * 104, palH + tabH + 4, 96, 32, tn[i],
                 g->tool == i ? C4(0.30f, 0.65f, 1.0f, 1) : C4(0.22f, 0.26f, 0.45f, 1),
                 C4(1, 1, 1, 1), 12))
        g->tool = i;
    if (button(e, 74, 432, palH + tabH + 4, 120, 32, "SCALE +", C4(0.30f, 0.40f, 0.65f, 1),
               C4(1, 1, 1, 1), 12))
      gd_ed_scale_sel(g, 1.25f);
    if (button(e, 75, 560, palH + tabH + 4, 120, 32, "SCALE -", C4(0.30f, 0.40f, 0.65f, 1),
               C4(1, 1, 1, 1), 12))
      gd_ed_scale_sel(g, 0.8f);
    if (button(e, 76, 688, palH + tabH + 4, 100, 32, "FLIP X", C4(0.30f, 0.40f, 0.65f, 1),
               C4(1, 1, 1, 1), 12))
      gd_ed_flip_sel(g, 0);
    if (g->selId >= 0 && g->selId < L->nobj &&
        button(e, 77, 796, palH + tabH + 4, 120, 32, "EDIT OBJ", C4(0.70f, 0.45f, 0.20f, 1),
               C4(1, 1, 1, 1), 12)) {
      e->popup = 1;
      g->editIdx = g->selId;
    }
  }

  /* вкладки каталога */
  quad(0, palH, w, tabH, C4(0, 0, 0, 0.5f));
  for (i = 0; i < GD_TAB_COUNT; i++) {
    float x = 6 + i * ((w - 12) / GD_TAB_COUNT);
    fmti(b, "", i + 1, "");
    if (button(e, 80 + i, x, palH + 4, (w - 12) / GD_TAB_COUNT - 6, tabH - 8, b,
               g->tab == i ? C4(0.30f, 0.65f, 1.0f, 1) : C4(0.20f, 0.24f, 0.42f, 1),
               C4(1, 1, 1, 1), 13))
      g->tab = i;
  }

  /* палитра объектов вкладки */
  quad(0, 0, w, palH, C4(0.05f, 0.07f, 0.18f, 0.92f));
  n = 0;
  for (i = 0; i < GD_CAT_N; i++) if (GD_CAT[i].tab == (unsigned char)g->tab) n++;
  cols = 12;
  {
    int k = 0;
    for (i = 0; i < GD_CAT_N; i++) {
      float cw, chh, x, y;
      int id;
      if (GD_CAT[i].tab != (unsigned char)g->tab) continue;
      cw = (w - 20) / cols;
      chh = (palH - 24) / 2.0f;
      x = 10 + (k % cols) * cw;
      y = palH - 14 - ((k / cols) + 1) * chh;
      id = 400 + i;
      if (button(e, id, x, y, cw - 6, chh - 6, "",
                 g->curObj == (int)GD_CAT[i].id ? C4(0.35f, 0.70f, 1.0f, 1)
                                                : C4(0.16f, 0.20f, 0.36f, 1),
                 C4(1, 1, 1, 1), 10)) {
        g->curObj = GD_CAT[i].id;
        if (GD_CAT[i].kind == 9) g->tool = GD_TOOL_BUILD;
      }
      /* миниатюра объекта */
      {
        GDObj tmp;
        memset(&tmp, 0, sizeof tmp);
        tmp.id = GD_CAT[i].id; tmp.w = GD_CAT[i].w; tmp.h = GD_CAT[i].h;
        tmp.sx = tmp.sy = 1.0f; tmp.alpha = 1.0f;
        tmp.x = CX + (x + cw * 0.5f - PXf(0)) / SC - tmp.w * 0.5f;
        tmp.y = CY + (y + chh * 0.55f - PYf(0)) / SC - tmp.h * 0.5f;
        {
          int scr = g->screen;
          g->screen = GD_SCR_EDITOR;      /* триггеры видны только в редакторе */
          draw_obj(e, &tmp, g->t);
          g->screen = scr;
        }
      }
      fmti(b, "", (int)GD_CAT[i].id, "");
      text(b, x + (cw - 6) * 0.5f, y + 2, 9, C4(1, 1, 1, 0.65f));
      k++;
    }
  }
  {
    const GDInfo *inf = gd_info(g->curObj);
    if (inf) {
      textL(inf->en, 12, palH - 14, 12, C4(1, 1, 1, 0.9f));
      if (inf->params[0]) textL(inf->params, 150, palH - 14, 11, C4(0.8f, 0.9f, 1.0f, 0.8f));
    }
  }
}

static void draw_popup(Eng *e)
{
  GDGame *g = &e->g;
  GDLevel *L = g->L;
  float w = 620, h = 480, x = ((float)e->W - w) * 0.5f, y = ((float)e->H - h) * 0.5f;
  int i;
  char b[80];
  GDObj *o;

  if (!L || g->editIdx < 0 || g->editIdx >= L->nobj) { e->popup = 0; return; }
  o = &L->o[g->editIdx];

  quad(0, 0, (float)e->W, (float)e->H, C4(0, 0, 0, 0.55f));
  quad(x, y, w, h, C4(0.08f, 0.11f, 0.24f, 1));
  quad(x, y + h - 46, w, 46, C4(0.15f, 0.20f, 0.40f, 1));
  fmti(b, "EDIT OBJECT ", (int)o->id, "");
  textsh(b, x + w * 0.5f, y + h - 32, 18, C4(1, 1, 1, 1));
  if (button(e, 800, x + w - 54, y + h - 44, 44, 40, "X", C4(0.70f, 0.30f, 0.30f, 1),
             C4(1, 1, 1, 1), 16))
    e->popup = 0;

  for (i = 0; i < 8; i++) {
    float ry = y + h - 96 - i * 46;
    fmti(b, "A", i, "");
    textL(b, x + 20, ry + 12, 14, C4(1, 1, 1, 0.9f));
    fmt(b, "", o->a[i], 2, "");
    textL(b, x + 90, ry + 12, 14, C4(0.7f, 1.0f, 0.8f, 1));
    if (button(e, 820 + i * 2, x + w - 200, ry, 60, 34, "-", C4(0.35f, 0.40f, 0.65f, 1),
               C4(1, 1, 1, 1), 14)) {
      o->a[i] -= (o->a[i] == (float)(int)o->a[i]) ? 1.0f : 0.1f;
    }
    if (button(e, 821 + i * 2, x + w - 130, ry, 60, 34, "+", C4(0.35f, 0.40f, 0.65f, 1),
               C4(1, 1, 1, 1), 14)) {
      o->a[i] += (o->a[i] == (float)(int)o->a[i]) ? 1.0f : 0.1f;
    }
  }
  {
    const GDInfo *inf = gd_info(o->id);
    if (inf && inf->params[0]) textL(inf->params, x + 20, y + 16, 12, C4(0.8f, 0.9f, 1.0f, 0.85f));
  }
}

/* ------------------------------------------------------------- пауза / финиш */

static void draw_pause(Eng *e)
{
  GDGame *g = &e->g;
  float cx = (float)e->W * 0.5f, w = 320, h = 58, y = (float)e->H * 0.62f;

  quad(0, 0, (float)e->W, (float)e->H, C4(0, 0, 0, 0.6f));
  textsh("PAUSED", cx, (float)e->H * 0.72f, 30, C4(1, 1, 1, 1));
  if (button(e, 910, cx - w * 0.5f, y, w, h, "RESUME", C4(0.25f, 0.60f, 0.95f, 1),
             C4(1, 1, 1, 1), 18))
    e->popup = 0;
  y -= h + 12;
  if (button(e, 911, cx - w * 0.5f, y, w, h, "RESTART", C4(0.30f, 0.65f, 0.40f, 1),
             C4(1, 1, 1, 1), 18)) {
    gd_start(g, g->cur, g->practice);
    e->popup = 0;
  }
  y -= h + 12;
  if (button(e, 912, cx - w * 0.5f, y, w, h, g->practice ? "PRACTICE: ON" : "PRACTICE: OFF",
             C4(0.40f, 0.45f, 0.75f, 1), C4(1, 1, 1, 1), 16)) {
    gd_start(g, g->cur, !g->practice);
    e->popup = 0;
  }
  y -= h + 12;
  if (button(e, 913, cx - w * 0.5f, y, w, h, "EXIT", C4(0.65f, 0.30f, 0.30f, 1),
             C4(1, 1, 1, 1), 18)) {
    e->popup = 0;
    gd_scr(g, GD_SCR_MENU);
  }
}

static void draw_done(Eng *e)
{
  GDGame *g = &e->g;
  GDLevel *L = g->L;
  float cx = (float)e->W * 0.5f, w = 340, h = 58, y = (float)e->H * 0.52f;
  char b[64];

  vgrad(0, 0, (float)e->W, (float)e->H, C4(0.05f, 0.10f, 0.28f, 1), C4(0.10f, 0.30f, 0.20f, 1));
  textsh("LEVEL COMPLETE!", cx, (float)e->H * 0.72f, 32, C4(1, 1, 1, 1));
  if (L) textsh(L->name, cx, (float)e->H * 0.66f, 16, C4(0.8f, 0.9f, 1.0f, 1));
  fmti(b, "ATTEMPTS ", g->attempt + 1, "");
  textsh(b, cx, (float)e->H * 0.60f, 15, C4(1, 1, 1, 0.9f));
  fmti(b, "JUMPS ", g->jumps, "");
  textsh(b, cx - 110, (float)e->H * 0.57f, 13, C4(1, 1, 1, 0.8f));
  fmti(b, "CLICKS ", g->clicks, "");
  textsh(b, cx + 110, (float)e->H * 0.57f, 13, C4(1, 1, 1, 0.8f));
  fmti(b, "COINS ", g->coinsGot, "");
  textsh(b, cx, (float)e->H * 0.54f, 15, C4(1.0f, 0.85f, 0.2f, 1));

  if (button(e, 920, cx - w * 0.5f, y, w, h, "MENU", C4(0.25f, 0.55f, 0.95f, 1),
             C4(1, 1, 1, 1), 18)) {
    save_store(e);
    gd_scr(g, GD_SCR_MENU);
  }
  y -= h + 12;
  if (button(e, 921, cx - w * 0.5f, y, w, h, "RETRY", C4(0.30f, 0.65f, 0.40f, 1),
             C4(1, 1, 1, 1), 18))
    gd_start(g, g->cur, 0);
}

/* ------------------------------------------------------------- кадр */

static void render(Eng *e)
{
  GDGame *g = &e->g;
  char b[64];

  SC = (float)e->H / VIEW_H;
  GY = (float)e->H * 0.28f;
  CX = g->camx; CY = g->camy;

  vn = 0;
  e->nbtn = 0;
  e->tdown = e->moved ? 0 : e->tdown;

  switch (g->screen) {
    case GD_SCR_MENU:     draw_menu(e); break;
    case GD_SCR_LEVELS:   draw_levels(e); break;
    case GD_SCR_PAGE:     draw_page(e); break;
    case GD_SCR_GARAGE:   draw_garage(e); break;
    case GD_SCR_ACCOUNT:  draw_account(e); break;
    case GD_SCR_SETTINGS: draw_settings(e); break;
    case GD_SCR_EDITOR:   draw_editor(e); break;
    case GD_SCR_DONE:     draw_done(e); break;
    default:              draw_game(e); break;
  }

  if (g->screen == GD_SCR_GAME && e->popup == 2) draw_pause(e);
  if (g->screen == GD_SCR_EDITOR && e->popup == 1) draw_popup(e);

  if (e->msgT > 0.0f) {
    quad(0, 0, (float)e->W, 40, C4(0, 0, 0, 0.55f));
    textsh(e->msg, (float)e->W * 0.5f, 12, 15, C4(1, 1, 0.7f, 1));
  }
  (void)b;
}

/* ------------------------------------------------------------ EGL / GLES */

static const char *VSRC =
  "attribute vec2 a_p; attribute vec4 a_c; varying vec4 v_c;"
  "void main(){ gl_Position = vec4(a_p, 0.0, 1.0); v_c = a_c; }";
static const char *FSRC =
  "precision mediump float; varying vec4 v_c; void main(){ gl_FragColor = v_c; }";

static GLuint shader(GLenum type, const char *src)
{
  GLuint s = glCreateShader(type);
  GLint ok;
  glShaderSource(s, 1, &src, NULL);
  glCompileShader(s);
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(s, sizeof log, NULL, log);
    LOGI("shader: %s", log);
  }
  return s;
}

static void e_term(Eng *e);

static int e_init(Eng *e)
{
  EGLint cfgattr[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 0, EGL_NONE
  };
  EGLint ctxattr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
  EGLint n = 0, w, h;
  EGLConfig cfg;
  GLint linked = 0;
  GLuint vs, fs;

  e->dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (e->dpy == EGL_NO_DISPLAY || !eglInitialize(e->dpy, NULL, NULL)) {
    LOGI("eglInitialize failed, err=0x%x", eglGetError()); e_term(e); return 0;
  }
  if (!eglChooseConfig(e->dpy, cfgattr, &cfg, 1, &n) || n < 1) {
    LOGI("eglChooseConfig failed, err=0x%x", eglGetError()); e_term(e); return 0;
  }

  e->surf = eglCreateWindowSurface(e->dpy, cfg, e->app->window, NULL);
  e->ctx = eglCreateContext(e->dpy, cfg, EGL_NO_CONTEXT, ctxattr);
  if (e->surf == EGL_NO_SURFACE || e->ctx == EGL_NO_CONTEXT) {
    LOGI("eglCreateWindowSurface/Context failed, err=0x%x", eglGetError());
    e_term(e); return 0;
  }
  if (!eglMakeCurrent(e->dpy, e->surf, e->surf, e->ctx)) {
    LOGI("eglMakeCurrent failed, err=0x%x", eglGetError()); e_term(e); return 0;
  }

  eglQuerySurface(e->dpy, e->surf, EGL_WIDTH, &w);
  eglQuerySurface(e->dpy, e->surf, EGL_HEIGHT, &h);
  e->W = w > 0 ? w : 1280;
  e->H = h > 0 ? h : 720;
  gW = e->W; gH = e->H;

  vs = shader(GL_VERTEX_SHADER, VSRC);
  fs = shader(GL_FRAGMENT_SHADER, FSRC);
  e->prog = glCreateProgram();
  glAttachShader(e->prog, vs);
  glAttachShader(e->prog, fs);
  glBindAttribLocation(e->prog, 0, "a_p");
  glBindAttribLocation(e->prog, 1, "a_c");
  glLinkProgram(e->prog);
  glGetProgramiv(e->prog, GL_LINK_STATUS, &linked);
  glDeleteShader(vs);
  glDeleteShader(fs);
  if (!linked) { LOGI("shader link failed"); e_term(e); return 0; }

  glUseProgram(e->prog);
  glGenBuffers(1, &e->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, e->vbo);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(V), (void *)0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(V), (void *)(2 * sizeof(float)));

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glViewport(0, 0, e->W, e->H);
  LOGI("GL ready %dx%d", e->W, e->H);
  return 1;
}

static void e_term(Eng *e)
{
  if (e->dpy == EGL_NO_DISPLAY) return;
  eglMakeCurrent(e->dpy, e->surf, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (e->prog) glDeleteProgram(e->prog);
  if (e->vbo) glDeleteBuffers(1, &e->vbo);
  if (e->surf != EGL_NO_SURFACE) eglDestroySurface(e->dpy, e->surf);
  if (e->ctx != EGL_NO_CONTEXT) eglDestroyContext(e->dpy, e->ctx);
  eglTerminate(e->dpy);
  e->dpy = EGL_NO_DISPLAY; e->surf = EGL_NO_SURFACE; e->ctx = EGL_NO_CONTEXT;
  e->prog = e->vbo = 0;
  e->ready = 0;
}

/* ------------------------------------------------------------ сохранения */

static void store_path(Eng *e, char *buf, int cap)
{
  const char *dir = e->app->activity ? e->app->activity->internalDataPath : 0;
  buf[0] = 0;
  if (dir && dir[0]) snprintf(buf, (size_t)cap, "%s/gd_derka.sav", dir);
  else snprintf(buf, (size_t)cap, "gd_derka.sav");
}

static void save_store(Eng *e)
{
  char path[512], tmp[GD_STORE];
  FILE *f;
  int n;
  store_path(e, path, sizeof path);
  n = gd_store_save(&e->g, tmp, sizeof tmp);
  if (n <= 0) return;
  f = fopen(path, "wb");
  if (!f) { LOGI("save failed: %s", path); return; }
  fwrite(tmp, 1, (size_t)n, f);
  fclose(f);
  LOGI("saved %d bytes to %s", n, path);
}

static void load_store(Eng *e)
{
  char path[512], tmp[GD_STORE];
  FILE *f;
  size_t n;
  store_path(e, path, sizeof path);
  f = fopen(path, "rb");
  if (!f) { LOGI("no save file at %s", path); return; }
  n = fread(tmp, 1, sizeof tmp - 1, f);
  fclose(f);
  tmp[n] = 0;
  gd_store_load(&e->g, tmp);
  LOGI("loaded %u bytes, accounts=%d levels=%d", (unsigned)n, e->g.nacc, e->g.nlv);
}

/* ------------------------------------------------------------- кадр и ввод */

static void frame(Eng *e)
{
  double now;
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  now = ts.tv_sec + ts.tv_nsec * 1e-9;
  gd_update(&e->g, e->last ? (float)(now - e->last) : 0.016f);
  e->last = now;
  if (e->msgT > 0.0f) e->msgT -= (float)(now - e->last);

  render(e);
  if (!e->logged) { e->logged = 1; LOGI("first frame: %d verts", vn); }
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glBufferData(GL_ARRAY_BUFFER, vn * (GLsizeiptr)sizeof(V), vb, GL_STREAM_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, vn);
  eglSwapBuffers(e->dpy, e->surf);
  e->tdown = 0;
  e->moved = 0;
}

/* обработка касания в мире редактора */
static void editor_touch(Eng *e, int down)
{
  GDGame *g = &e->g;
  float wx = CX + (e->tx - 0.0f) / SC;
  float wy = CY + (e->ty - GY) / SC;

  if (e->ty < 236.0f || e->ty > (float)e->H - 56.0f) return;   /* панели */
  if (down) {
    e->sx = e->tx; e->sy = e->ty; e->drag = 1; e->moved = 0;
    if (g->tool == GD_TOOL_BUILD) return;      /* ставим на отпускании (см. ниже) */
    if (g->tool == GD_TOOL_EDIT) { gd_ed_pick(g, wx, wy, 0); return; }
    if (g->tool == GD_TOOL_DEL) {
      int i = gd_obj_at(g, wx, wy);
      if (i >= 0) { g->nsel = 1; g->selected[0] = i; gd_ed_del_sel(g); }
      return;
    }
    gd_ed_pick(g, wx, wy, 0);
  } else {
    e->drag = 0;
    if (e->moved) return;
    if (g->tool == GD_TOOL_BUILD) gd_ed_place(g, wx, wy);
  }
}

static int32_t on_input(struct android_app *app, AInputEvent *ev)
{
  Eng *e = (Eng *)app->userData;
  int32_t type = AInputEvent_getType(ev);

  if (type == AINPUT_EVENT_TYPE_MOTION) {
    int32_t a = AMotionEvent_getAction(ev) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(ev, 0);
    float y = (float)e->H - AMotionEvent_getY(ev, 0);   /* GL: Y снизу */

    if (a == AMOTION_EVENT_ACTION_DOWN || a == AMOTION_EVENT_ACTION_POINTER_DOWN) {
      e->tx = x; e->ty = y; e->sx = x; e->sy = y;
      e->tdown = 1; e->moved = 0; e->drag = 1;
      if (e->g.screen == GD_SCR_EDITOR) { editor_touch(e, 1); return 1; }
      if (e->g.screen == GD_SCR_GAME && !e->popup) { gd_press(&e->g); return 1; }
      return 1;
    }
    if (a == AMOTION_EVENT_ACTION_MOVE) {
      float dx = x - e->sx, dy = y - e->sy;
      e->tx = x; e->ty = y;
      if (fabsf(dx) + fabsf(dy) > 14.0f) e->moved = 1;
      if (e->moved && e->g.screen == GD_SCR_EDITOR) {
        if (e->g.nsel && e->g.tool == GD_TOOL_EDIT)
          gd_ed_move_sel(&e->g, dx / SC, dy / SC);
        else { e->g.camx -= dx / SC; e->g.camy -= dy / SC; }
        e->sx = x; e->sy = y;
      }
      if (e->moved && e->g.screen == GD_SCR_LEVELS) {
        e->scroll -= (int)dy;
        if (e->scroll < 0) e->scroll = 0;
        e->sx = x; e->sy = y;
      }
      return 1;
    }
    if (a == AMOTION_EVENT_ACTION_UP || a == AMOTION_EVENT_ACTION_POINTER_UP ||
        a == AMOTION_EVENT_ACTION_CANCEL) {
      e->tx = x; e->ty = y;
      if (e->g.screen == GD_SCR_EDITOR) editor_touch(e, 0);
      else gd_release(&e->g);
      e->drag = 0;
      return 1;
    }
    return 1;
  }
  if (type == AINPUT_EVENT_TYPE_KEY) {
    int32_t kc = AKeyEvent_getKeyCode(ev);
    int dn = AKeyEvent_getAction(ev) == AKEY_EVENT_ACTION_DOWN;
    if (kc == AKEYCODE_BACK) {
      if (dn) {
        if (e->popup) e->popup = 0;
        else if (e->g.screen == GD_SCR_GAME) { gd_release(&e->g); e->popup = 2; }
        else if (e->g.screen != GD_SCR_MENU) gd_scr(&e->g, GD_SCR_MENU);
      }
      return 1;
    }
    if (kc == AKEYCODE_SPACE || kc == AKEYCODE_ENTER || kc == AKEYCODE_DPAD_CENTER ||
        kc == AKEYCODE_DPAD_UP || kc == AKEYCODE_BUTTON_A || kc == AKEYCODE_W) {
      if (e->g.screen == GD_SCR_GAME && !e->popup) {
        if (dn) gd_press(&e->g); else gd_release(&e->g);
      }
      return 1;
    }
  }
  return 0;
}

static void on_cmd(struct android_app *app, int32_t cmd)
{
  Eng *e = (Eng *)app->userData;

  switch (cmd) {
    case APP_CMD_INIT_WINDOW:
      LOGI("APP_CMD_INIT_WINDOW");
      e->ready = e_init(e);
      if (!e->ready) LOGI("EGL/GL init FAILED, eglGetError=0x%x", eglGetError());
      break;
    case APP_CMD_TERM_WINDOW:
      e_term(e);
      break;
    case APP_CMD_WINDOW_RESIZED:
      if (e->ready) {
        EGLint w, h;
        eglQuerySurface(e->dpy, e->surf, EGL_WIDTH, &w);
        eglQuerySurface(e->dpy, e->surf, EGL_HEIGHT, &h);
        if (w > 0 && h > 0) {
          e->W = w; e->H = h; gW = w; gH = h;
          glViewport(0, 0, w, h);
        }
      }
      break;
    case APP_CMD_GAINED_FOCUS:
    case APP_CMD_RESUME:
      e->anim = 1;
      break;
    case APP_CMD_LOST_FOCUS:
    case APP_CMD_PAUSE:
      e->anim = 0;
      gd_release(&e->g);
      save_store(e);
      break;
    case APP_CMD_DESTROY:
      save_store(e);
      e_term(e);
      break;
  }
}

void android_main(struct android_app *state)
{
  Eng e;
  int ident, events;
  struct android_poll_source *src;

  memset(&e, 0, sizeof e);
  e.app = state;
  e.dpy = EGL_NO_DISPLAY;
  e.surf = EGL_NO_SURFACE;
  e.ctx = EGL_NO_CONTEXT;
  state->userData = &e;
  state->onAppCmd = on_cmd;
  state->onInputEvent = on_input;

  LOGI("android_main: Geometry Dash for Derka starting");
  gd_init(&e.g);
  load_store(&e);
  if (e.g.nlv > 0) gd_lvl_select(&e.g, 0);
  LOGI("ready: %d levels, %d accounts, catalog %d objects",
       e.g.nlv, e.g.nacc, GD_CAT_N);

  for (;;) {
    while ((ident = ALooper_pollAll(e.anim && e.ready ? 0 : -1, NULL, &events,
                                    (void **)&src)) >= 0) {
      if (src) src->process(state, src);
      if (state->destroyRequested) { save_store(&e); e_term(&e); return; }
    }
    if (e.anim && e.ready) frame(&e);
  }
}
