//
// Created by Skyler on 9/19/22.
//

#include "lib_json.h"

#include "../memory.h"
#include "../vm.h"

#include <stdlib.h>

static Value parseJSON(VM *vm, const json_value *json) {
    switch (json->type) {
        case json_none:
        case json_null: return NULL_VAL;
        case json_object: {
            ObjMap *map = newMap(vm);
            push(vm, OBJ_VAL(map));

            for (unsigned int i = 0; i < json->u.object.length; ++i) {
                Value val = parseJSON(vm, json->u.object.values[i].value);
                push(vm, val);
                Value key = OBJ_VAL(copyString(vm, json->u.object.values[i].name, json->u.object.values[i].name_length));
                push(vm, key);
                mapSet(vm, map, key, val);
                pop(vm);
                pop(vm);
            }

            pop(vm);
            return OBJ_VAL(map);
        }
        case json_array: {
            ObjArray *array = newArray(vm);
            push(vm, OBJ_VAL(array));

            for (unsigned int i = 0; i < json->u.array.length; i++) {
                Value val = parseJSON(vm, json->u.array.values[i]);
                push(vm, val);
                writeValueArray(vm, &array->data, val);
                pop(vm);
            }

            pop(vm);
            return OBJ_VAL(array);
        }
        case json_integer: return NUMBER_VAL(json->u.integer);
        case json_double: return NUMBER_VAL(json->u.dbl);
        case json_string: return OBJ_VAL(copyString(vm, json->u.string.ptr, json->u.string.length));
        case json_boolean: return BOOL_VAL(json->u.boolean);
        default: return ERROR_VAL;
    }
}

static Value jsonParse(VM *vm, int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function parse() expected 1 argument but got %d.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function parse() expected type 'string' but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    ObjString *jsonStr = AS_STRING(args[0]);
    json_value *jsonObj = json_parse(jsonStr->str, jsonStr->len);

    if (jsonObj == NULL) {
        return NULL_VAL;
    }

    Value value = parseJSON(vm, jsonObj);
    if (value == ERROR_VAL) {
        return NULL_VAL;
    }

    json_value_free(jsonObj);
    return value;
}

json_value *stringifyJSON(VM *vm, Value value) {
    if (IS_NULL(value)) {
        return json_null_new();
    } else if (IS_BOOL(value)) {
        return json_boolean_new(AS_BOOL(value));
    } else if (IS_NUMBER(value)) {
        double num = AS_NUMBER(value);
        if ((int)num == num) {
            return json_integer_new((int)num);
        }

        return json_double_new(num);
    } else if (IS_OBJ(value)) {
        switch (AS_OBJ(value)->type) {
            case OBJ_STRING: return json_string_new(AS_CSTRING(value));
            case OBJ_ARRAY: {
                ObjArray *array = AS_ARRAY(value);
                json_value *jsonValue = json_array_new(array->data.count);

                for (int i = 0; i < array->data.count; i++) {
                    json_array_push(jsonValue, stringifyJSON(vm, array->data.values[i]));
                }

                return jsonValue;
            }
            case OBJ_MAP: {
                ObjMap *map = AS_MAP(value);
                json_value *jsonValue = json_object_new(map->count);

                for (int i = 0; i <= map->capacity; i++) {
                    MapItem *item = &map->items[i];
                    if (IS_ERR(item->key)) {
                        continue;
                    }

                    char *key;
                    if (IS_STRING(item->key)) {
                        ObjString *str = AS_STRING(item->key);
                        key = str->str;
                    } else if (IS_NULL(item->key)) {
                        key = malloc(5);
                        memcpy(key, "null", 4);
                        key[4] = '\0';
                    } else {
                        key = valueToString(item->key);
                    }

                    json_object_push(jsonValue, key, stringifyJSON(vm, item->value));

                    if (!IS_STRING(item->key)) {
                        free(key);
                    }
                }

                return jsonValue;
            }
            default: {}
        }
    }

    return NULL;
}

static Value jsonStringify(VM *vm, int argc, const Value *args) {
    if (argc != 1 && argc != 2) {
        runtimeError(vm, "Function stringify() expected 1 or 2 arguments but got %d.", argc);
        return ERROR_VAL;
    }

    int indent = 4;
    int lineType = json_serialize_mode_single_line;

    if (argc == 2) {
        if (!IS_NUMBER(args[1])) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function stringify() expected type 'number' but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        indent = AS_NUMBER(args[1]);
        lineType = json_serialize_mode_multiline;
    }

    json_value *jsonValue = stringifyJSON(vm, args[0]);
    if (jsonValue == NULL) {
        return NULL_VAL;
    }

    json_serialize_opts defaultOps = {
            lineType,
            json_serialize_opt_pack_brackets,
            indent,
    };

    int len = (int)json_measure_ex(jsonValue, defaultOps);
    char *buf = ALLOCATE(vm, char, len);
    json_serialize_ex(buf, jsonValue, defaultOps);
    int actualLen = (int)strlen(buf);
    if (actualLen != len) {
        buf = SHRINK_ARRAY(vm, buf, char, len, actualLen + 1);
    }

    ObjString *str = takeString(vm, buf, actualLen);
    json_builder_free(jsonValue);

    return OBJ_VAL(str);
}

Value useJsonLib(VM *vm) {
    ObjString *name = copyString(vm, "json", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));
    
    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "parse", jsonParse, &lib->values);
    defineNative(vm, "stringify", jsonStringify, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
