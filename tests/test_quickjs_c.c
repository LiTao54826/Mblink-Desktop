#include <stdio.h>
#include "quickjs.h"

int main() {
    printf("Creating QuickJS runtime...\n");
    fflush(stdout);
    
    JSRuntime *rt = JS_NewRuntime();
    if (!rt) {
        fprintf(stderr, "Failed to create runtime\n");
        return 1;
    }
    
    printf("Runtime created!\n");
    fflush(stdout);
    
    JSContext *ctx = JS_NewContext(rt);
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        JS_FreeRuntime(rt);
        return 1;
    }
    
    printf("Context created!\n");
    fflush(stdout);
    
    // Eval simple expression
    JSValue result = JS_Eval(ctx, "1 + 1", 5, "<input>", JS_EVAL_TYPE_GLOBAL);
    
    if (JS_IsException(result)) {
        fprintf(stderr, "Exception occurred\n");
    } else {
        int32_t val;
        JS_ToInt32(ctx, &val, result);
        printf("Result: %d\n", val);
    }
    
    JS_FreeValue(ctx, result);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    
    printf("Test passed!\n");
    return 0;
}

