#include <cstdio>
#include "version.hpp"

const char VERSION[] = "1.1";
const char BUILD[] = "60";

const char* getVersion() {
    return VERSION;
}
const char* getBuild() {
    return BUILD;
}
char* getVersionFull() {
    static char fVer[7];
    snprintf(fVer, sizeof(fVer), "%s.%s", VERSION, BUILD);
    return fVer;
}
