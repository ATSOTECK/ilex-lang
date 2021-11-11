package com.nice;

public class Interpreter implements Expr.Visitor<Object> {
    void interpret(Expr expr) {
        try {
            Object value = evaluate(expr);
            System.out.println(stringify(value));
        } catch (RuntimeError e) {
            Main.runtimeError(e);
        }
    }
    
    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr._value;
    }
    
    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluate(expr._expression);
    }
    
    @Override
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = evaluate(expr._right);
        
        switch (expr._operator._type) {
            case TK_NOT -> {return !isTruthy(right);}
            case TK_MINUS -> {
                checkNumberOperand(expr._operator, right);
                return -(double)right;
            }
        }
        
        return null;
    }
    
    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluate(expr._left);
        Object right = evaluate(expr._right);
        
        switch (expr._operator._type) {
            case TK_PLUS -> {
                if (left instanceof Double && right instanceof Double) {
                    return (double)left + (double)right;
                } else if (left instanceof String && right instanceof String) {
                    return (String)left + (String)right;
                }
                
                throw new RuntimeError(expr._operator, "Operands must be two numbers or two strings.");
            }
            case TK_MINUS -> {
                checkNumberOperands(expr._operator, left, right);
                return (double)left - (double)right;
            }
            case TK_SLASH -> {
                checkNumberOperands(expr._operator, left, right);
                return (double)left / (double)right;
            }
            case TK_STAR -> {
                checkNumberOperands(expr._operator, left, right);
                return (double)left * (double)right;
            }
            case TK_GREATER -> {
                checkNumberOperands(expr._operator, left, right);
                return (double)left > (double)right;
            }
            case TK_GEQ ->  {
                checkNumberOperands(expr._operator, left, right);
                return (double)left >= (double)right;
            }
            case TK_LESS ->  {
                checkNumberOperands(expr._operator, left, right);
                return (double)left < (double)right;
            }
            case TK_LEQ ->  {
                checkNumberOperands(expr._operator, left, right);
                return (double)left <= (double)right;
            }
            case TK_NOTEQ -> {return !isEqual(left, right);}
            case TK_EQ -> {return isEqual(left, right);}
        }
        return null;
    }
    
    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) {
            return;
        }
        
        throw new RuntimeError(operator, "Operand must be a number.");
    }
    
    private void checkNumberOperands(Token operator, Object left, Object right) {
        if (left instanceof Double && right instanceof Double) {
            return;
        }
        
        throw new RuntimeError(operator, "Operands must be a number.");
    }
    
    private boolean isTruthy(Object object) {
        if (object == null) {
            return false;
        }
        
        if (object instanceof Boolean) {
            return (boolean)object;
        }
        
        return true;
    }
    
    private boolean isEqual(Object a, Object b) {
        if (a == null &&  b == null) {
            return true;
        } else if (a == null) {
            return false;
        }
        
        return a.equals(b);
    }
    
    private String stringify(Object object) {
        if (object == null) {
            return "nil";
        }
        
        if (object instanceof Double) {
            String txt = object.toString();
            if (txt.endsWith(".0")) {
                txt = txt.substring(0, txt.length() - 2);
            }
            
            return txt;
        }
        
        return object.toString();
    }
    
    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }
}
