#!/usr/bin/env bash
# Хостовые проверки без Android: всё, что можно прогнать обычным gcc.
#   1) ядро (gd_core.c): физика, орбы, порталы, триггеры, редактор, аккаунты, строка уровня
#   2) оболочка (game.c): компиляция
#   3) оболочка: реальный прогон отрисовки всех экранов на стабах OpenGL
set -euo pipefail
cd "$(dirname "$0")/.."

echo "== 1. ядро: автопилот и юнит-тесты (сдвиг тайминга 0) =="
gcc -std=c99 -O2 -Wall -DGD_SIM game/gd_core.c -o /tmp/gdsim -lm
/tmp/gdsim 0

echo "== 1b. ядро: автопилот с опозданием прыжка на 0.6 блока =="
/tmp/gdsim -0.6

echo "== 2. оболочка: компиляция game.c =="
gcc -std=gnu99 -O2 -Wall -Itools/hoststub -Igame -c game/game.c -o /tmp/gd_game.o

echo "== 3. оболочка: прогон экранов (меню, уровни, гараж, аккаунты, редактор, игра) =="
gcc -std=gnu99 -O1 -Wall -Itools/hoststub -Igame \
    tools/ui_harness.c tools/hoststub/stubs.c game/gd_core.c -o /tmp/gdui -lm
/tmp/gdui

echo "== ВСЕ ХОСТОВЫЕ ПРОВЕРКИ ПРОЙДЕНЫ =="
