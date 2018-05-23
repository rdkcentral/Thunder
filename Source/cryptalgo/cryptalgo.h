#ifndef __CRYPTALGO_H
#define __CRYPTALGO_H

#include "Module.h"

#include "AES.h"
#include "Hash.h"
#include "HashStream.h"
#include "HMAC.h"
#include "Random.h"

#ifdef __WIN32__
#pragma comment(lib, "cryptalgo.lib")
#endif

#endif // __CRYPTALGO_H
