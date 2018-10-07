package com.osic.compiler.arguments;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

public class Arguments
{

    public static final String BARISTA_JAVA_TRANSPILER = "OSICCompiler";

    private CommandLine cmd = null;

    public Arguments(String[] args){
        Options options = new Options();

        Option input = new Option("i", "input", true, "input file path");
        input.setRequired(true);
        options.addOption(input);

        Option output = new Option("o", "output", true, "output file");
        output.setRequired(true);
        options.addOption(output);

        CommandLineParser cmdParser = new DefaultParser();
        HelpFormatter formatter = new HelpFormatter();

        try {
            cmd = cmdParser.parse(options, args);
        } catch (ParseException e) {
            System.out.println(e.getMessage());
            formatter.printHelp(BARISTA_JAVA_TRANSPILER, options);

            System.exit(1);
        }
    }

    public String getArgument(String argument) {
        return cmd.getOptionValue(argument);
    }
}
