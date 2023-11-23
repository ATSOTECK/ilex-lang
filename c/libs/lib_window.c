//
// Created by Skyler on 11/22/23.
//

#include "lib_window.h"

#include "../memory.h"
#include "../vm.h"

#include <stdlib.h>

static void windowResizeCallback(GLFWwindow *window, int w, int h) {
    glViewport(0, 0, w, h);
}

static Value windowNewWindow(VM *vm, int argc, Value *args) {
    if (argc != 1 && argc != 3) {
        runtimeError(vm, "Function newWindow() expected 1 or 3 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function newWindow() expected type 'string' for first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    if (argc == 3) {
        if (!IS_NUMBER(args[1])) {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function newWindow() expected type 'number' for second argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        if (!IS_NUMBER(args[2])) {
            char *type = valueType(args[2]);
            runtimeError(vm, "Function newWindow() expected type 'number' for third argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }
    }

    if (vm->window != nullptr) {
        return OBJ_VAL(vm->window);
    }

    if (!glfwInit()) {
        runtimeError(vm, "Failed to initialize GLFW!");
        return ERROR_VAL;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef I_MAC
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int w, h;
    if (argc == 1) {
        w = WINDOW_WIDTH_DEFAULT;
        h = WINDOW_HEIGHT_DEFAULT;
    } else {
        w = AS_NUMBER(args[1]);
        h = AS_NUMBER(args[2]);
    }

    char *title = AS_CSTRING(args[0]);

    GLFWwindow* window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (window == nullptr) {
        runtimeError(vm, "Failed to create GLFW window!");
        glfwTerminate();
        return ERROR_VAL;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        runtimeError(vm, "Failed to initialize GLAD!");
        return ERROR_VAL;
    }

    glViewport(0, 0, w, h);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    ObjWindow *objWindow = ALLOCATE(vm, ObjWindow, 1);
    objWindow->window = window;

    vm->window = objWindow;

    return OBJ_VAL(objWindow);
}

static Value windowShouldClose(VM *vm, int argc, Value *args) {
    if (argc != 0) {
        runtimeError(vm, "Function windowShouldClose() expected 0 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (vm->window == nullptr) {
        runtimeError(vm, "Function windowShouldClose() called before the window was initialized.");
        return ERROR_VAL;
    }

    return BOOL_VAL(glfwWindowShouldClose(vm->window->window));
}

static Value windowClose(VM *vm, int argc, Value *args) {
    if (argc != 0) {
        runtimeError(vm, "Function windowClose() expected 0 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (vm->window == nullptr) {
        runtimeError(vm, "Function windowClose() called before the window was initialized.");
        return ERROR_VAL;
    }

    glfwTerminate();

    return ZERO_VAL;
}

static Value windowClear(VM *vm, int argc, Value *args) {
    if (argc != 0 && argc != 1) {
        runtimeError(vm, "Function windowClear() expected 0 or 1 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (vm->window == nullptr) {
        runtimeError(vm, "Function windowClear() called before the window was initialized.");
        return ERROR_VAL;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    return ZERO_VAL;
}

static Value windowDraw(VM *vm, int argc, Value *args) {
    if (argc != 0) {
        runtimeError(vm, "Function windowDraw() expected 0 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (vm->window == nullptr) {
        runtimeError(vm, "Function windowDraw() called before the window was initialized.");
        return ERROR_VAL;
    }

    glfwSwapBuffers(vm->window->window);
    glfwPollEvents();
    return ZERO_VAL;
}

Value useWindowLib(VM *vm) {
    ObjString *name = copyString(vm, "window", 6);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "newWindow", windowNewWindow, &lib->values);
    defineNative(vm, "windowShouldClose", windowShouldClose, &lib->values);
    defineNative(vm, "windowClose", windowClose, &lib->values);
    defineNative(vm, "windowClear", windowClear, &lib->values);
    defineNative(vm, "windowDraw", windowDraw, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
