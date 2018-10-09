package com.osic.compiler.compile.parser;

import com.osic.compiler.compile.astObjects.FilePrepareStatement;
import com.osic.compiler.compile.astObjects.FileWriteStatement;
import com.osic.compiler.compile.astObjects.LetCharsStatement;
import com.osic.compiler.compile.tokenizer.Tokenizer;
import com.osic.compiler.compile.astObjects.BinaryExpression;
import com.osic.compiler.compile.astObjects.BinaryOperator;
import com.osic.compiler.compile.astObjects.BranchStatement;
import com.osic.compiler.compile.astObjects.BranchType;
import com.osic.compiler.compile.astObjects.EndStatement;
import com.osic.compiler.compile.astObjects.Expression;
import com.osic.compiler.compile.astObjects.IfStatement;
import com.osic.compiler.compile.astObjects.ImmediateExpression;
import com.osic.compiler.compile.astObjects.ImmediateString;
import com.osic.compiler.compile.astObjects.InputStatement;
import com.osic.compiler.compile.astObjects.LetStatement;
import com.osic.compiler.compile.astObjects.Line;
import com.osic.compiler.compile.astObjects.PrintStatement;
import com.osic.compiler.compile.astObjects.Program;
import com.osic.compiler.compile.astObjects.RelationalOperator;
import com.osic.compiler.compile.astObjects.ReturnStatement;
import com.osic.compiler.compile.astObjects.Statement;
import com.osic.compiler.compile.astObjects.StringExpression;
import com.osic.compiler.compile.astObjects.UnaryExpression;
import com.osic.compiler.compile.astObjects.UnaryOperator;
import com.osic.compiler.compile.astObjects.VariableExpression;
import com.osic.compiler.compile.tokenizer.Token;
import com.osic.compiler.compile.tokenizer.Token.Type;

import java.io.Closeable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public final class Parser implements Closeable
{

    private final Tokenizer tokenizer;
    private Token token;

    public Parser(Tokenizer tokenizer) throws IOException
    {
        this.tokenizer = tokenizer;
        this.token = tokenizer.nextToken();
    }

    private void consume() throws IOException
    {
        token = tokenizer.nextToken();

    }

    private boolean accept(Type type) throws IOException
    {
        if (token.getType() == type) {
            consume();
            return true;
        }

        return false;
    }

    private void expect(Type type) throws IOException
    {
        if (!accept(type)) {
            throw new IOException("Unexpected " + token.getType() + ", expecting " + type);
        }
    }

    public Program parse() throws IOException
    {
        List<Line> lines = new ArrayList<>();
        while (!accept(Type.EOF)) {
            lines.add(nextLine());
        }
        return new Program(lines);
    }

    Line nextLine() throws IOException
    {
        if (token.getType() != Type.NUMBER) {
            throw new IOException("Unexpected " + token.getType() + ", expecting NUMBER");
        }

        int lineNumber = Integer.parseInt(token.getValue().get());
        consume();

        Statement stmt = nextStatement();

        if (token.getType() != Type.COMMANDEND) {
            throw new IOException("Unexpected " + token.getType() + ", expectingCommandEnd");
        }

        consume();

        if (token.getType() != Type.LF && token.getType() != Type.EOF) {
            throw new IOException("Unexpected " + token.getType() + ", expecting LF or EOF");
        }

        consume();
        return new Line(lineNumber, stmt);
    }

    Statement nextStatement() throws IOException
    {
        if (token.getType() != Type.KEYWORD) {
            throw new IOException("Unexpected " + token.getType() + ", expecting KEYWORD");
        }

        String keyword = token.getValue().get();
        switch (keyword) {
            case "print":
                consume();

                List<StringExpression> values = new ArrayList<>();
                do {
                    if (token.getType() == Type.STRING) {
                        values.add(new ImmediateString(token.getValue().get()));
                        consume();
                    }
                    else {
                        values.add(nextExpression());
                    }
                } while (accept(Type.COMMA));

                return new PrintStatement(values);

            case "fopen":
                consume();

                List<StringExpression> filenameValues = new ArrayList<>();
                do {
                    if (token.getType() == Type.STRING) {
                        filenameValues.add(new ImmediateString(token.getValue().get()));
                        consume();
                    }
                    else {
                        throw new IOException("Unexpected " + token.getType() + ", expecting Filename as STRING");
                    }
                } while (accept(Type.COMMA) && accept(Type.LPAREN) && accept(Type.RPAREN));

                return new FilePrepareStatement(filenameValues);

            case "fwrite":
                consume();

                List<StringExpression> fileInputValue = new ArrayList<>();
                do {
                    if (token.getType() == Type.STRING) {
                        fileInputValue.add(new ImmediateString(token.getValue().get()));
                        consume();
                    }
                    else {
                        fileInputValue.add(nextExpression());
                    }
                } while (accept(Type.COMMA) && accept(Type.LPAREN) && accept(Type.RPAREN));

                return new FileWriteStatement(fileInputValue);

            case "if":
                consume();

                Expression left = nextExpression();

                RelationalOperator operator;
                switch (token.getType()) {
                    case EQ:
                        operator = RelationalOperator.EQ;
                        break;
                    case NE:
                        operator = RelationalOperator.NE;
                        break;
                    case LT:
                        operator = RelationalOperator.LT;
                        break;
                    case LTE:
                        operator = RelationalOperator.LTE;
                        break;
                    case GT:
                        operator = RelationalOperator.GT;
                        break;
                    case GTE:
                        operator = RelationalOperator.GTE;
                        break;
                    default:
                        throw new IOException("Unexpected " + token.getType() + ", expecting EQ, NE, LT, LTE, GT or GTE");
                }
                consume();

                Expression right = nextExpression();

                if (token.getType() != Type.KEYWORD) {
                    throw new IOException("Unexpected " + token.getType() + ", expecting KEYWORD");
                }

                String thenKeyword = token.getValue().get();
                if (!thenKeyword.equals("then")) {
                    throw new IOException("Unexpected keyword " + keyword + ", expecting THEN");
                }

                consume();

                Statement statement = nextStatement();
                return new IfStatement(operator, left, right, statement);

            case "call":
                consume();

                if (token.getType() != Type.NUMBER) {
                    throw new IOException("Unexpected " + token.getType() + ", expecting NUMBER");
                }

                int target = Integer.parseInt(token.getValue().get());
                consume();

                BranchType type = keyword.equals("goto") ? BranchType.GOTO : BranchType.GOSUB;
                return new BranchStatement(type, target);

            case "input":
                consume();

                List<String> names = new ArrayList<>();
                do {
                    if (token.getType() != Type.VAR) {
                        throw new IOException("Unexpected " + token.getType() + ", expecting VAR");
                    }

                    names.add(token.getValue().get());
                    consume();
                } while (accept(Type.COMMA));

                return new InputStatement(names);

            case "int":
                consume();

                if (token.getType() != Type.VAR) {
                    throw new IOException("Unexpected " + token.getType() + ", expecting VAR");
                }

                String integer = token.getValue().get();
                consume();

                expect(Type.EQ);

                return new LetStatement(integer, nextExpression());

            case "chars":
                consume();

                if (token.getType() != Type.VAR && token.getType() != Type.STRING) {
                    throw new IOException("Unexpected " + token.getType() + ", expecting VAR or STRING");
                }

                String string = token.getValue().get();
                consume();

                expect(Type.EQ);

                return new LetCharsStatement(string, nextExpression());

            case "return":
                consume();
                return new ReturnStatement();

            case "end":
                consume();
                return new EndStatement();

            default:
                throw new IOException("Unknown keyword: " + keyword);
        }
    }

    Expression nextExpression() throws IOException
    {
        Expression left;

        if (token.getType() == Type.PLUS || token.getType() == Type.MINUS) {
            UnaryOperator operator = token.getType() == Type.PLUS ? UnaryOperator.PLUS : UnaryOperator.MINUS;
            consume();

            left = new UnaryExpression(operator, nextTerm());
        }
        else {
            left = nextTerm();
        }

        while (token.getType() == Type.PLUS || token.getType() == Type.MINUS) {
            BinaryOperator operator = token.getType() == Type.PLUS ? BinaryOperator.PLUS : BinaryOperator.MINUS;
            consume();

            Expression right = nextTerm();
            left = new BinaryExpression(operator, left, right);
        }

        return left;
    }

    private Expression nextTerm() throws IOException
    {
        Expression left = nextFactor();

        while (token.getType() == Type.MULT || token.getType() == Type.DIV) {
            BinaryOperator operator = token.getType() == Type.MULT ? BinaryOperator.MULT : BinaryOperator.DIV;
            consume();

            left = new BinaryExpression(operator, left, nextFactor());
        }

        return left;
    }

    private Expression nextFactor() throws IOException
    {
        switch (token.getType()) {
            case VAR:
                Expression expr = new VariableExpression(token.getValue().get());
                consume();
                return expr;

            case STRING:
                Expression characterExpr = new VariableExpression(token.getValue().get());
                consume();
                return characterExpr;

            case NUMBER:
                expr = new ImmediateExpression(Integer.parseInt(token.getValue().get()));
                consume();
                return expr;

            case LPAREN:
                consume();
                expr = nextExpression();
                expect(Type.RPAREN);
                return expr;

            default:
                throw new IOException("Unexpected " + token.getType() + ", expecting VAR, NUMBER, CHAR or LPAREN");
        }
    }

    @Override
    public void close() throws IOException
    {
        tokenizer.close();
    }

}
