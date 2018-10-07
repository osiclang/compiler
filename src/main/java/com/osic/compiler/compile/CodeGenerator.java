package com.osic.compiler.compile;

import com.osic.compiler.compile.stackir.InstructionSequence;

import java.io.Closeable;
import java.io.IOException;

public abstract class CodeGenerator implements Closeable {

    public abstract void generate(InstructionSequence seq) throws IOException;

}
