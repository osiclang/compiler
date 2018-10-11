package com.osic.compiler.sourceMapper;

import com.osic.compiler.sourceMapper.sourceDTO.ClassDTO;
import com.osic.compiler.sourceMapper.sourceDTO.CommandDTO;
import com.osic.compiler.sourceMapper.sourceDTO.MethodDTO;
import com.osic.compiler.sourceMapper.sourceDTO.SourceDTO;
import com.osic.compiler.sourceMapper.sourceDTO.PackageDTO;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class SourceMapper
{
    private static final String CLASS_NAME_PATTERN = "\r?\nclass(.*?);";
    private static final String PACKAGE_NAME_PATTERN = "\r?\n?package(.*?);";
    private static final String METHOD_NAME_PATTERN = "\r?\n(.*?)\\(\\)\\{";
    private static final String METHOD_LINES_PATTERN = "(?<=\r?\n%s\\(\\)\\{)([^\r]*?)(?=})";

    private String filename;
    private PackageDTO packageDTO = new PackageDTO();
    private SourceDTO sourceDTO = new SourceDTO();
    private ClassDTO classDTO = new ClassDTO();

    public SourceDTO mapClass(File file) throws Exception
    {
        this.filename = file.getName();
        parse(readFile(file.getAbsolutePath()), file.getName());
        sourceDTO.addPackageDTO(packageDTO);

        return sourceDTO;
    }

    private void parse(String source, String className) throws Exception
    {
        fetchClassName(source);
        fetchPackageName(source);
        fetchMethods(source);
        packageDTO.addClass(classDTO);

    }

    private void fetchPackageName(String source) throws Exception
    {
        Pattern packageNamePattern = Pattern.compile(PACKAGE_NAME_PATTERN);
        Matcher packageNameMatcher = packageNamePattern.matcher(source);
        while (packageNameMatcher.find()){
            if(packageDTO.getPackageName() != null){
                throw new Exception(String.format("Syntax[%s] package is set more than once", filename));
            }
            packageDTO.setPackageName(packageNameMatcher.group(1).trim());
        }
    }

    private void fetchClassName(String source) throws Exception
    {
        Pattern classNamePattern = Pattern.compile(CLASS_NAME_PATTERN);
        Matcher classNameMatcher = classNamePattern.matcher(source);
        while (classNameMatcher.find()){
            if(classDTO.getClassName() != null){
                throw new Exception(String.format("Syntax[%s] class is set more than once", filename));
            }
            classDTO.setClassName(classNameMatcher.group(1).trim());
        }
    }

    private void fetchMethods(String source)
    {
        Pattern pattern = Pattern.compile(METHOD_NAME_PATTERN);
        Matcher matcher = pattern.matcher(source);

        List<MethodDTO> methodDTOList = new ArrayList<>();

        while (matcher.find()) {
            MethodDTO methodDTO = new MethodDTO();
            String methodName = matcher.group(1);
            methodDTO.setMethodName(methodName);
            List<CommandDTO> command = new ArrayList<>();

            Pattern methodPattern = Pattern.compile(String.format(METHOD_LINES_PATTERN, methodName));
            Matcher methodMatcher = methodPattern.matcher(source);

            while (methodMatcher.find()) {
                String[] methodLines = methodMatcher.group(1).split("\r?\n");
                for (String methodLine : methodLines) {
                    if (!methodLine.trim().equals("")) {
                        String prepMethodLine = methodLine.trim() + "\n";
                        CommandDTO commandDTO = new CommandDTO();
                        commandDTO.setCommand(prepMethodLine);
                        command.add(commandDTO);
                    }
                }
                methodDTO.setCommands(command);
            }
            methodDTOList.add(methodDTO);
        }
        classDTO.setMethods(methodDTOList);
    }


    private String readFile(String pathname) throws IOException
    {
        File file = new File(pathname);
        StringBuilder fileContents = new StringBuilder((int) file.length());
        Scanner scanner = new Scanner(file);
        String lineSeparator = System.getProperty("line.separator");

        try {
            while (scanner.hasNextLine()) {
                fileContents.append(scanner.nextLine() + lineSeparator);
            }
            return fileContents.toString();
        }
        finally {
            scanner.close();
        }
    }


}
