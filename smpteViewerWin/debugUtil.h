#pragma once

#include <stdio.h>

#define logerror(x) printf("ERROR: " x  "\n");
#define logwarn(x) printf("WARN: " x  "\n");
#define loginfo(x) printf("INFO: " x  "\n");
#define logdebug(x) printf("DEBUG: " x  "\n");

//#define logerror(x) (void);
//#define logwarn(x) (void);
//#define loginfo(x) (void);
//#define logdebug(x) (void);