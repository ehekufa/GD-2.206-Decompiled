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

adb shell am start -W -n "$PKG/$ACT" > amstart.txt 2>&1 || note "am start rc=$?"
head -6 amstart.txt | while IFS= read -r l; do note "amstart: $l"; done
note "stage=started, sleeping 20s"
sleep 20

PID=$(adb shell pidof "$PKG" | tr -d '\r\n' || true)
note "stage=pid pid=${PID:-DEAD}"

adb logcat -d > logcat.txt || true
{
  echo "run=${GITHUB_RUN_ID:-?} pid=${PID:-DEAD}"
  echo "--- am start ---"; cat amstart.txt 2>/dev/null | head -8
  echo "--- targeted ---"
  grep -aE 'derka|NativeActivity|FATAL EXCEPTION|Fatal signal|UnsatisfiedLink|dlopen failed' logcat.txt | head -12
  if [ -z "$PID" ]; then
    echo "--- backtrace ---"
    grep -a -A14 'Fatal signal' logcat.txt | head -30
    echo "--- java crash ---"
    grep -a -A16 'FATAL EXCEPTION' logcat.txt | head -30
    echo "--- raw tail ---"
    tail -n 40 logcat.txt
  fi
} > smoke.txt

# дублируем отчёт в комментарий PR: его, в отличие от логов шага, можно прочитать снаружи
if command -v gh >/dev/null 2>&1 && [ -n "${GITHUB_TOKEN:-}" ]; then
  BODY=$(cat smoke.txt)
  PRNUM=$(gh pr list --state open --json number,headRefName \
          --jq '.[] | select(.headRefName=="'"${GITHUB_REF_NAME:-}"'") | .number' 2>/dev/null | head -1)
  if [ -n "$PRNUM" ]; then
    gh pr comment "$PRNUM" --body "smoke run ${GITHUB_RUN_ID:-?}:
\`\`\`
$BODY
\`\`\`" >/dev/null 2>&1 || true
    note "report posted to PR #$PRNUM"
  fi
fi

if [ -z "$PID" ]; then
  while IFS= read -r l; do echo "::error::SMOKE: $l"; done < smoke.txt
  exit 1
fi
while IFS= read -r l; do note "$l"; done < smoke.txt
adb shell am force-stop "$PKG" || true
note "stage=done, game is alive"
