package com.nice;

import java.util.HashMap;
import java.util.Map;

public class Environment {
    private final Map<String, Object> _values = new HashMap<>();
    
    Object get(Token name) {
        if (_values.containsKey(name._lexeme)) {
            return _values.get(name._lexeme);
        }
        
        throw new RuntimeError(name, "Undefined variable '" + name._lexeme + "'.");
    }
    
    void assign(Token name, Object value) {
        if (_values.containsKey(name._lexeme)) {
            _values.put(name._lexeme, value);
            return;
        }
        
        throw new RuntimeError(name, "Undefined variable '" + name + "'.");
    }
    
    void define(String name, Object value) {
        _values.put(name, value);
    }
}
