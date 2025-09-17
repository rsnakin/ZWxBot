#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <sys/stat.h>
#include "log.hpp"

Log::Log() {
    On = false;
    writeResult = false;
};

Log::~Log() {
    ;
}

void Log::setLogPath(const char *logPath) {
    if (isLogPathInitialized(logPath)) {
        snprintf(__logPath, sizeof(__logPath), "%s", logPath);
        On = true;
    } else {
        On = false;
    }
}

bool Log::isLogPathInitialized(const char *arr) {
    for (int i = 0; i < sizeof(arr); ++i) {
        if (arr[i] != '\0') {
            return true;
        }
    }
    return false;
}

void Log::setLogFilePath() {

    struct stat st;
    if (stat(__logPath, &st) != 0) {
        if (mkdir(__logPath, 0755) != 0) {
            On = false;
            return;
        }
    }

    time_t t = time(nullptr);
    struct tm tm_now;
    localtime_r(&t, &tm_now);

    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &tm_now);
    snprintf(__logFile, sizeof(__logFile), "%s/log_%s.log", __logPath, dateStr);

}

const char* Log::getLogFilePath() {
    setLogFilePath();
    return __logFile;
}

void Log::write(const char *format, ...) {
    writeResult = false;
    setLogFilePath();
    if (!On) return;

    FILE* flog = fopen(__logFile, "a");
    if (!flog) return;

    char cDate[21];
    time_t unixTime = time(nullptr);
    struct tm ts = *localtime(&unixTime);
    strftime(cDate, sizeof(cDate), "%d-%m-%Y %H:%M:%S", &ts);
    fprintf(flog, "[%s]: ", cDate);

    va_list args;
    va_start(args, format);

    for (const char *p = format; *p; ++p) {
        if (*p != '%') {
            fputc(*p, flog);
            continue;
        }

        switch (*++p) {
            case 'd':
                fprintf(flog, "%d", va_arg(args, int));
                break;
            case 'f':
                fprintf(flog, "%2.3f", va_arg(args, double));
                break;
            case 's': {
                const char *sval = va_arg(args, const char *);
                fputs(sval ? sval : "(null)", flog);
                break;
            }
            default:
                fputc(*p, flog);
                break;
        }
    }

    va_end(args);
    fputc('\n', flog);
    fclose(flog);
    writeResult = true;
}
