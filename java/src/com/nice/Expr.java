package com.nice;

import java.util.List;

abstract class Expr {
  interface Visitor<R> {
    R visitAssignExpr(Assign expr);
    R visitBinaryExpr(Binary expr);
    R visitGroupingExpr(Grouping expr);
    R visitLiteralExpr(Literal expr);
    R visitUnaryExpr(Unary expr);
    R visitVariableExpr(Variable expr);
  }
  static class Assign extends Expr {
    Assign(Token name, Expr value) {
      _name = name;
      _value = value;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitAssignExpr(this);
    }

    final Token _name;
    final Expr _value;
  }
  static class Binary extends Expr {
    Binary(Expr left, Token operator, Expr right) {
      _left = left;
      _operator = operator;
      _right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitBinaryExpr(this);
    }

    final Expr _left;
    final Token _operator;
    final Expr _right;
  }
  static class Grouping extends Expr {
    Grouping(Expr expression) {
      _expression = expression;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitGroupingExpr(this);
    }

    final Expr _expression;
  }
  static class Literal extends Expr {
    Literal(Object value) {
      _value = value;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitLiteralExpr(this);
    }

    final Object _value;
  }
  static class Unary extends Expr {
    Unary(Token operator, Expr right) {
      _operator = operator;
      _right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitUnaryExpr(this);
    }

    final Token _operator;
    final Expr _right;
  }
  static class Variable extends Expr {
    Variable(Token name) {
      _name = name;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitVariableExpr(this);
    }

    final Token _name;
  }

  abstract <R> R accept(Visitor<R> visitor);
}
