#!/usr/bin/env bash
# Smoke-тест APK на эмуляторе (вызывается из CI-шага android-emulator-runner).
# Отчёт (pids, am start, выжимка logcat) постится комментарием в PR: логи шагов снаружи не прочитать.
set -u

PKG=com.derka.gd
ACT=android.app.NativeActivity

note() { echo "::notice::SMOKE: $*"; }
die()  { echo "::error::SMOKE: $*"; exit 1; }

post_report() {
  if command -v gh >/dev/null 2>&1 && [ -n "${GITHUB_TOKEN:-}" ]; then
    local body; body=$(cat smoke.txt)
    local num=""
    num=$(gh pr list --state open --json number,headRefName \
          --jq '.[] | select(.headRefName=="'"${GITHUB_REF_NAME:-}"'") | .number' 2>/dev/null | head -1) || true
    if [ -z "$num" ]; then
      local hr; hr=$(gh pr view 1 --json headRefName --jq .headRefName 2>/dev/null) || true
      [ "$hr" = "${GITHUB_REF_NAME:-}" ] && num=1
    fi
    if [ -n "$num" ]; then
      gh pr comment "$num" --body "smoke run ${GITHUB_RUN_ID:-?}:
\`\`\`
$body
\`\`\`" >/dev/null 2>&1 || echo "::warning::SMOKE: pr comment failed"
      note "report posted to PR #$num"
    else
      echo "::warning::SMOKE: no PR found for branch, report only in annotations"
    fi
  fi
}

note "stage=env adb=$(command -v adb || echo MISSING)"
adb wait-for-device || die "adb wait-for-device failed"

adb install -r Game.apk || die "adb install failed"
note "stage=installed"

{ echo "run=${GITHUB_RUN_ID:-?}";
  echo "--- pm path ---"; adb shell pm path "$PKG" 2>&1 | head -4;
  echo "--- dumpsys package (abi) ---"; adb shell dumpsys package "$PKG" 2>&1 | grep -aE 'abi|versionName|codePath' | head -6;
} > pre.txt || true

# большой ring + живой logcat с момента ДО старта
adb logcat -G 4M >/dev/null 2>&1 || true
adb logcat -c    >/dev/null 2>&1 || true
adb logcat -v time > live.log 2>&1 &
LOGPID=$!

adb shell am start -W -n "$PKG/$ACT" > amstart.txt 2>&1 || note "am start rc=$?"
head -6 amstart.txt | while IFS= read -r l; do note "amstart: $l"; done

: > pids.txt
for i in $(seq 1 10); do
  P=$(adb shell pidof "$PKG" 2>/dev/null | tr -d '\r\n' || true)
  echo "t=$((i*2))s pid=${P:-DEAD}" >> pids.txt
  sleep 2
done
kill "$LOGPID" >/dev/null 2>&1 || true
wait "$LOGPID" >/dev/null 2>&1 || true

PID=$(adb shell pidof "$PKG" 2>/dev/null | tr -d '\r\n' || true)
note "stage=pid pid=${PID:-DEAD}"

{
  cat pre.txt
  echo "--- pids ---"; cat pids.txt
  echo "--- am start ---"; head -8 amstart.txt
  echo "--- derka/AM/crash lines ---"
  grep -aE 'derka|NativeActivity|FATAL|Fatal signal|ANR|avc|Zygote.*derka|ActivityManager|ActivityTaskManager' live.log | head -25
  echo "--- live tail ---"
  tail -n 50 live.log
} > smoke.txt

post_report

if [ -z "$PID" ]; then
  head -40 smoke.txt | while IFS= read -r l; do echo "::error::SMOKE: $l"; done
  exit 1
fi
head -12 smoke.txt | while IFS= read -r l; do note "$l"; done
adb shell am force-stop "$PKG" || true
note "stage=done, game is alive"
