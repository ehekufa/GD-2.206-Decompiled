#ifndef STUB_ANDROID_LOG_H
#define STUB_ANDROID_LOG_H
typedef enum { ANDROID_LOG_INFO = 4 } android_LogPriority;
int __android_log_print(int prio, const char *tag, const char *fmt, ...);
#endif
