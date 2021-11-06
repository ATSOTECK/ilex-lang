package com.nice;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Main {
    static boolean _hadError;
    
    public static void run(String source) {
        Lexer lexer = new Lexer(source);
        List<Token> tokens = lexer.lex();
        
        for (Token token : tokens) {
            System.out.println(token);
        }
    }
    
    static void error(int line, String msg) {
        report(line, "", msg);
    }
    
    private static void report(int line, String where, String msg) {
        System.err.println("[line " + line + "] Error" + where + ": " + msg);
        _hadError = true;
    }
    
    public static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));
        
        if (_hadError) {
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
