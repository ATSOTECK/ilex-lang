//
// Created by Skyler Burwell on 2/24/25.
//

#ifndef __C_LIB_TOML_H__
#define __C_LIB_TOML_H__

#include "../vm.h"

#include <toml/toml.h>

void setMapValuesFromToml(VM *vm, ObjMap *map, const toml_table_t *conf);

Value useTomlLib(VM *vm);

#endif //__C_LIB_TOML_H__
