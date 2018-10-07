package com.osic.compiler;

import com.osic.compiler.asmBuilder.buildX86_64;
import com.osic.compiler.compile.CodeGenerator;
import com.osic.compiler.compile.parser.Parser;
import com.osic.compiler.compile.tokenizer.Tokenizer;
import com.osic.compiler.sourceMapper.sourceDTO.SourceDTO;
import com.osic.compiler.arguments.Arguments;
import com.osic.compiler.sourceSerializer.SourceBuilder;
import com.osic.compiler.sourceMapper.SourceMapper;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public final class OSICCompiler
{
    public static void main(String[] args) throws Exception {
        Arguments arguments = new Arguments(args);

        Path inputPath = Paths.get(arguments.getArgument("input"));
        Path outputPath = Paths.get(arguments.getArgument("output"));

        //scan given Input file for sourcecode and return as pojo
        SourceMapper sourceMapper = new SourceMapper();
        SourceDTO sourceMap = sourceMapper.mapClass(inputPath.toFile());

        //build imperative commands from DTO to make it compatible for NASM
        //NASM and OOP are not really compatible - so we go this way
        SourceBuilder sourceBuilder = new SourceBuilder(sourceMap);
        String source= sourceBuilder.fetchImperativeSource();

        //now we can translate the imperative Commands to something flat for ASM
        try (Tokenizer tokenizer = new Tokenizer(source)) {
            try (Parser parser = new Parser(tokenizer)) {
                try (CodeGenerator generator = new buildX86_64(Files.newBufferedWriter(outputPath, StandardCharsets.UTF_8))) {
                    generator.generate(parser.parse().compile());
                }
            }
        }
    }
}
