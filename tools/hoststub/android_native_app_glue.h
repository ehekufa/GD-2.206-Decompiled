#ifndef STUB_GLUE_H
#define STUB_GLUE_H
#include <stdint.h>
#include <stddef.h>
#include <android/log.h>
typedef struct ANativeActivity { const char *internalDataPath; const char *externalDataPath; } ANativeActivity;
typedef struct AInputEvent AInputEvent;
typedef struct AInputQueue AInputQueue;
typedef struct ALooper ALooper;
struct android_app;
struct android_poll_source { int32_t id; struct android_app *app; void (*process)(struct android_app *, struct android_poll_source *); };
typedef struct android_app {
  void *userData;
  void (*onAppCmd)(struct android_app *, int32_t);
  int32_t (*onInputEvent)(struct android_app *, AInputEvent *);
  ANativeActivity *activity;
  void *window;
  int destroyRequested;
} android_app;
#define APP_CMD_INIT_WINDOW 1
#define APP_CMD_TERM_WINDOW 2
#define APP_CMD_WINDOW_RESIZED 3
#define APP_CMD_GAINED_FOCUS 6
#define APP_CMD_LOST_FOCUS 7
#define APP_CMD_RESUME 10
#define APP_CMD_PAUSE 11
#define APP_CMD_DESTROY 13
#define AINPUT_EVENT_TYPE_MOTION 2
#define AINPUT_EVENT_TYPE_KEY 1
#define AMOTION_EVENT_ACTION_MASK 0xff
#define AMOTION_EVENT_ACTION_DOWN 0
#define AMOTION_EVENT_ACTION_UP 1
#define AMOTION_EVENT_ACTION_MOVE 2
#define AMOTION_EVENT_ACTION_CANCEL 3
#define AMOTION_EVENT_ACTION_POINTER_DOWN 5
#define AMOTION_EVENT_ACTION_POINTER_UP 6
#define AKEY_EVENT_ACTION_DOWN 0
#define AKEYCODE_BACK 4
#define AKEYCODE_SPACE 62
#define AKEYCODE_ENTER 66
#define AKEYCODE_DPAD_CENTER 23
#define AKEYCODE_DPAD_UP 19
#define AKEYCODE_BUTTON_A 96
#define AKEYCODE_W 51
int32_t AInputEvent_getType(const AInputEvent *e);
int32_t AMotionEvent_getAction(const AInputEvent *e);
float AMotionEvent_getX(const AInputEvent *e, size_t i);
float AMotionEvent_getY(const AInputEvent *e, size_t i);
int32_t AKeyEvent_getKeyCode(const AInputEvent *e);
int32_t AKeyEvent_getAction(const AInputEvent *e);
int ALooper_pollAll(int timeout, int *outFd, int *outEvents, void **outData);
#endif
