package com.osic.compiler.compile.astObjects;

import com.osic.compiler.compile.stackir.InstructionSequence;

public abstract class StringExpression {

    public abstract void compile(InstructionSequence seq);

}
