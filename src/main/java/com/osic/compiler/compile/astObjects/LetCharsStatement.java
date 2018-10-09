package com.osic.compiler.compile.astObjects;

import com.osic.compiler.compile.stackir.Instruction;
import com.osic.compiler.compile.stackir.InstructionSequence;
import com.osic.compiler.compile.stackir.Opcode;
import java.util.Objects;

public class LetCharsStatement extends Statement
{
    private final String name;
    private final Expression value;

    public LetCharsStatement(String name, Expression value)
    {
        this.name = name;
        this.value = value;
    }

    public String getName()
    {
        return name;
    }

    public Expression getValue()
    {
        return value;
    }

    @Override
    public boolean equals(Object o)
    {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        LetCharsStatement that = (LetCharsStatement) o;

        if (!name.equals(that.name)) {
            return false;
        }
        if (!value.equals(that.value)) {
            return false;
        }

        return true;
    }

    @Override
    public int hashCode()
    {
        return Objects.hash(name, value);
    }

    @Override
    public String toString()
    {
        return "LET " + name + " = " + value;
    }

    @Override
    public void compile(InstructionSequence seq)
    {
        value.compile(seq);
        seq.append(new Instruction(Opcode.CHARSSTORE, value.toString()));
    }

}
