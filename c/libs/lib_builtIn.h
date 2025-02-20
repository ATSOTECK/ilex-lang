//
// Created by Skyler on 4/29/22.
//

#ifndef __C_LIB_BUILTIN_H__
#define __C_LIB_BUILTIN_H__

#include "../value.h"
#include "../vm.h"

#include "lib_env.h"
#include "lib_ilex.h"
#include "lib_io.h"
#include "lib_json.h"
#include "lib_math.h"
#include "lib_random.h"
#include "lib_sys.h"
#include "lib_window.h"
#include "lib_ascii.h"
#include "lib_web.h"
#include "lib_http.h"

BuiltInLibs makeLib(VM *vm, const char *name, BuiltInLib lib);
void initBuiltInLibs(VM *vm);
Value useBuiltInLib(VM *vm, int idx);
int findBuiltInLib(VM *vm, char *name, int len);

#endif //__C_LIB_BUILTIN_H__
