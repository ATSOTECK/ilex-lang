package com.nice;

import java.util.List;

abstract class Stmt {
  interface Visitor<R> {
    R visitExpressionStmt(Expression stmt);
    R visitPrintStmt(Print stmt);
    R visitVarStmt(Var stmt);
  }
  static class Expression extends Stmt {
    Expression(Expr expression) {
      _expression = expression;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitExpressionStmt(this);
    }

    final Expr _expression;
  }
  static class Print extends Stmt {
    Print(Expr expression) {
      _expression = expression;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitPrintStmt(this);
    }

    final Expr _expression;
  }
  static class Var extends Stmt {
    Var(Token name, Expr initializer) {
      _name = name;
      _initializer = initializer;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitVarStmt(this);
    }

    final Token _name;
    final Expr _initializer;
  }

  abstract <R> R accept(Visitor<R> visitor);
}
