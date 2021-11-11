package com.nice;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Main {
    public static final Interpreter _interpreter = new Interpreter();
    static boolean _hadError = false;
    static boolean _hadRuntimeError = false;
    
    public static void run(String source) {
        Lexer lexer = new Lexer(source);
        List<Token> tokens = lexer.lex();
        Parser parser = new Parser(tokens);
        Expr expr = parser.parse();
        
        for (Token token : tokens) {
            System.out.println(token);
        }
        
        if (_hadError) {
            return;
        }
        
        _interpreter.interpret(expr);
        
        //System.out.println(new AstPrinter().print(expr));
    }
    
    static void error(int line, String msg) {
        report(line, "", msg);
    }
    
    static void runtimeError(RuntimeError e) {
        System.err.println(e.getMessage() + "\n[line " + e._token._line + "]");
        _hadRuntimeError = true;
    }
    
    private static void report(int line, String where, String msg) {
        System.err.println("[line " + line + "] Error" + where + ": " + msg);
        _hadError = true;
    }
    
    static void error(Token token, String msg) {
        if (token._type == TokenType.TK_EOF) {
            report(token._line, " at end", msg);
        } else {
            report(token._line, " at '" + token._lexeme + "'", msg);
        }
    }
    
    public static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));
        
        if (_hadError) {
            System.exit(69);
        }
        
        if (_hadRuntimeError) {
            System.exit(69);
        }
    }
    
    public static void runPrompt() throws IOException {
        InputStreamReader input = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(input);
    
        for (;;) {
            System.out.print("> ");
            String line = reader.readLine();
            if (line == null) {
                break;
            }
            
            run(line);
            _hadError = false;
        }
    }
    
    public static void main(String[] args) throws IOException {
	   if (args.length > 1) {
           System.out.println("Usage: 69 [script]");
           System.exit(69);
       } else if (args.length == 1) {
           runFile(args[0]);
       } else {
           runPrompt();
       }
    }
}
