#!/usr/bin/env bash
# Smoke-тест APK на эмуляторе: поставить, запустить, дождаться первого кадра,
# снять скриншот и убедиться, что процесс не умер через несколько секунд.
#
# Отчёт уходит в сводку задания ($GITHUB_STEP_SUMMARY) и в артефакт smoke-report.
# В аннотациях остаётся ровно одна строка-вердикт, а ::error:: печатается
# ТОЛЬКО если игра реально не поехала (раньше отчёт об успехе тоже шёл через
# ::error::, из-за чего зелёный билд показывал «1 error»).
set -u

PKG=com.derka.gd
ACT=android.app.NativeActivity
GL_TIMEOUT=90        # сколько секунд ждём строку "GL ready" в logcat
WATCH_SECS=12        # сколько секунд следим, что процесс жив
TOOLS_DIR=$(cd "$(dirname "$0")" && pwd)

note() { echo "::notice::$*"; }
warn() { echo "::warning::$*"; }

: > smoke.txt
say() { echo "$*" >> smoke.txt; }

report_and_exit() {                      # $1 — код возврата, $2 — вердикт
  rc="$1"; verdict="$2"
  if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    {
      echo "## Smoke-тест на эмуляторе — $verdict"
      echo
      echo '```text'
      cat smoke.txt
      echo '```'
    } >> "$GITHUB_STEP_SUMMARY"
  fi
  echo "==================== SMOKE REPORT ===================="
  cat smoke.txt
  echo "======================================================"
  if [ "$rc" = 0 ]; then
    note "SMOKE: $verdict"
  else
    echo "::error::SMOKE: $verdict (полный отчёт — в сводке задания и артефакте smoke-report)"
  fi
  exit "$rc"
}

# ------------------------------------------------------------------ эмулятор
adb wait-for-device || report_and_exit 1 "adb wait-for-device не дождался устройства"

# эмулятор рапортует boot раньше, чем поднимаются сервисы:
# без этой паузы adb install ловит "Can't find service: package"
BOOT=""
for _ in $(seq 1 90); do
  BOOT=$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r\n' || true)
  [ "$BOOT" = "1" ] && break
  sleep 2
done
for _ in $(seq 1 60); do
  adb shell pm path android >/dev/null 2>&1 && break
  sleep 3
done

SDK=$(adb shell getprop ro.build.version.sdk 2>/dev/null | tr -d '\r\n' || true)
ABI=$(adb shell getprop ro.product.cpu.abi 2>/dev/null | tr -d '\r\n' || true)
say "run=${GITHUB_RUN_ID:-?} boot_completed=${BOOT:-none} api=${SDK:-?} abi=${ABI:-?}"

# ------------------------------------------------------------------ установка
OK=""
for i in 1 2 3 4 5; do
  if adb install -r Game.apk > install.txt 2>&1; then OK=1; break; fi
  say "install: попытка $i не удалась: $(tail -n 1 install.txt)"
  sleep 10
done
[ -n "$OK" ] || report_and_exit 1 "adb install не прошёл за 5 попыток: $(tail -n 1 install.txt 2>/dev/null)"
say "install: ok, $(adb shell pm path "$PKG" 2>&1 | tr -d '\r' | head -1)"

# --------------------------------------------------------------------- запуск
adb logcat -G 4M >/dev/null 2>&1 || true
adb logcat -c    >/dev/null 2>&1 || true
adb logcat -v time > live.log 2>&1 &
LOGPID=$!

adb shell am start -W -n "$PKG/$ACT" > amstart.txt 2>&1 || true
say "am start: $(tr -d '\r' < amstart.txt | grep -aE '^(Status|LaunchState|TotalTime|WaitTime|Error)' | paste -sd' ' -)"

# ждём первый кадр: игра логирует "GL ready WxH" сразу после eglMakeCurrent
GLLINE=""
for t in $(seq 1 "$GL_TIMEOUT"); do
  GLLINE=$(grep -a -m1 'GL ready' live.log | tr -d '\r' || true)
  [ -n "$GLLINE" ] && { say "первый кадр получен через ${t}с"; break; }
  adb shell pidof "$PKG" >/dev/null 2>&1 || { sleep 1; continue; }
  sleep 1
done

# ------------------------------------------------------- живой ли процесс
: > pids.txt
ALIVE=0; SAMPLES=0
for i in $(seq 1 $((WATCH_SECS / 2))); do
  P=$(adb shell pidof "$PKG" 2>/dev/null | tr -d '\r\n' || true)
  echo "t=$((i * 2))s pid=${P:-DEAD}" >> pids.txt
  SAMPLES=$((SAMPLES + 1))
  [ -n "$P" ] && ALIVE=$((ALIVE + 1))
  sleep 2
done

# ------------------------------------------------------------------ скриншот
adb exec-out screencap    > screen.raw 2>/dev/null || true
adb exec-out screencap -p > screen.png 2>/dev/null || true
SHOT=$(python3 "$TOOLS_DIR/screenstat.py" screen.raw --ascii 2>&1); SHOT_RC=$?

PID=$(adb shell pidof "$PKG" 2>/dev/null | tr -d '\r\n' || true)

{
  echo "--- логи игры (tag gdderka) ---"
  grep -a 'gdderka' live.log | tail -12 | tr -d '\r'
  echo "--- процесс ---"
  cat pids.txt
  echo "--- скриншот ---"
  echo "$SHOT"
  echo "--- падения/ANR ---"
  { grep -aE 'FATAL|Fatal signal|ANR in|beginning of crash' live.log | head -10 | tr -d '\r'
    adb logcat -b crash -d 2>/dev/null | head -10 | tr -d '\r'; } | head -12
  echo "--- хвост logcat по приложению ---"
  grep -aE "gdderka|$PKG|NativeActivity" live.log | tail -15 | tr -d '\r'
} >> smoke.txt

kill "$LOGPID" >/dev/null 2>&1 || true
wait "$LOGPID" >/dev/null 2>&1 || true
adb shell am force-stop "$PKG" >/dev/null 2>&1 || true

# ------------------------------------------------------------------- вердикт
[ -n "$PID" ] || report_and_exit 1 "процесс игры умер (${ALIVE}/${SAMPLES} живых замеров) — смотри секцию «падения/ANR»"
[ -n "$GLLINE" ] || report_and_exit 1 "EGL/GLES не поднялись за ${GL_TIMEOUT}с: в логах нет «GL ready» (чёрный экран/вылет при старте)"

VERTS=$(grep -a -m1 'first frame' live.log | tr -d '\r' | sed 's/.*first frame: //' || true)
GLSIZE=$(printf '%s' "$GLLINE" | sed 's/.*GL ready //')
if [ "$SHOT_RC" != 0 ]; then
  warn "SMOKE: процесс жив и GL поднялся, но screencap вернул однотонный кадр (частая особенность software-GL на эмуляторе)"
  report_and_exit 0 "игра жива: pid=$PID, GL ready $GLSIZE, кадр ${VERTS:-?}, скриншот однотонный"
fi
report_and_exit 0 "игра жива: pid=$PID, GL ready $GLSIZE, кадр ${VERTS:-?}, скриншот содержит картинку"
