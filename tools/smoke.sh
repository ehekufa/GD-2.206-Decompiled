#!/usr/bin/env bash
# Smoke-тест APK на эмуляторе (вызывается из CI-шага android-emulator-runner).
# Всё, что найдено в logcat, дублируется в аннотации и в issue репозитория,
# потому что логи шагов извне не прочитать.
set -u

PKG=com.derka.gd
ACT=android.app.NativeActivity

note() { echo "::notice::SMOKE: $*"; }
die()  { echo "::error::SMOKE: $*"; exit 1; }
trap 'die "script died rc=$? near line $LINENO"' ERR

note "stage=env adb=$(command -v adb || echo MISSING)"
adb wait-for-device || die "adb wait-for-device failed"
adb devices | head -4 | while IFS= read -r l; do note "dev: $l"; done

adb install -r Game.apk || die "adb install failed"
note "stage=installed"

adb shell am start -W -n "$PKG/$ACT" || note "am start rc=$?"
note "stage=started, sleeping 20s"
sleep 20

PID=$(adb shell pidof "$PKG" | tr -d '\r\n' || true)
note "stage=pid pid=${PID:-DEAD}"

adb logcat -d > logcat.txt || true
{
  echo "run=${GITHUB_RUN_ID:-?} pid=${PID:-DEAD}"
  grep -aE 'FATAL EXCEPTION|Fatal signal|gdderka|AndroidRuntime|UnsatisfiedLink|dlopen|libc    ' logcat.txt | head -12
  if [ -z "$PID" ]; then
    echo "--- backtrace ---"
    grep -a -A14 'Fatal signal' logcat.txt | head -30
    echo "--- java crash ---"
    grep -a -A16 'FATAL EXCEPTION' logcat.txt | head -30
  fi
} > smoke.txt

# дублируем отчёт в issue: его, в отличие от логов шага, можно прочитать снаружи
if command -v gh >/dev/null 2>&1 && [ -n "${GITHUB_TOKEN:-}" ]; then
  BODY=$(cat smoke.txt)
  NUM=$(gh issue list --state open --search "in:title Smoke report" --json number --jq '.[0].number' 2>/dev/null || true)
  if [ -n "$NUM" ]; then
    gh issue comment "$NUM" --body "$BODY" >/dev/null 2>&1 || true
  else
    gh issue create --title "Smoke report (auto)" --body "$BODY" >/dev/null 2>&1 || true
  fi
fi

if [ -z "$PID" ]; then
  while IFS= read -r l; do echo "::error::SMOKE: $l"; done < smoke.txt
  exit 1
fi
while IFS= read -r l; do note "$l"; done < smoke.txt
adb shell am force-stop "$PKG" || true
note "stage=done, game is alive"
