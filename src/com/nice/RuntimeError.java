package com.nice;

public class RuntimeError extends RuntimeException {
    final Token _token;
    
    RuntimeError(Token token, String msg) {
        super(msg);
        _token = token;
    }
}
