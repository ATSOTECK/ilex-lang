//
// Created by Skyler Burwell on 2/19/25.
//

#include "lib_http.h"

#include "../memory.h"

#include <stdlib.h>
#include <curl/curl.h>

#define HTTP_METHOD_GET     "GET"
#define HTTP_METHOD_HEAD    "HEAD"
#define HTTP_METHOD_POST    "POST"
#define HTTP_METHOD_PUT     "PUT"
#define HTTP_METHOD_PATCH   "PATCH" // RFC 5789
#define HTTP_METHOD_DELETE  "DELETE"
#define HTTP_METHOD_CONNECT "CONNECT"
#define HTTP_METHOD_OPTIONS "OPTIONS"
#define HTTP_METHOD_TRACE   "TRACE"

#define HTTP_STATUS_CODE_CONTINUE                        100 // RFC 7231, 6.2.1
#define HTTP_STATUS_CODE_SWITCHING_PROTOCOLS             101 // RFC 7231, 6.2.2
#define HTTP_STATUS_CODE_PROCESSING                      102 // RFC 2518, 10.1
#define HTTP_STATUS_CODE_EARLY_HINTS                     103 // RFC 8297

#define HTTP_STATUS_CODE_OK                              200 // RFC 7231, 6.3.1
#define HTTP_STATUS_CODE_CREATED                         201 // RFC 7231, 6.3.2
#define HTTP_STATUS_CODE_ACCEPTED                        202 // RFC 7231, 6.3.3
#define HTTP_STATUS_CODE_NON_AUTHORITATIVE_INFO          203 // RFC 7231, 6.3.4
#define HTTP_STATUS_CODE_NO_CONTENT                      204 // RFC 7231, 6.3.5
#define HTTP_STATUS_CODE_RESET_CONTENT                   205 // RFC 7231, 6.3.6
#define HTTP_STATUS_CODE_PARTIAL_CONTENT                 206 // RFC 7233, 4.1
#define HTTP_STATUS_CODE_MULTI_STATUS                    207 // RFC 4918, 11.1
#define HTTP_STATUS_CODE_ALREADY_REPORTED                208 // RFC 5842, 7.1
#define HTTP_STATUS_CODE_IM_USED                         226 // RFC 3229, 10.4.1

#define HTTP_STATUS_CODE_MULTIPLE_CHOICES                300 // RFC 7231, 6.4.1
#define HTTP_STATUS_CODE_MOVED_PERMANENTLY               301 // RFC 7231, 6.4.2
#define HTTP_STATUS_CODE_FOUND                           302 // RFC 7231, 6.4.3
#define HTTP_STATUS_CODE_SEE_OTHER                       303 // RFC 7231, 6.4.4
#define HTTP_STATUS_CODE_NOT_MODIFIED                    304 // RFC 7232, 4.1
#define HTTP_STATUS_CODE_USE_PROXY                       305 // RFC 7231, 6.4.5
#define HTTP_STATUS_CODE_TEMPORARY_REDIRECT              307 // RFC 7231, 6.4.7
#define HTTP_STATUS_CODE_PERMANENT_REDIRECT              308 // RFC 7538, 3

#define HTTP_STATUS_CODE_BAD_REQUEST                     400 // RFC 7231, 6.5.1
#define HTTP_STATUS_CODE_UNAUTHORIZED                    401 // RFC 7235, 3.1
#define HTTP_STATUS_CODE_PAYMENT_REQUIRED                402 // RFC 7231, 6.5.2
#define HTTP_STATUS_CODE_FORBIDDEN                       403 // RFC 7231, 6.5.3
#define HTTP_STATUS_CODE_NOT_FOUND                       404 // RFC 7231, 6.5.4
#define HTTP_STATUS_CODE_METHOD_NOT_ALLOWED              405 // RFC 7231, 6.5.5
#define HTTP_STATUS_CODE_NOT_ACCEPTABLE                  406 // RFC 7231, 6.5.6
#define HTTP_STATUS_CODE_PROXY_AUTH_REQUIRED             407 // RFC 7235, 3.2
#define HTTP_STATUS_CODE_REQUEST_TIMEOUT                 408 // RFC 7231, 6.5.7
#define HTTP_STATUS_CODE_CONFLICT                        409 // RFC 7231, 6.5.8
#define HTTP_STATUS_CODE_GONE                            410 // RFC 7231, 6.5.9
#define HTTP_STATUS_CODE_LENGTH_REQUIRED                 411 // RFC 7231, 6.5.10
#define HTTP_STATUS_CODE_PRECONDITION_FAILED             412 // RFC 7232, 4.2
#define HTTP_STATUS_CODE_REQUEST_ENTITY_TOO_LARGE        413 // RFC 7231, 6.5.11
#define HTTP_STATUS_CODE_REQUEST_URI_TOO_LONG            414 // RFC 7231, 6.5.12
#define HTTP_STATUS_CODE_UNSUPPORTED_MEDIA_TYPE          415 // RFC 7231, 6.5.13
#define HTTP_STATUS_CODE_REQUESTED_RANGE_NOT_SATISFIABLE 416 // RFC 7233, 4.4
#define HTTP_STATUS_CODE_EXPECTATION_FAILED              417 // RFC 7231, 6.5.14
#define HTTP_STATUS_CODE_TEAPOT                          418 // RFC 7168, 2.3.3
#define HTTP_STATUS_CODE_MISDIRECTED_REQUEST             421 // RFC 7540, 9.1.2
#define HTTP_STATUS_CODE_UNPROCESSABLE_ENTITY            422 // RFC 4918, 11.2
#define HTTP_STATUS_CODE_LOCKED                          423 // RFC 4918, 11.3
#define HTTP_STATUS_CODE_FAILED_DEPENDENCY               424 // RFC 4918, 11.4
#define HTTP_STATUS_CODE_TOO_EARLY                       425 // RFC 8470, 5.2.
#define HTTP_STATUS_CODE_UPGRADE_REQUIRED                426 // RFC 7231, 6.5.15
#define HTTP_STATUS_CODE_PRECONDITION_REQUIRED           428 // RFC 6585, 3
#define HTTP_STATUS_CODE_TOO_MANY_REQUESTS               429 // RFC 6585, 4
#define HTTP_STATUS_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE 431 // RFC 6585, 5
#define HTTP_STATUS_CODE_UNAVAILABLE_FOR_LEGAL_REASONS   451 // RFC 7725, 3

#define HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR           500 // RFC 7231, 6.6.1
#define HTTP_STATUS_CODE_NOT_IMPLEMENTED                 501 // RFC 7231, 6.6.2
#define HTTP_STATUS_CODE_BAD_GATEWAY                     502 // RFC 7231, 6.6.3
#define HTTP_STATUS_CODE_SERVICE_UNAVAILABLE             503 // RFC 7231, 6.6.4
#define HTTP_STATUS_CODE_GATEWAY_TIMEOUT                 504 // RFC 7231, 6.6.5
#define HTTP_STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED      505 // RFC 7231, 6.6.6
#define HTTP_STATUS_CODE_VARIANT_ALSO_NEGOTIATES         506 // RFC 2295, 8.1
#define HTTP_STATUS_CODE_INSUFFICIENT_STORAGE            507 // RFC 4918, 11.5
#define HTTP_STATUS_CODE_LOOP_DETECTED                   508 // RFC 5842, 7.2
#define HTTP_STATUS_CODE_NOT_EXTENDED                    510 // RFC 2774, 7
#define HTTP_STATUS_CODE_NETWORK_AUTHENTICATION_REQUIRED 511 // RFC 6585, 6

#define HTTP_STATUS_MESSAGE_CONTINUE                        "Continue"
#define HTTP_STATUS_MESSAGE_SWITCHING_PROTOCOLS             "Switching Protocols"
#define HTTP_STATUS_MESSAGE_PROCESSING                      "Processing"
#define HTTP_STATUS_MESSAGE_EARLY_HINTS                     "Early Hints"

#define HTTP_STATUS_MESSAGE_OK                              "OK"
#define HTTP_STATUS_MESSAGE_CREATED                         "Created"
#define HTTP_STATUS_MESSAGE_ACCEPTED                        "Accepted"
#define HTTP_STATUS_MESSAGE_NON_AUTHORITATIVE_INFO          "Non-Authoritative Information"
#define HTTP_STATUS_MESSAGE_NO_CONTENT                      "No Content"
#define HTTP_STATUS_MESSAGE_RESET_CONTENT                   "Reset Content"
#define HTTP_STATUS_MESSAGE_PARTIAL_CONTENT                 "Partial Content"
#define HTTP_STATUS_MESSAGE_MULTI_STATUS                    "Multi-Status"
#define HTTP_STATUS_MESSAGE_ALREADY_REPORTED                "Already Reported"
#define HTTP_STATUS_MESSAGE_IM_USED                         "IM Used"

#define HTTP_STATUS_MESSAGE_MULTIPLE_CHOICES                "Multiple Choices"
#define HTTP_STATUS_MESSAGE_MOVED_PERMANENTLY               "Moved Permanently"
#define HTTP_STATUS_MESSAGE_FOUND                           "Found"
#define HTTP_STATUS_MESSAGE_SEE_OTHER                       "See Other"
#define HTTP_STATUS_MESSAGE_NOT_MODIFIED                    "Not Modified"
#define HTTP_STATUS_MESSAGE_USE_PROXY                       "Use Proxy"
#define HTTP_STATUS_MESSAGE_TEMPORARY_REDIRECT              "Temporary Redirect"
#define HTTP_STATUS_MESSAGE_PERMANENT_REDIRECT              "Permanent Redirect"

#define HTTP_STATUS_MESSAGE_BAD_REQUEST                     "Bad Request"
#define HTTP_STATUS_MESSAGE_UNAUTHORIZED                    "Unauthorized"
#define HTTP_STATUS_MESSAGE_PAYMENT_REQUIRED                "Payment Required"
#define HTTP_STATUS_MESSAGE_FORBIDDEN                       "Forbidden"
#define HTTP_STATUS_MESSAGE_NOT_FOUND                       "Not Found"
#define HTTP_STATUS_MESSAGE_METHOD_NOT_ALLOWED              "Method Not Allowed"
#define HTTP_STATUS_MESSAGE_NOT_ACCEPTABLE                  "Not Acceptable"
#define HTTP_STATUS_MESSAGE_PROXY_AUTH_REQUIRED             "Proxy Authentication Required"
#define HTTP_STATUS_MESSAGE_REQUEST_TIMEOUT                 "Request Timeout"
#define HTTP_STATUS_MESSAGE_CONFLICT                        "Conflict"
#define HTTP_STATUS_MESSAGE_GONE                            "Gone"
#define HTTP_STATUS_MESSAGE_LENGTH_REQUIRED                 "Length Required"
#define HTTP_STATUS_MESSAGE_PRECONDITION_FAILED             "Precondition Failed"
#define HTTP_STATUS_MESSAGE_REQUEST_ENTITY_TOO_LARGE        "Request Entity Too Large"
#define HTTP_STATUS_MESSAGE_REQUEST_URI_TOO_LONG            "Request URI Too Long"
#define HTTP_STATUS_MESSAGE_UNSUPPORTED_MEDIA_TYPE          "Unsupported Media Type"
#define HTTP_STATUS_MESSAGE_REQUESTED_RANGE_NOT_SATISFIABLE "Requested Range Not Satisfiable"
#define HTTP_STATUS_MESSAGE_EXPECTATION_FAILED              "Expectation Failed"
#define HTTP_STATUS_MESSAGE_TEAPOT                          "I'm a teapot"
#define HTTP_STATUS_MESSAGE_MISDIRECTED_REQUEST             "Misdirected Request"
#define HTTP_STATUS_MESSAGE_UNPROCESSABLE_ENTITY            "Unprocessable Entity"
#define HTTP_STATUS_MESSAGE_LOCKED                          "Locked"
#define HTTP_STATUS_MESSAGE_FAILED_DEPENDENCY               "Failed Dependency"
#define HTTP_STATUS_MESSAGE_TOO_EARLY                       "Too Early"
#define HTTP_STATUS_MESSAGE_UPGRADE_REQUIRED                "Upgrade Required"
#define HTTP_STATUS_MESSAGE_PRECONDITION_REQUIRED           "Precondition Required"
#define HTTP_STATUS_MESSAGE_TOO_MANY_REQUESTS               "Too Many Requests"
#define HTTP_STATUS_MESSAGE_REQUEST_HEADER_FIELDS_TOO_LARGE "Request Header Fields Too Large"
#define HTTP_STATUS_MESSAGE_UNAVAILABLE_FOR_LEGAL_REASONS   "Unavailable For Legal Reasons"

#define HTTP_STATUS_MESSAGE_INTERNAL_SERVER_ERROR           "Internal Server Error"
#define HTTP_STATUS_MESSAGE_NOT_IMPLEMENTED                 "Not Implemented"
#define HTTP_STATUS_MESSAGE_BAD_GATEWAY                     "Bad Gateway"
#define HTTP_STATUS_MESSAGE_SERVICE_UNAVAILABLE             "Service Unavailable"
#define HTTP_STATUS_MESSAGE_GATEWAY_TIMEOUT                 "Gateway Timeout"
#define HTTP_STATUS_MESSAGE_HTTP_VERSION_NOT_SUPPORTED      "HTTP Version Not Supported"
#define HTTP_STATUS_MESSAGE_VARIANT_ALSO_NEGOTIATES         "Variant Also Negotiates"
#define HTTP_STATUS_MESSAGE_INSUFFICIENT_STORAGE            "Insufficient Storage"
#define HTTP_STATUS_MESSAGE_LOOP_DETECTED                   "Loop Detected"
#define HTTP_STATUS_MESSAGE_NOT_EXTENDED                    "Not Extended"
#define HTTP_STATUS_MESSAGE_NETWORK_AUTHENTICATION_REQUIRED "Network Authentication Required"
#define HTTP_STATUS_MESSAGE_UNKNOWN                         "Unknown Status"

#define DEFAULT_REQUEST_TIMEOUT 20

static const char *httpStatusCodeToMessage(const int status) {
    switch (status) {
        case HTTP_STATUS_CODE_CONTINUE:                        return HTTP_STATUS_MESSAGE_CONTINUE;
        case HTTP_STATUS_CODE_SWITCHING_PROTOCOLS:             return HTTP_STATUS_MESSAGE_SWITCHING_PROTOCOLS;
        case HTTP_STATUS_CODE_PROCESSING:                      return HTTP_STATUS_MESSAGE_PROCESSING;
        case HTTP_STATUS_CODE_EARLY_HINTS:                     return HTTP_STATUS_MESSAGE_EARLY_HINTS;
        case HTTP_STATUS_CODE_OK:                              return HTTP_STATUS_MESSAGE_OK;
        case HTTP_STATUS_CODE_CREATED:                         return HTTP_STATUS_MESSAGE_CREATED;
        case HTTP_STATUS_CODE_ACCEPTED:                        return HTTP_STATUS_MESSAGE_ACCEPTED;
        case HTTP_STATUS_CODE_NON_AUTHORITATIVE_INFO:          return HTTP_STATUS_MESSAGE_NON_AUTHORITATIVE_INFO;
        case HTTP_STATUS_CODE_NO_CONTENT:                      return HTTP_STATUS_MESSAGE_NO_CONTENT;
        case HTTP_STATUS_CODE_RESET_CONTENT:                   return HTTP_STATUS_MESSAGE_RESET_CONTENT;
        case HTTP_STATUS_CODE_PARTIAL_CONTENT:                 return HTTP_STATUS_MESSAGE_PARTIAL_CONTENT;
        case HTTP_STATUS_CODE_MULTI_STATUS:                    return HTTP_STATUS_MESSAGE_MULTI_STATUS;
        case HTTP_STATUS_CODE_ALREADY_REPORTED:                return HTTP_STATUS_MESSAGE_ALREADY_REPORTED;
        case HTTP_STATUS_CODE_IM_USED:                         return HTTP_STATUS_MESSAGE_IM_USED;
        case HTTP_STATUS_CODE_MULTIPLE_CHOICES:                return HTTP_STATUS_MESSAGE_MULTIPLE_CHOICES;
        case HTTP_STATUS_CODE_MOVED_PERMANENTLY:               return HTTP_STATUS_MESSAGE_MOVED_PERMANENTLY;
        case HTTP_STATUS_CODE_FOUND:                           return HTTP_STATUS_MESSAGE_FOUND;
        case HTTP_STATUS_CODE_SEE_OTHER:                       return HTTP_STATUS_MESSAGE_SEE_OTHER;
        case HTTP_STATUS_CODE_NOT_MODIFIED:                    return HTTP_STATUS_MESSAGE_NOT_MODIFIED;
        case HTTP_STATUS_CODE_USE_PROXY:                       return HTTP_STATUS_MESSAGE_USE_PROXY;
        case HTTP_STATUS_CODE_TEMPORARY_REDIRECT:              return HTTP_STATUS_MESSAGE_TEMPORARY_REDIRECT;
        case HTTP_STATUS_CODE_PERMANENT_REDIRECT:              return HTTP_STATUS_MESSAGE_PERMANENT_REDIRECT;
        case HTTP_STATUS_CODE_BAD_REQUEST:                     return HTTP_STATUS_MESSAGE_BAD_REQUEST;
        case HTTP_STATUS_CODE_UNAUTHORIZED:                    return HTTP_STATUS_MESSAGE_UNAUTHORIZED;
        case HTTP_STATUS_CODE_PAYMENT_REQUIRED:                return HTTP_STATUS_MESSAGE_PAYMENT_REQUIRED;
        case HTTP_STATUS_CODE_FORBIDDEN:                       return HTTP_STATUS_MESSAGE_FORBIDDEN;
        case HTTP_STATUS_CODE_NOT_FOUND:                       return HTTP_STATUS_MESSAGE_NOT_FOUND;
        case HTTP_STATUS_CODE_METHOD_NOT_ALLOWED:              return HTTP_STATUS_MESSAGE_METHOD_NOT_ALLOWED;
        case HTTP_STATUS_CODE_NOT_ACCEPTABLE:                  return HTTP_STATUS_MESSAGE_NOT_ACCEPTABLE;
        case HTTP_STATUS_CODE_PROXY_AUTH_REQUIRED:             return HTTP_STATUS_MESSAGE_PROXY_AUTH_REQUIRED;
        case HTTP_STATUS_CODE_REQUEST_TIMEOUT:                 return HTTP_STATUS_MESSAGE_REQUEST_TIMEOUT;
        case HTTP_STATUS_CODE_CONFLICT:                        return HTTP_STATUS_MESSAGE_CONFLICT;
        case HTTP_STATUS_CODE_GONE:                            return HTTP_STATUS_MESSAGE_GONE;
        case HTTP_STATUS_CODE_LENGTH_REQUIRED:                 return HTTP_STATUS_MESSAGE_LENGTH_REQUIRED;
        case HTTP_STATUS_CODE_PRECONDITION_FAILED:             return HTTP_STATUS_MESSAGE_PRECONDITION_FAILED;
        case HTTP_STATUS_CODE_REQUEST_ENTITY_TOO_LARGE:        return HTTP_STATUS_MESSAGE_REQUEST_ENTITY_TOO_LARGE;
        case HTTP_STATUS_CODE_REQUEST_URI_TOO_LONG:            return HTTP_STATUS_MESSAGE_REQUEST_URI_TOO_LONG;
        case HTTP_STATUS_CODE_UNSUPPORTED_MEDIA_TYPE:          return HTTP_STATUS_MESSAGE_UNSUPPORTED_MEDIA_TYPE;
        case HTTP_STATUS_CODE_REQUESTED_RANGE_NOT_SATISFIABLE: return HTTP_STATUS_MESSAGE_REQUESTED_RANGE_NOT_SATISFIABLE;
        case HTTP_STATUS_CODE_EXPECTATION_FAILED:              return HTTP_STATUS_MESSAGE_EXPECTATION_FAILED;
        case HTTP_STATUS_CODE_TEAPOT:                          return HTTP_STATUS_MESSAGE_TEAPOT;
        case HTTP_STATUS_CODE_MISDIRECTED_REQUEST:             return HTTP_STATUS_MESSAGE_MISDIRECTED_REQUEST;
        case HTTP_STATUS_CODE_UNPROCESSABLE_ENTITY:            return HTTP_STATUS_MESSAGE_UNPROCESSABLE_ENTITY;
        case HTTP_STATUS_CODE_LOCKED:                          return HTTP_STATUS_MESSAGE_LOCKED;
        case HTTP_STATUS_CODE_FAILED_DEPENDENCY:               return HTTP_STATUS_MESSAGE_FAILED_DEPENDENCY;
        case HTTP_STATUS_CODE_TOO_EARLY:                       return HTTP_STATUS_MESSAGE_TOO_EARLY;
        case HTTP_STATUS_CODE_UPGRADE_REQUIRED:                return HTTP_STATUS_MESSAGE_UPGRADE_REQUIRED;
        case HTTP_STATUS_CODE_PRECONDITION_REQUIRED:           return HTTP_STATUS_MESSAGE_PRECONDITION_REQUIRED;
        case HTTP_STATUS_CODE_TOO_MANY_REQUESTS:               return HTTP_STATUS_MESSAGE_TOO_MANY_REQUESTS;
        case HTTP_STATUS_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE: return HTTP_STATUS_MESSAGE_REQUEST_HEADER_FIELDS_TOO_LARGE;
        case HTTP_STATUS_CODE_UNAVAILABLE_FOR_LEGAL_REASONS:   return HTTP_STATUS_MESSAGE_UNAVAILABLE_FOR_LEGAL_REASONS;
        case HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR:           return HTTP_STATUS_MESSAGE_INTERNAL_SERVER_ERROR;
        case HTTP_STATUS_CODE_NOT_IMPLEMENTED:                 return HTTP_STATUS_MESSAGE_NOT_IMPLEMENTED;
        case HTTP_STATUS_CODE_BAD_GATEWAY:                     return HTTP_STATUS_MESSAGE_BAD_GATEWAY;
        case HTTP_STATUS_CODE_SERVICE_UNAVAILABLE:             return HTTP_STATUS_MESSAGE_SERVICE_UNAVAILABLE;
        case HTTP_STATUS_CODE_GATEWAY_TIMEOUT:                 return HTTP_STATUS_MESSAGE_GATEWAY_TIMEOUT;
        case HTTP_STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED:      return HTTP_STATUS_MESSAGE_HTTP_VERSION_NOT_SUPPORTED;
        case HTTP_STATUS_CODE_VARIANT_ALSO_NEGOTIATES:         return HTTP_STATUS_MESSAGE_VARIANT_ALSO_NEGOTIATES;
        case HTTP_STATUS_CODE_INSUFFICIENT_STORAGE:            return HTTP_STATUS_MESSAGE_INSUFFICIENT_STORAGE;
        case HTTP_STATUS_CODE_LOOP_DETECTED:                   return HTTP_STATUS_MESSAGE_LOOP_DETECTED;
        case HTTP_STATUS_CODE_NOT_EXTENDED:                    return HTTP_STATUS_MESSAGE_NOT_EXTENDED;
        case HTTP_STATUS_CODE_NETWORK_AUTHENTICATION_REQUIRED: return HTTP_STATUS_MESSAGE_NETWORK_AUTHENTICATION_REQUIRED;
        default: return HTTP_STATUS_MESSAGE_UNKNOWN;
    }
}

static void createResponse(VM *vm, Response *response) {
    response->vm = vm;
    response->headers = newMap(vm);

    push(vm, OBJ_VAL(response->headers));

    response->len = 0;
    response->res = NULL;
    response->firstIteration = true;
}

static size_t writeResponse(const char *ptr, const size_t size, const size_t nmemb, void *data) {
    Response *response = (Response *)data;
    const size_t new_len = response->len + size * nmemb;
    response->res = GROW_ARRAY(response->vm, char, response->res, response->len + !response->firstIteration, new_len + 1);
    response->firstIteration = false;

    if (response->res == NULL) {
        printf("Unable to allocate memory\n");
        exit(71);
    }

    memcpy(response->res + response->len, ptr, size * nmemb);
    response->res[new_len] = '\0';
    response->len = new_len;

    return size * nmemb;
}

static size_t writeHeaders(const char *ptr, const size_t size, const size_t nitems, void *data) {
    const Response *response = (Response *)data;
    // If nitems equals 2 it's an empty header
    if (nitems != 2) {
        const ObjString *str = copyString(response->vm, ptr, (int)((nitems - 2) * size));

        const char *key = strtok(str->str, ": ");
        const char *value = strtok(NULL, ": ");

        const Value keyValue = OBJ_VAL(copyString(response->vm, key, strlen(key)));
        const Value valueValue = OBJ_VAL(copyString(response->vm, value, strlen(value)));

        push(response->vm, keyValue);
        push(response->vm, valueValue);
        mapSet(response->vm, response->headers, keyValue, valueValue);
        pop(response->vm);
        pop(response->vm);
    }

    return size * nitems;
}

static bool setRequestHeaders(VM *vm, struct curl_slist *list, CURL *curl, ObjMap *headers) {
    if (headers->count == 0) {
        return true;
    }

    const ValueArray keys = mapKeys(vm, headers);
    const ValueArray values = mapKeys(vm, headers);

    for (int i = 0; i < keys.count; ++i) {
        if (!IS_STRING(keys.values[i]) || !IS_STRING(values.values[i])) {
            runtimeError(vm, "Headers list must only contain strings");
            return false;
        }

        const char *keyStr = AS_CSTRING(keys.values[i]);
        const char *valueStr = AS_CSTRING(values.values[i]);
        char *headerItem = (char*)malloc(sizeof(char) * (strlen(keyStr) + strlen(valueStr) + 3));

        sprintf(headerItem, "%s: %s", keyStr, valueStr);
        list = curl_slist_append(list, headerItem);

        free(headerItem);
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    return true;
}

static ObjMap *makeResponse(VM *vm, CURL *curl, Response response, const bool cleanup) {
    // Get status code
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);
    ObjString *body;
    if (response.res != NULL) {
        body = takeString(vm, response.res, (int)response.len);
    } else {
        body = copyString(vm, "", 0);
    }

    push(vm, OBJ_VAL(body));

    ObjMap *responseMap = newMap(vm);
    push(vm, OBJ_VAL(responseMap));

    ObjString *key = copyString(vm, "body", 4);
    push(vm, OBJ_VAL(key));
    mapSet(vm, responseMap, OBJ_VAL(key), OBJ_VAL(body));
    pop(vm);

    key = copyString(vm, "headers", 7);
    push(vm, OBJ_VAL(key));
    mapSet(vm, responseMap, OBJ_VAL(key), OBJ_VAL(response.headers));
    pop(vm);

    key = copyString(vm, "statusCode", 10);
    push(vm, OBJ_VAL(key));
    mapSet(vm, responseMap, OBJ_VAL(key), NUMBER_VAL(response.statusCode));
    pop(vm);

    key = copyString(vm, "status", 6);
    push(vm, OBJ_VAL(key));
    const char *statusMessage = httpStatusCodeToMessage(response.statusCode);
    mapSet(vm, responseMap, OBJ_VAL(key), OBJ_VAL(copyString(vm, statusMessage, strlen(statusMessage)))); // TODO: Status string.
    pop(vm);

    pop(vm); // map
    pop(vm); // content
    pop(vm); // headers from createResponse

    if (cleanup) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }

    return responseMap;
}

static ObjMap *makeResponseWithError(VM *vm, CURL *curl, Response response, const bool cleanup) {
    return NULL;
}

static char *mapToPayload(ObjMap *map) {
    int len = 100;
    char *ret = (char*)malloc(sizeof(char) * len);
    int currentLen = 0;

    for (int i = 0; i <= map->capacity; i++) {
        const MapItem *entry = &map->items[i];
        if (IS_ERR(entry->key)) {
            continue;
        }

        char *key;
        if (IS_STRING(entry->key)) {
            key = AS_CSTRING(entry->key);
        } else {
            key = valueToString(entry->key);
        }

        char *value;
        if (IS_STRING(entry->value)) {
            value = AS_CSTRING(entry->value);
        } else {
            value = valueToString(entry->value);
        }

        int keyLen = (int)strlen(key);
        int valLen = (int)strlen(value);

        if (currentLen + keyLen + valLen > len) {
            len = len * 2 + keyLen + valLen;
            ret = realloc(ret, len);

            if (ret == NULL) {
                printf("Unable to allocate memory\n");
                exit(71);
            }
        }

        memcpy(ret + currentLen, key, keyLen);
        currentLen += keyLen;
        memcpy(ret + currentLen, "=", 1);
        currentLen += 1;
        memcpy(ret + currentLen, value, valLen);
        currentLen += valLen;
        memcpy(ret + currentLen, "&", 1);
        currentLen += 1;

        if (!IS_STRING(entry->key)) {
            free(key);
        }
        if (!IS_STRING(entry->value)) {
            free(value);
        }
    }

    ret[currentLen] = '\0';

    return ret;
}

static Value httpGet(VM *vm, const int argc, Value *args) {
    if (argc < 1 || argc > 3) {
        runtimeError(vm, "Function get() expected 1 to 3 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function get() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    char *url = AS_CSTRING(args[0]);
    ObjMap *headers = NULL;
    int timeout = DEFAULT_REQUEST_TIMEOUT;

    if (argc >= 2) {
        if (!IS_MAP(args[1])) {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function get() expected type 'map' for the second argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        headers = AS_MAP(args[1]);
    }

    if (argc == 3) {
        if (!IS_NUMBER(args[2])) {
            char *type = valueType(args[2]);
            runtimeError(vm, "Function get() expected type 'number' for the third argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        timeout = AS_NUMBER(args[3]);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();

    if (curl) {
        Response response;
        createResponse(vm, &response);
        struct curl_slist *list = NULL;

        if (headers) {
            if (!setRequestHeaders(vm, list, curl, headers)) {
                curl_slist_free_all(list);
                return NULL_VAL;
            }
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeResponse);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writeHeaders);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        const CURLcode curlResponse = curl_easy_perform(curl);

        if (headers) {
            curl_slist_free_all(list);
        }

        if (curlResponse != CURLE_OK) {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            pop(vm);

            // char *errorString = (char *) curl_easy_strerror(curlResponse);
            return NULL_VAL; // TODO: Return map with error.
        }

        return OBJ_VAL(makeResponse(vm, curl, response, true));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    pop(vm);

    // char *errorString = (char *) curl_easy_strerror(CURLE_FAILED_INIT);
    return NULL_VAL; // TODO: Return map with error.
}

static Value httpPost(VM *vm, const int argc, Value *args) {
    if (argc < 1 || argc > 4) {
        runtimeError(vm, "Function post() expected 1 to 4 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    ObjMap *payloadMap = NULL;
    const ObjString *payloadString = NULL;
    ObjMap *headers = NULL;
    int timeout = DEFAULT_REQUEST_TIMEOUT;

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function post() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    if (argc >= 2) {
        if (IS_MAP(args[1])) {
            payloadMap = AS_MAP(args[1]);
        } else if (IS_STRING(args[1])) {
            payloadString = AS_STRING(args[1]);
        } else {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function post() expected type 'map' or 'string' for the second argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }
    }

    if (argc >= 3) {
        if (!IS_MAP(args[2])) {
            char *type = valueType(args[2]);
            runtimeError(vm, "Function post() expected type 'map' for the third argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        headers = AS_MAP(args[2]);
    }

    if (argc == 4) {
        if (!IS_NUMBER(args[3])) {
            char *type = valueType(args[3]);
            runtimeError(vm, "Function post() expected type 'number' for the fourth argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        timeout = (int)AS_NUMBER(args[3]);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();

    if (curl) {
        Response response;
        createResponse(vm, &response);
        char *url = AS_CSTRING(args[0]);
        char *payload = "";
        struct curl_slist *list = NULL;

        if (headers) {
            if (!setRequestHeaders(vm, list, curl, headers)) {
                curl_slist_free_all(list);
                return NULL_VAL;
            }
        }

        if (payloadMap != NULL) {
            payload = mapToPayload(payloadMap);
        } else if (payloadString != NULL) {
            payload = payloadString->str;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeResponse);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writeHeaders);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        const CURLcode curlResponse = curl_easy_perform(curl);

        if (headers) {
            curl_slist_free_all(list);
        }

        if (payloadMap) {
            free(payload);
        }

        if (curlResponse != CURLE_OK) {
            /* always cleanup */
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            pop(vm);

            // char *errorString = (char *) curl_easy_strerror(curlResponse);
            return NULL_VAL;
        }

        return OBJ_VAL(makeResponse(vm, curl, response, true));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    pop(vm);

    // char *errorString = (char *) curl_easy_strerror(CURLE_FAILED_INIT);
    return NULL_VAL;
}

static Value httpPut(VM *vm, const int argc, Value *args) {
    if (argc < 1 || argc > 4) {
        runtimeError(vm, "Function put() expected 1 to 4 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    ObjMap *payloadMap = NULL;
    const ObjString *payloadString = NULL;
    ObjMap *headers = NULL;
    int timeout = DEFAULT_REQUEST_TIMEOUT;

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function put() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    if (argc >= 2) {
        if (IS_MAP(args[1])) {
            payloadMap = AS_MAP(args[1]);
        } else if (IS_STRING(args[1])) {
            payloadString = AS_STRING(args[1]);
        } else {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function put() expected type 'map' or 'string' for the second argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }
    }

    if (argc >= 3) {
        if (!IS_MAP(args[2])) {
            char *type = valueType(args[2]);
            runtimeError(vm, "Function put() expected type 'map' for the third argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        headers = AS_MAP(args[2]);
    }

    if (argc == 4) {
        if (!IS_NUMBER(args[3])) {
            char *type = valueType(args[3]);
            runtimeError(vm, "Function put() expected type 'number' for the fourth argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        timeout = (int)AS_NUMBER(args[3]);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();

    if (curl) {
        Response response;
        createResponse(vm, &response);
        char *url = AS_CSTRING(args[0]);
        char *payload = "";
        struct curl_slist *list = NULL;

        if (headers) {
            if (!setRequestHeaders(vm, list, curl, headers)) {
                curl_slist_free_all(list);
                return NULL_VAL;
            }
        }

        if (payloadMap != NULL) {
            payload = mapToPayload(payloadMap);
        } else if (payloadString != NULL) {
            payload = payloadString->str;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeResponse);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writeHeaders);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        const CURLcode curlResponse = curl_easy_perform(curl);

        if (headers) {
            curl_slist_free_all(list);
        }

        if (payloadMap) {
            free(payload);
        }

        if (curlResponse != CURLE_OK) {
            /* always cleanup */
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            pop(vm);

            // char *errorString = (char *) curl_easy_strerror(curlResponse);
            return NULL_VAL;
        }

        return OBJ_VAL(makeResponse(vm, curl, response, true));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    pop(vm);

    // char *errorString = (char *) curl_easy_strerror(CURLE_FAILED_INIT);
    return NULL_VAL;
}

static Value httpHead(VM *vm, const int argc, Value *args) {
    if (argc < 1 || argc > 3) {
        runtimeError(vm, "Function head() expected 1 to 3 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function head() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    char *url = AS_CSTRING(args[0]);
    ObjMap *headers = NULL;
    int timeout = DEFAULT_REQUEST_TIMEOUT;

    if (argc >= 2) {
        if (!IS_MAP(args[1])) {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function head() expected type 'map' for the second argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        headers = AS_MAP(args[1]);
    }

    if (argc == 3) {
        if (!IS_NUMBER(args[2])) {
            char *type = valueType(args[2]);
            runtimeError(vm, "Function head() expected type 'number' for the third argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        timeout = AS_NUMBER(args[3]);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();

    if (curl) {
        Response response;
        createResponse(vm, &response);
        struct curl_slist *list = NULL;

        if (headers) {
            if (!setRequestHeaders(vm, list, curl, headers)) {
                curl_slist_free_all(list);
                return NULL_VAL;
            }
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writeHeaders);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        const CURLcode curlResponse = curl_easy_perform(curl);

        if (headers) {
            curl_slist_free_all(list);
        }

        if (curlResponse != CURLE_OK) {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            pop(vm);

            // char *errorString = (char *) curl_easy_strerror(curlResponse);
            return NULL_VAL; // TODO: Return map with error.
        }

        return OBJ_VAL(makeResponse(vm, curl, response, true));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    pop(vm);

    // char *errorString = (char *) curl_easy_strerror(CURLE_FAILED_INIT);
    return NULL_VAL; // TODO: Return map with error.
}

static Value httpOptions(VM *vm, const int argc, Value *args) {
    if (argc < 1 || argc > 3) {
        runtimeError(vm, "Function options() expected 1 to 3 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function options() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    char *url = AS_CSTRING(args[0]);
    ObjMap *headers = NULL;
    int timeout = DEFAULT_REQUEST_TIMEOUT;

    if (argc >= 2) {
        if (!IS_MAP(args[1])) {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function options() expected type 'map' for the second argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        headers = AS_MAP(args[1]);
    }

    if (argc == 3) {
        if (!IS_NUMBER(args[2])) {
            char *type = valueType(args[2]);
            runtimeError(vm, "Function options() expected type 'number' for the third argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        timeout = AS_NUMBER(args[3]);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();

    if (curl) {
        Response response;
        createResponse(vm, &response);
        struct curl_slist *list = NULL;

        if (headers) {
            if (!setRequestHeaders(vm, list, curl, headers)) {
                curl_slist_free_all(list);
                return NULL_VAL;
            }
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
        curl_easy_setopt(curl, CURLOPT_REQUEST_TARGET, "*");
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writeHeaders);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        const CURLcode curlResponse = curl_easy_perform(curl);

        if (headers) {
            curl_slist_free_all(list);
        }

        if (curlResponse != CURLE_OK) {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            pop(vm);

            // char *errorString = (char *) curl_easy_strerror(curlResponse);
            return NULL_VAL; // TODO: Return map with error.
        }

        return OBJ_VAL(makeResponse(vm, curl, response, true));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    pop(vm);

    // char *errorString = (char *) curl_easy_strerror(CURLE_FAILED_INIT);
    return NULL_VAL; // TODO: Return map with error.
}

Value useHttpLib(VM *vm) {
    ObjString *name = copyString(vm, "http", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNativeValue(vm, "MethodGet", OBJ_VAL(copyString(vm, HTTP_METHOD_GET, strlen(HTTP_METHOD_GET))), &lib->values);
    defineNativeValue(vm, "MethodHead", OBJ_VAL(copyString(vm, HTTP_METHOD_HEAD, strlen(HTTP_METHOD_HEAD))), &lib->values);
    defineNativeValue(vm, "MethodPost", OBJ_VAL(copyString(vm, HTTP_METHOD_POST, strlen(HTTP_METHOD_POST))), &lib->values);
    defineNativeValue(vm, "MethodPut", OBJ_VAL(copyString(vm, HTTP_METHOD_PUT, strlen(HTTP_METHOD_PUT))), &lib->values);
    defineNativeValue(vm, "MethodPatch", OBJ_VAL(copyString(vm, HTTP_METHOD_PATCH, strlen(HTTP_METHOD_PATCH))), &lib->values);
    defineNativeValue(vm, "MethodDelete", OBJ_VAL(copyString(vm, HTTP_METHOD_DELETE, strlen(HTTP_METHOD_DELETE))), &lib->values);
    defineNativeValue(vm, "MethodConnect", OBJ_VAL(copyString(vm, HTTP_METHOD_CONNECT, strlen(HTTP_METHOD_CONNECT))), &lib->values);
    defineNativeValue(vm, "MethodOptions", OBJ_VAL(copyString(vm, HTTP_METHOD_OPTIONS, strlen(HTTP_METHOD_OPTIONS))), &lib->values);
    defineNativeValue(vm, "MethodTrace", OBJ_VAL(copyString(vm, HTTP_METHOD_TRACE, strlen(HTTP_METHOD_TRACE))), &lib->values);

    defineNativeValue(vm, "StatusContinue", NUMBER_VAL(HTTP_STATUS_CODE_CONTINUE), &lib->values);
    defineNativeValue(vm, "StatusSwitchingProtocols", NUMBER_VAL(HTTP_STATUS_CODE_SWITCHING_PROTOCOLS), &lib->values);
    defineNativeValue(vm, "StatusProcessing", NUMBER_VAL(HTTP_STATUS_CODE_PROCESSING), &lib->values);
    defineNativeValue(vm, "StatusEarlyHints", NUMBER_VAL(HTTP_STATUS_CODE_EARLY_HINTS), &lib->values);

    defineNativeValue(vm, "StatusOk", NUMBER_VAL(HTTP_STATUS_CODE_OK), &lib->values);
    defineNativeValue(vm, "StatusCreated", NUMBER_VAL(HTTP_STATUS_CODE_CREATED), &lib->values);
    defineNativeValue(vm, "StatusAccepted", NUMBER_VAL(HTTP_STATUS_CODE_ACCEPTED), &lib->values);
    defineNativeValue(vm, "StatusNonAuthoritativeInfo", NUMBER_VAL(HTTP_STATUS_CODE_NON_AUTHORITATIVE_INFO), &lib->values);
    defineNativeValue(vm, "StatusNoContent", NUMBER_VAL(HTTP_STATUS_CODE_NO_CONTENT), &lib->values);
    defineNativeValue(vm, "StatusResetContent", NUMBER_VAL(HTTP_STATUS_CODE_RESET_CONTENT), &lib->values);
    defineNativeValue(vm, "StatusPartialContent", NUMBER_VAL(HTTP_STATUS_CODE_PARTIAL_CONTENT), &lib->values);
    defineNativeValue(vm, "StatusMultiStatus", NUMBER_VAL(HTTP_STATUS_CODE_MULTI_STATUS), &lib->values);
    defineNativeValue(vm, "StatusAlreadyReported", NUMBER_VAL(HTTP_STATUS_CODE_ALREADY_REPORTED), &lib->values);
    defineNativeValue(vm, "StatusIMUsed", NUMBER_VAL(HTTP_STATUS_CODE_IM_USED), &lib->values);

    defineNativeValue(vm, "StatusMultipleChoices", NUMBER_VAL(HTTP_STATUS_CODE_MULTIPLE_CHOICES), &lib->values);
    defineNativeValue(vm, "StatusMovedPermanently", NUMBER_VAL(HTTP_STATUS_CODE_MOVED_PERMANENTLY), &lib->values);
    defineNativeValue(vm, "StatusFound", NUMBER_VAL(HTTP_STATUS_CODE_FOUND), &lib->values);
    defineNativeValue(vm, "StatusSeeOther", NUMBER_VAL(HTTP_STATUS_CODE_SEE_OTHER), &lib->values);
    defineNativeValue(vm, "StatusNotModified", NUMBER_VAL(HTTP_STATUS_CODE_NOT_MODIFIED), &lib->values);
    defineNativeValue(vm, "StatusUseProxy", NUMBER_VAL(HTTP_STATUS_CODE_USE_PROXY), &lib->values);
    defineNativeValue(vm, "StatusTemporaryRedirect", NUMBER_VAL(HTTP_STATUS_CODE_TEMPORARY_REDIRECT), &lib->values);
    defineNativeValue(vm, "StatusPermanentRedirect", NUMBER_VAL(HTTP_STATUS_CODE_PERMANENT_REDIRECT), &lib->values);

    defineNativeValue(vm, "StatusBadRequest", NUMBER_VAL(HTTP_STATUS_CODE_BAD_REQUEST), &lib->values);
    defineNativeValue(vm, "StatusUnauthorized", NUMBER_VAL(HTTP_STATUS_CODE_UNAUTHORIZED), &lib->values);
    defineNativeValue(vm, "StatusPaymentRequired", NUMBER_VAL(HTTP_STATUS_CODE_PAYMENT_REQUIRED), &lib->values);
    defineNativeValue(vm, "StatusForbidden", NUMBER_VAL(HTTP_STATUS_CODE_FORBIDDEN), &lib->values);
    defineNativeValue(vm, "StatusNotFound", NUMBER_VAL(HTTP_STATUS_CODE_NOT_FOUND), &lib->values);
    defineNativeValue(vm, "StatusMethodNotAllowed", NUMBER_VAL(HTTP_STATUS_CODE_METHOD_NOT_ALLOWED), &lib->values);
    defineNativeValue(vm, "StatusNotAcceptable", NUMBER_VAL(HTTP_STATUS_CODE_NOT_ACCEPTABLE), &lib->values);
    defineNativeValue(vm, "StatusProxyAuthRequired", NUMBER_VAL(HTTP_STATUS_CODE_PROXY_AUTH_REQUIRED), &lib->values);
    defineNativeValue(vm, "StatusRequestTimeout", NUMBER_VAL(HTTP_STATUS_CODE_REQUEST_TIMEOUT), &lib->values);
    defineNativeValue(vm, "StatusConflict", NUMBER_VAL(HTTP_STATUS_CODE_CONFLICT), &lib->values);
    defineNativeValue(vm, "StatusGone", NUMBER_VAL(HTTP_STATUS_CODE_GONE), &lib->values);
    defineNativeValue(vm, "StatusLengthRequired", NUMBER_VAL(HTTP_STATUS_CODE_LENGTH_REQUIRED), &lib->values);
    defineNativeValue(vm, "StatusPreconditionFailed", NUMBER_VAL(HTTP_STATUS_CODE_PRECONDITION_FAILED), &lib->values);
    defineNativeValue(vm, "StatusRequestEntityTooLarge", NUMBER_VAL(HTTP_STATUS_CODE_REQUEST_ENTITY_TOO_LARGE), &lib->values);
    defineNativeValue(vm, "StatusRequestUriTooLong", NUMBER_VAL(HTTP_STATUS_CODE_REQUEST_URI_TOO_LONG), &lib->values);
    defineNativeValue(vm, "StatusUnsupportedMediaType", NUMBER_VAL(HTTP_STATUS_CODE_UNSUPPORTED_MEDIA_TYPE), &lib->values);
    defineNativeValue(vm, "StatusRequestedRangeNotSatisfiable", NUMBER_VAL(HTTP_STATUS_CODE_REQUESTED_RANGE_NOT_SATISFIABLE), &lib->values);
    defineNativeValue(vm, "StatusExpectationFailed", NUMBER_VAL(HTTP_STATUS_CODE_EXPECTATION_FAILED), &lib->values);
    defineNativeValue(vm, "StatusTeapot", NUMBER_VAL(HTTP_STATUS_CODE_TEAPOT), &lib->values);
    defineNativeValue(vm, "StatusMisdirectedRequest", NUMBER_VAL(HTTP_STATUS_CODE_MISDIRECTED_REQUEST), &lib->values);
    defineNativeValue(vm, "StatusUnprocessableEntity", NUMBER_VAL(HTTP_STATUS_CODE_UNPROCESSABLE_ENTITY), &lib->values);
    defineNativeValue(vm, "StatusLocked", NUMBER_VAL(HTTP_STATUS_CODE_LOCKED), &lib->values);
    defineNativeValue(vm, "StatusFailedDependency", NUMBER_VAL(HTTP_STATUS_CODE_FAILED_DEPENDENCY), &lib->values);
    defineNativeValue(vm, "StatusTooEarly", NUMBER_VAL(HTTP_STATUS_CODE_TOO_EARLY), &lib->values);
    defineNativeValue(vm, "StatusUpgradeRequired", NUMBER_VAL(HTTP_STATUS_CODE_UPGRADE_REQUIRED), &lib->values);
    defineNativeValue(vm, "StatusPreconditionRequired", NUMBER_VAL(HTTP_STATUS_CODE_PRECONDITION_REQUIRED), &lib->values);
    defineNativeValue(vm, "StatusTooManyRequests", NUMBER_VAL(HTTP_STATUS_CODE_TOO_MANY_REQUESTS), &lib->values);
    defineNativeValue(vm, "StatusRequestHeaderFieldsTooLarge", NUMBER_VAL(HTTP_STATUS_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE), &lib->values);
    defineNativeValue(vm, "StatusUnavailableForLegalReasons", NUMBER_VAL(HTTP_STATUS_CODE_UNAVAILABLE_FOR_LEGAL_REASONS), &lib->values);

    defineNativeValue(vm, "StatusInternalServerError", NUMBER_VAL(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR), &lib->values);
    defineNativeValue(vm, "StatusNotImplemented", NUMBER_VAL(HTTP_STATUS_CODE_NOT_IMPLEMENTED), &lib->values);
    defineNativeValue(vm, "StatusBadGateway", NUMBER_VAL(HTTP_STATUS_CODE_BAD_GATEWAY), &lib->values);
    defineNativeValue(vm, "StatusServiceUnavailable", NUMBER_VAL(HTTP_STATUS_CODE_SERVICE_UNAVAILABLE), &lib->values);
    defineNativeValue(vm, "StatusGatewayTimeout", NUMBER_VAL(HTTP_STATUS_CODE_GATEWAY_TIMEOUT), &lib->values);
    defineNativeValue(vm, "StatusHttpVersionNotSupported", NUMBER_VAL(HTTP_STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED), &lib->values);
    defineNativeValue(vm, "StatusVariantAlsoNegotiates", NUMBER_VAL(HTTP_STATUS_CODE_VARIANT_ALSO_NEGOTIATES), &lib->values);
    defineNativeValue(vm, "StatusInsufficientStorage", NUMBER_VAL(HTTP_STATUS_CODE_INSUFFICIENT_STORAGE), &lib->values);
    defineNativeValue(vm, "StatusLoopDetected", NUMBER_VAL(HTTP_STATUS_CODE_LOOP_DETECTED), &lib->values);
    defineNativeValue(vm, "StatusNotExtended", NUMBER_VAL(HTTP_STATUS_CODE_NOT_EXTENDED), &lib->values);
    defineNativeValue(vm, "StatusNetworkAuthenticationRequired", NUMBER_VAL(HTTP_STATUS_CODE_NETWORK_AUTHENTICATION_REQUIRED), &lib->values);

    defineNativeValue(vm, "MessageContinue", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_CONTINUE, strlen(HTTP_STATUS_MESSAGE_CONTINUE))), &lib->values);
    defineNativeValue(vm, "MessageSwitchingProtocols", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_SWITCHING_PROTOCOLS, strlen(HTTP_STATUS_MESSAGE_SWITCHING_PROTOCOLS))), &lib->values);
    defineNativeValue(vm, "MessageProcessing", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_PROCESSING, strlen(HTTP_STATUS_MESSAGE_PROCESSING))), &lib->values);
    defineNativeValue(vm, "MessageEarlyHints", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_EARLY_HINTS, strlen(HTTP_STATUS_MESSAGE_EARLY_HINTS))), &lib->values);

    defineNativeValue(vm, "MessageOk", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_OK, strlen(HTTP_STATUS_MESSAGE_OK))), &lib->values);
    defineNativeValue(vm, "MessageCreated", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_CREATED, strlen(HTTP_STATUS_MESSAGE_CREATED))), &lib->values);
    defineNativeValue(vm, "MessageAccepted", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_ACCEPTED, strlen(HTTP_STATUS_MESSAGE_ACCEPTED))), &lib->values);
    defineNativeValue(vm, "MessageNonAuthoritativeInfo", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_NON_AUTHORITATIVE_INFO, strlen(HTTP_STATUS_MESSAGE_NON_AUTHORITATIVE_INFO))), &lib->values);
    defineNativeValue(vm, "MessageNoContent", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_NO_CONTENT, strlen(HTTP_STATUS_MESSAGE_NO_CONTENT))), &lib->values);
    defineNativeValue(vm, "MessageResetContent", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_RESET_CONTENT, strlen(HTTP_STATUS_MESSAGE_RESET_CONTENT))), &lib->values);
    defineNativeValue(vm, "MessagePartialContent", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_PARTIAL_CONTENT, strlen(HTTP_STATUS_MESSAGE_PARTIAL_CONTENT))), &lib->values);
    defineNativeValue(vm, "MessageMultiStatus", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_MULTI_STATUS, strlen(HTTP_STATUS_MESSAGE_MULTI_STATUS))), &lib->values);
    defineNativeValue(vm, "MessageAlreadyReported", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_ALREADY_REPORTED, strlen(HTTP_STATUS_MESSAGE_ALREADY_REPORTED))), &lib->values);
    defineNativeValue(vm, "MessageImUsed", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_IM_USED, strlen(HTTP_STATUS_MESSAGE_IM_USED))), &lib->values);

    defineNativeValue(vm, "MessageMultipleChoices", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_MULTIPLE_CHOICES, strlen(HTTP_STATUS_MESSAGE_MULTIPLE_CHOICES))), &lib->values);
    defineNativeValue(vm, "MessageMovedPermanently", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_MOVED_PERMANENTLY, strlen(HTTP_STATUS_MESSAGE_MOVED_PERMANENTLY))), &lib->values);
    defineNativeValue(vm, "MessageFound", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_FOUND, strlen(HTTP_STATUS_MESSAGE_FOUND))), &lib->values);
    defineNativeValue(vm, "MessageSeeOther", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_SEE_OTHER, strlen(HTTP_STATUS_MESSAGE_SEE_OTHER))), &lib->values);
    defineNativeValue(vm, "MessageNotModified", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_NOT_MODIFIED, strlen(HTTP_STATUS_MESSAGE_NOT_MODIFIED))), &lib->values);
    defineNativeValue(vm, "MessageUseProxy", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_USE_PROXY, strlen(HTTP_STATUS_MESSAGE_USE_PROXY))), &lib->values);
    defineNativeValue(vm, "MessageTemporaryRedirect", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_TEMPORARY_REDIRECT, strlen(HTTP_STATUS_MESSAGE_TEMPORARY_REDIRECT))), &lib->values);
    defineNativeValue(vm, "MessagePermanentRedirect", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_PERMANENT_REDIRECT, strlen(HTTP_STATUS_MESSAGE_PERMANENT_REDIRECT))), &lib->values);

    defineNativeValue(vm, "MessageBadRequest", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_BAD_REQUEST, strlen(HTTP_STATUS_MESSAGE_BAD_REQUEST))), &lib->values);
    defineNativeValue(vm, "MessageUnauthorized", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_UNAUTHORIZED, strlen(HTTP_STATUS_MESSAGE_UNAUTHORIZED))), &lib->values);
    defineNativeValue(vm, "MessagePaymentRequired", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_PAYMENT_REQUIRED, strlen(HTTP_STATUS_MESSAGE_PAYMENT_REQUIRED))), &lib->values);
    defineNativeValue(vm, "MessageForbidden", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_FORBIDDEN, strlen(HTTP_STATUS_MESSAGE_FORBIDDEN))), &lib->values);
    defineNativeValue(vm, "MessageNotFound", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_NOT_FOUND, strlen(HTTP_STATUS_MESSAGE_NOT_FOUND))), &lib->values);
    defineNativeValue(vm, "MessageMethodNotAllowed", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_METHOD_NOT_ALLOWED, strlen(HTTP_STATUS_MESSAGE_METHOD_NOT_ALLOWED))), &lib->values);
    defineNativeValue(vm, "MessageNotAcceptable", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_NOT_ACCEPTABLE, strlen(HTTP_STATUS_MESSAGE_NOT_ACCEPTABLE))), &lib->values);
    defineNativeValue(vm, "MessageProxyAuthRequired", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_PROXY_AUTH_REQUIRED, strlen(HTTP_STATUS_MESSAGE_PROXY_AUTH_REQUIRED))), &lib->values);
    defineNativeValue(vm, "MessageRequestTimeout", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_REQUEST_TIMEOUT, strlen(HTTP_STATUS_MESSAGE_REQUEST_TIMEOUT))), &lib->values);
    defineNativeValue(vm, "MessageConflict", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_CONFLICT, strlen(HTTP_STATUS_MESSAGE_CONFLICT))), &lib->values);
    defineNativeValue(vm, "MessageGone", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_GONE, strlen(HTTP_STATUS_MESSAGE_GONE))), &lib->values);
    defineNativeValue(vm, "MessageLengthRequired", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_LENGTH_REQUIRED, strlen(HTTP_STATUS_MESSAGE_LENGTH_REQUIRED))), &lib->values);
    defineNativeValue(vm, "MessagePreconditionFailed", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_PRECONDITION_FAILED, strlen(HTTP_STATUS_MESSAGE_PRECONDITION_FAILED))), &lib->values);
    defineNativeValue(vm, "MessageRequestEntityTooLarge", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_REQUEST_ENTITY_TOO_LARGE, strlen(HTTP_STATUS_MESSAGE_REQUEST_ENTITY_TOO_LARGE))), &lib->values);
    defineNativeValue(vm, "MessageRequestUriTooLong", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_REQUEST_URI_TOO_LONG, strlen(HTTP_STATUS_MESSAGE_REQUEST_URI_TOO_LONG))), &lib->values);
    defineNativeValue(vm, "MessageUnsupportedMediaType", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_UNSUPPORTED_MEDIA_TYPE, strlen(HTTP_STATUS_MESSAGE_UNSUPPORTED_MEDIA_TYPE))), &lib->values);
    defineNativeValue(vm, "MessageRequestedRangeNotSatisfiable", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_REQUESTED_RANGE_NOT_SATISFIABLE, strlen(HTTP_STATUS_MESSAGE_REQUESTED_RANGE_NOT_SATISFIABLE))), &lib->values);
    defineNativeValue(vm, "MessageExpectationFailed", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_EXPECTATION_FAILED, strlen(HTTP_STATUS_MESSAGE_EXPECTATION_FAILED))), &lib->values);
    defineNativeValue(vm, "MessageTeapot", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_TEAPOT, strlen(HTTP_STATUS_MESSAGE_TEAPOT))), &lib->values);
    defineNativeValue(vm, "MessageMisdirectedRequest", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_MISDIRECTED_REQUEST, strlen(HTTP_STATUS_MESSAGE_MISDIRECTED_REQUEST))), &lib->values);
    defineNativeValue(vm, "MessageUnprocessableEntity", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_UNPROCESSABLE_ENTITY, strlen(HTTP_STATUS_MESSAGE_UNPROCESSABLE_ENTITY))), &lib->values);
    defineNativeValue(vm, "MessageLocked", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_LOCKED, strlen(HTTP_STATUS_MESSAGE_LOCKED))), &lib->values);
    defineNativeValue(vm, "MessageFailedDependency", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_FAILED_DEPENDENCY, strlen(HTTP_STATUS_MESSAGE_FAILED_DEPENDENCY))), &lib->values);
    defineNativeValue(vm, "MessageTooEarly", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_TOO_EARLY, strlen(HTTP_STATUS_MESSAGE_TOO_EARLY))), &lib->values);
    defineNativeValue(vm, "MessageUpgradeRequired", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_UPGRADE_REQUIRED, strlen(HTTP_STATUS_MESSAGE_UPGRADE_REQUIRED))), &lib->values);
    defineNativeValue(vm, "MessagePreconditionRequired", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_PRECONDITION_REQUIRED, strlen(HTTP_STATUS_MESSAGE_PRECONDITION_REQUIRED))), &lib->values);
    defineNativeValue(vm, "MessageTooManyRequests", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_TOO_MANY_REQUESTS, strlen(HTTP_STATUS_MESSAGE_TOO_MANY_REQUESTS))), &lib->values);
    defineNativeValue(vm, "MessageRequestHeaderFieldsTooLarge", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_REQUEST_HEADER_FIELDS_TOO_LARGE, strlen(HTTP_STATUS_MESSAGE_REQUEST_HEADER_FIELDS_TOO_LARGE))), &lib->values);
    defineNativeValue(vm, "MessageUnavailableForLegalReasons", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_UNAVAILABLE_FOR_LEGAL_REASONS, strlen(HTTP_STATUS_MESSAGE_UNAVAILABLE_FOR_LEGAL_REASONS))), &lib->values);

    defineNativeValue(vm, "MessageInternalServerError", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_INTERNAL_SERVER_ERROR, strlen(HTTP_STATUS_MESSAGE_INTERNAL_SERVER_ERROR))), &lib->values);
    defineNativeValue(vm, "MessageNotImplemented", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_NOT_IMPLEMENTED, strlen(HTTP_STATUS_MESSAGE_NOT_IMPLEMENTED))), &lib->values);
    defineNativeValue(vm, "MessageBadGateway", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_BAD_GATEWAY, strlen(HTTP_STATUS_MESSAGE_BAD_GATEWAY))), &lib->values);
    defineNativeValue(vm, "MessageServiceUnavailable", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_SERVICE_UNAVAILABLE, strlen(HTTP_STATUS_MESSAGE_SERVICE_UNAVAILABLE))), &lib->values);
    defineNativeValue(vm, "MessageGatewayTimeout", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_GATEWAY_TIMEOUT, strlen(HTTP_STATUS_MESSAGE_GATEWAY_TIMEOUT))), &lib->values);
    defineNativeValue(vm, "MessageHttpVersionNotSupported", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_HTTP_VERSION_NOT_SUPPORTED, strlen(HTTP_STATUS_MESSAGE_HTTP_VERSION_NOT_SUPPORTED))), &lib->values);
    defineNativeValue(vm, "MessageVariantAlsoNegotiates", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_VARIANT_ALSO_NEGOTIATES, strlen(HTTP_STATUS_MESSAGE_VARIANT_ALSO_NEGOTIATES))), &lib->values);
    defineNativeValue(vm, "MessageInsufficientStorage", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_INSUFFICIENT_STORAGE, strlen(HTTP_STATUS_MESSAGE_INSUFFICIENT_STORAGE))), &lib->values);
    defineNativeValue(vm, "MessageLoopDetected", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_LOOP_DETECTED, strlen(HTTP_STATUS_MESSAGE_LOOP_DETECTED))), &lib->values);
    defineNativeValue(vm, "MessageNotExtended", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_NOT_EXTENDED, strlen(HTTP_STATUS_MESSAGE_NOT_EXTENDED))), &lib->values);
    defineNativeValue(vm, "MessageNetworkAuthenticationRequired", OBJ_VAL(copyString(vm, HTTP_STATUS_MESSAGE_NETWORK_AUTHENTICATION_REQUIRED, strlen(HTTP_STATUS_MESSAGE_NETWORK_AUTHENTICATION_REQUIRED))), &lib->values);

    defineNative(vm, "get", httpGet, &lib->values);
    defineNative(vm, "post", httpPost, &lib->values);
    defineNative(vm, "put", httpPut, &lib->values);
    defineNative(vm, "head", httpHead, &lib->values);
    defineNative(vm, "options", httpOptions, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}