#pragma once

#include <stdio.h>

#ifndef MODULE
#define MODULE      "Hibernate"
#endif

#define LOGINFO(fmt, ...) do { fprintf(stdout, MODULE " [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);fflush(stdout); } while (0)
#define LOGERR(fmt, ...) do { fprintf(stderr, MODULE " [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);fflush(stderr); } while (0)