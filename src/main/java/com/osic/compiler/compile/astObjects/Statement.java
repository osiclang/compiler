package com.osic.compiler.compile.astObjects;

import com.osic.compiler.compile.stackir.InstructionSequence;

public abstract class Statement {

    public abstract void compile(InstructionSequence seq);

}
