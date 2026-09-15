/* Прогон оболочки на хосте: реально вызывает render() для каждого экрана. */
#define android_main gd_android_main_unused
#include "../game/game.c"
#undef android_main

extern int g_last_verts;
static int FAILS;
static void ck(int c, const char *what) { printf("  %s %s\n", c?"OK  ":"FAIL", what); if(!c) FAILS++; }

static void scr(Eng *e, int s) { e->g.screen = s; render(e); }

int main(void)
{
  Eng e;
  int i;
  memset(&e, 0, sizeof e);
  e.W = gW = 1280; e.H = gH = 720;
  e.dpy = EGL_NO_DISPLAY; e.surf = EGL_NO_SURFACE; e.ctx = EGL_NO_CONTEXT;
  gd_init(&e.g);
  ck(e.g.nlv == 3, "демо-уровни созданы");

  scr(&e, GD_SCR_MENU);    ck(vn > 200, "меню рисуется");
  scr(&e, GD_SCR_LEVELS);  ck(vn > 200, "список уровней рисуется");
  e.page = 0; scr(&e, GD_SCR_PAGE); ck(vn > 200, "страница уровня рисуется");
  scr(&e, GD_SCR_GARAGE);  ck(vn > 200, "гараж рисуется");
  scr(&e, GD_SCR_ACCOUNT); ck(vn > 200, "экран аккаунта рисуется");
  scr(&e, GD_SCR_SETTINGS);ck(vn > 200, "настройки рисуются");

  /* регистрация через экранную клавиатуру */
  strcpy(e.nm, "derka"); strcpy(e.pw, "secret1");
  ck(gd_acc_register(&e.g, e.nm, e.pw, 0) == 0, "регистрация из оболочки");
  scr(&e, GD_SCR_ACCOUNT); ck(vn > 200, "профиль рисуется");

  /* гараж: переключение режимов и цветов */
  for (i = 0; i < GD_MODE_N; i++) { e.garageMode = i; scr(&e, GD_SCR_GARAGE); }
  ck(e.g.nacc > 0 && e.g.acc[0].glow == 0, "иконка сохранена в аккаунте");

  /* редактор: ставим объекты всех вкладок и рисуем */
  {
    int li = gd_lvl_new(&e.g, "UI Test", "derka");
    gd_ed_init(&e.g, li, GD_TOOL_BUILD, 0);
    for (i = 0; i < GD_CAT_N; i++) {
      e.g.curObj = GD_CAT[i].id;
      if (GD_CAT[i].kind == 9) {
        int o = gd_obj_add(&e.g, GD_CAT[i].id, (float)(i % 20), (float)(i / 20));
        if (o >= 0) { gd_obj_set(&e.g, o, 0, 1); gd_obj_set(&e.g, o, 1, 30); gd_obj_set(&e.g, o, 3, 1.0f); }
      } else {
        e.g.curObj = GD_CAT[i].id;
        gd_ed_place(&e.g, (float)(i % 20) + 0.4f, (float)(i / 20) + 0.4f);
      }
    }
    scr(&e, GD_SCR_EDITOR);
    ck(e.g.L->nobj >= GD_CAT_N - 4, "все объекты каталога поставлены");
    ck(vn > 500, "редактор рисуется");
    /* попап редактирования триггера */
    for (i = 0; i < e.g.L->nobj; i++) if (gd_info(e.g.L->o[i].id)->kind == 9) { e.g.selId = i; break; }
    e.popup = 1; e.g.editIdx = e.g.selId;
    scr(&e, GD_SCR_EDITOR);
    ck(vn > 500, "попап параметров триггера рисуется");
    e.popup = 0;
  }

  /* игра: несколько кадров во всех режимах */
  gd_start(&e.g, 0, 0);
  for (i = 0; i < 60; i++) { gd_press(&e.g); gd_update(&e.g, 1.0f/60.0f); scr(&e, GD_SCR_GAME); }
  ck(vn > 300, "игра рисуется");
  ck(e.g.p[0].x > 5.0f, "игрок движется");
  e.popup = 2; scr(&e, GD_SCR_GAME); ck(vn > 300, "пауза рисуется"); e.popup = 0;

  /* все режимы через порталы второго уровня */
  gd_start(&e.g, 1, 0);
  for (i = 0; i < 60 * 40; i++) {
    if (e.g.p[0].y + e.g.p[0].vy * 0.2f < 4.0f) gd_press(&e.g); else gd_release(&e.g);
    gd_update(&e.g, 1.0f/60.0f);
    if (i % 240 == 0) scr(&e, GD_SCR_GAME);
    if (e.g.screen == GD_SCR_DONE) break;
  }
  ck(e.g.screen == GD_SCR_DONE, "уровень с порталами пройден из оболочки");
  scr(&e, GD_SCR_DONE); ck(vn > 200, "экран прохождения рисуется");

  /* сохранения */
  {
    char buf[GD_STORE];
    int n = gd_store_save(&e.g, buf, sizeof buf);
    ck(n > 100, "сохранение формируется");
  }
  printf("итог: %s (%d провалено), вершин в последнем кадре %d\n",
         FAILS ? "ПРОВАЛ" : "ВСЁ ОК", FAILS, g_last_verts);
  return FAILS ? 1 : 0;
}
