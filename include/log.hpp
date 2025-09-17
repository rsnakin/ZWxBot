#pragma once

class Log {

    public:
        bool On;
        bool writeResult;
        Log();
        ~Log();
        void setLogPath(const char *logPath);
        const char* getLogFilePath();
        void write(const char *format, ...);
    private:
        char __logPath[256] = {0};
        char __logFile[256] = {0};
        bool isLogPathInitialized(const char *arr);
        void setLogFilePath();
};
