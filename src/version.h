#pragma once

#define VERSION_MAJOR 1
#define VERSION_MINOR 3

#define lxstr(s) lstr(s)
#define lstr(s) #s

#define VERSION lxstr(VERSION_MAJOR) "." lxstr(VERSION_MINOR)
#define COMPILED_TS __DATE__ " " __TIME__
#define COMPILED_TS_DATE __DATE__
