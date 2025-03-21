//
// Created by Skyler Burwell on 2/24/25.
//

#include "lib_toml.h"

#include <stdlib.h>

static void setMapString(VM *vm, ObjMap *map, const toml_datum_t datum, const char *key) {
    const Value valueKey = OBJ_VAL(copyString(vm, key, (int)strlen(key)));
    push(vm, valueKey);

    const Value value = OBJ_VAL(copyString(vm, datum.u.s, (int)strlen(datum.u.s)));
    push(vm, value);

    mapSet(vm, map, valueKey, value);
    pop(vm);
    pop(vm);
}

static void setMapBool(VM *vm, ObjMap *map, const toml_datum_t datum, const char *key) {
    const Value valueKey = OBJ_VAL(copyString(vm, key, (int)strlen(key)));
    push(vm, valueKey);

    const Value value = BOOL_VAL(datum.u.b);
    push(vm, value);

    mapSet(vm, map, valueKey, value);
    pop(vm);
    pop(vm);
}

static void setMapInt(VM *vm, ObjMap *map, const toml_datum_t datum, const char *key) {
    const Value valueKey = OBJ_VAL(copyString(vm, key, (int)strlen(key)));
    push(vm, valueKey);

    const Value value = NUMBER_VAL(datum.u.i);
    push(vm, value);

    mapSet(vm, map, valueKey, value);
    pop(vm);
    pop(vm);
}

static void setMapDouble(VM *vm, ObjMap *map, const toml_datum_t datum, const char *key) {
    const Value valueKey = OBJ_VAL(copyString(vm, key, (int)strlen(key)));
    push(vm, valueKey);

    const Value value = NUMBER_VAL(datum.u.d);
    push(vm, value);

    mapSet(vm, map, valueKey, value);
    pop(vm);
    pop(vm);
}

static void setMapMap(VM *vm, ObjMap *map, ObjMap *valueMap, const char *key) {
    const Value valueKey = OBJ_VAL(copyString(vm, key, (int)strlen(key)));
    push(vm, valueKey);

    const Value value = OBJ_VAL(valueMap);
    push(vm, value);

    mapSet(vm, map, valueKey, value);
    pop(vm);
    pop(vm);
}

static void appendString(VM *vm, ObjArray *array, const toml_datum_t datum) {
    const Value value = OBJ_VAL(copyString(vm, datum.u.s, (int)strlen(datum.u.s)));
    push(vm, value);

    writeValueArray(vm, &array->data, value);
    pop(vm);
}

static void appendBool(VM *vm, ObjArray *array, const toml_datum_t datum) {
    const Value value = BOOL_VAL(datum.u.b);
    push(vm, value);

    writeValueArray(vm, &array->data, value);
    pop(vm);
}

static void appendInt(VM *vm, ObjArray *array, const toml_datum_t datum) {
    const Value value = NUMBER_VAL(datum.u.i);
    push(vm, value);

    writeValueArray(vm, &array->data, value);
    pop(vm);
}

static void appendDouble(VM *vm, ObjArray *array, const toml_datum_t datum) {
    const Value value = NUMBER_VAL(datum.u.d);
    push(vm, value);

    writeValueArray(vm, &array->data, value);
    pop(vm);
}

static void appendMap(VM *vm, ObjArray *array, ObjMap *map) {
    const Value value = OBJ_VAL(map);
    push(vm, value);

    writeValueArray(vm, &array->data, value);
    pop(vm);
}

static void appendArray(VM *vm, ObjArray *array, ObjArray *subArray) {
    const Value value = OBJ_VAL(subArray);
    push(vm, value);

    writeValueArray(vm, &array->data, value);
    pop(vm);
}

static void setArrayValueFromToml(VM *vm, ObjArray *array, const toml_array_t *arr) {
    for (int i = 0;; ++i) {
        toml_datum_t datum = toml_string_at(arr, i);
        if (datum.ok) {
            appendString(vm, array, datum);
            continue;
        }

        datum = toml_bool_at(arr, i);
        if (datum.ok) {
            appendBool(vm, array, datum);
            continue;
        }

        datum = toml_int_at(arr, i);
        if (datum.ok) {
            appendInt(vm, array, datum);
            continue;
        }

        datum = toml_double_at(arr, i);
        if (datum.ok) {
            appendDouble(vm, array, datum);
            continue;
        }

        const toml_table_t *table = toml_table_at(arr, i);
        if (table != NULL) {
            ObjMap *map = newMap(vm);
            setMapValuesFromToml(vm, map, table);
            appendMap(vm, array, map);
            continue;
        }

        const toml_array_t *subArr = toml_array_at(arr, i);
        if (subArr != NULL) {
            ObjArray *subArray = newArray(vm);
            setArrayValueFromToml(vm, subArray, subArr);
            appendArray(vm, array, subArray);
            continue;
        }

        break;
    }
}

static void setMapArray(VM *vm, ObjMap *map, const toml_array_t *arr, const char *key) {
    const Value valueKey = OBJ_VAL(copyString(vm, key, (int)strlen(key)));
    push(vm, valueKey);

    ObjArray *array = newArray(vm);
    setArrayValueFromToml(vm, array, arr);

    const Value value = OBJ_VAL(array);
    push(vm, value);

    mapSet(vm, map, valueKey, value);
    pop(vm);
    pop(vm);
}

void setMapValuesFromToml(VM *vm, ObjMap *map, const toml_table_t *conf) {
    for (int i = 0;; ++i) {
        const char *key = toml_key_in(conf, i);
        if (!key) {
            break;
        }

        toml_datum_t datum = toml_string_in(conf, key);
        if (datum.ok) {
            setMapString(vm, map, datum, key);
            continue;
        }

        datum = toml_bool_in(conf, key);
        if (datum.ok) {
            setMapBool(vm, map, datum, key);
            continue;
        }

        datum = toml_int_in(conf, key);
        if (datum.ok) {
            setMapInt(vm, map, datum, key);
            continue;
        }

        datum = toml_double_in(conf, key);
        if (datum.ok) {
            setMapDouble(vm, map, datum, key);
            continue;
        }

        const toml_table_t *table = toml_table_in(conf, key);
        if (table != NULL) {
            ObjMap *subMap = newMap(vm);
            setMapValuesFromToml(vm, subMap, table);
            setMapMap(vm, map, subMap, key);
            continue;
        }

        const toml_array_t *arr = toml_array_in(conf, key);
        if (arr != NULL) {
            setMapArray(vm, map, arr, key);
        }
    }
}

static Value tomlParseFile(VM *vm, const int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function parseFile() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function parseFile() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    const ObjString *file = AS_STRING(args[0]);
    FILE *fp = fopen(file->str, "r");
    if (fp == NULL) {
        return NULL_VAL; // TODO: error
    }

    char errbuf[200];
    const toml_table_t *conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (conf == NULL) {
        return NULL_VAL; // TODO: error
    }

    ObjMap *map = newMap(vm);
    setMapValuesFromToml(vm, map, conf);

    return OBJ_VAL(map);
}

static Value tomlParseString(VM *vm, const int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function parseString() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function parseString() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    const ObjString *string = AS_STRING(args[0]);

    char errbuf[200];
    const toml_table_t *conf = toml_parse(string->str, errbuf, sizeof(errbuf));
    if (conf == NULL) {
        return NULL_VAL; // TODO: error
    }

    ObjMap *map = newMap(vm);
    setMapValuesFromToml(vm, map, conf);

    return OBJ_VAL(map);
}

static Value tomlMapToToml(VM *vm, const int argc, const Value *args) {
    if (argc != 1 && argc != 2) {
        runtimeError(vm, "Function stringify() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_MAP(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function stringify() expected type 'map' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    int ident = 4;

    if (argc == 2) {
        if (!IS_NUMBER(args[1])) {
            char *type = valueType(args[0]);
            runtimeError(vm, "Function stringify() expected type 'number' for the second argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        ident = AS_NUMBER(args[1]);
    }

    return TRUE_VAL;
}

Value useTomlLib(VM *vm) {
    ObjString *name = copyString(vm, "toml", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "parseFile", tomlParseFile, &lib->values);
    defineNative(vm, "parseString", tomlParseString, &lib->values);
    defineNative(vm, "stringify", tomlMapToToml, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
