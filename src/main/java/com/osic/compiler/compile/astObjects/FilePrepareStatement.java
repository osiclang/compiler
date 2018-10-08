package com.osic.compiler.compile.astObjects;

import com.osic.compiler.compile.stackir.Instruction;
import com.osic.compiler.compile.stackir.InstructionSequence;
import com.osic.compiler.compile.stackir.Opcode;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class FilePrepareStatement extends Statement
{

    private final List<StringExpression> values;

    public FilePrepareStatement(StringExpression... values)
    {
        this.values = Collections.unmodifiableList(Arrays.asList(values));
    }

    public FilePrepareStatement(List<StringExpression> values)
    {
        this.values = Collections.unmodifiableList(new ArrayList<>(values));
    }

    public List<StringExpression> getValues()
    {
        return values;
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

        FilePrepareStatement that = (FilePrepareStatement) o;

        if (!values.equals(that.values)) {
            return false;
        }

        return true;
    }

    @Override
    public int hashCode()
    {
        return values.hashCode();
    }

    @Override
    public String toString()
    {
        StringBuilder buf = new StringBuilder("FPREP ");
        for (int i = 0; i < values.size(); i++) {
            buf.append(values.get(i));
            if (i != (values.size() - 1)) {
                buf.append(", ");
            }
        }
        return buf.toString();
    }

    @Override
    public void compile(InstructionSequence seq)
    {
        for (StringExpression value : values) {
            value.compile(seq);
            seq.append(new Instruction(Opcode.FPREP));
        }
    }

}
