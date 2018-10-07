package com.osic.compiler.sourceSerializer;

import com.osic.compiler.sourceMapper.sourceDTO.CommandDTO;
import com.osic.compiler.sourceMapper.sourceDTO.MethodDTO;
import com.osic.compiler.sourceMapper.sourceDTO.SourceDTO;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.TreeMap;

public class SourceBuilder
{
    private SourceDTO source;

    private HashMap<Integer, String> commands = new HashMap<>();

    public SourceBuilder(SourceDTO source)
    {
        this.source = source;
    }

    public String fetchImperativeSource()
    {
        source.getPackageDTOS().forEach(packageDTO -> packageDTO.getClasses().forEach(classDTO -> classDTO.getMethods().forEach(methodDTO -> {
            if (getMethodName(methodDTO.getMethodName()).equals("main")) {
                addMethods(methodDTO, 10);
            }
        })));

        StringBuilder sourceBuilder = new StringBuilder();
        Map<Integer, String> sortedMap = new TreeMap<>(commands);

        for (Map.Entry<Integer, String> entry : sortedMap.entrySet()) {
            sourceBuilder.append(entry.getKey()).append(" ").append(entry.getValue());
        }

        return sourceBuilder.toString();
    }

    private Boolean hasMethod(String methodName)
    {
        List<String> methods = new ArrayList<>();

        source.getPackageDTOS().forEach(
            packageDTO -> packageDTO.getClasses().forEach(
                classDTO -> {
                    classDTO.getMethods().forEach(
                        methodDTO -> methods.add(methodDTO.getMethodName())
                    );
                }
            )
        );

        return methods.contains(getMethodName(methodName));
    }

    private Integer getMethodLineCode(String methodName)
    {
        List<Integer> methodsLines = new ArrayList<>();

        source.getPackageDTOS().forEach(
            packageDTO -> packageDTO.getClasses().forEach(
                classDTO -> {
                    classDTO.getMethods().forEach(
                        methodDTO -> {
                            if (methodDTO.getMethodName().equals(getMethodName(methodName))) {
                                methodsLines.add(methodDTO.getKnowSubLineNumber());
                            }
                        }
                    );
                }
            )
        );

        if (methodsLines.size() == 0) {
            return 0;
        }
        return methodsLines.get(0);
    }

    private void addMethods(MethodDTO methodDTO, Integer lineCounter)
    {

        for (CommandDTO commandDTO : methodDTO.getCommands()) {
            if (hasMethod(commandDTO.getCommand())) {
                if (getMethodLineCode(commandDTO.getCommand()) == 0) {
                    int subCounter = lineCounter + 500;
                    commands.put(lineCounter, "call " + (subCounter) + ";\n");
                    lineCounter++;
                    fetchMethodSources(commandDTO.getCommand(), subCounter);
                }
                else {
                    commands.put(lineCounter, "call " + getMethodLineCode(commandDTO.getCommand()) + ";\n");
                    lineCounter++;
                }
                continue;
            }
            commands.put(lineCounter, commandDTO.getCommand());
            lineCounter++;
        }

        if (methodDTO.getMethodName().equals("main")) {
            commands.put(lineCounter, "end;\n");
        } else {
            commands.put(lineCounter, "return;\n");
        }
    }

    private void fetchMethodSources(String methodName, Integer lineCounter)
    {
        List<MethodDTO> methods = new ArrayList<>();
        source.getPackageDTOS().forEach(
            packageDTO -> packageDTO.getClasses().forEach(
                classDTO -> {
                    methods.addAll(classDTO.getMethods());
                }
            )
        );

        methods.forEach(methodDTO -> {
            if (methodDTO.getMethodName().equals(getMethodName(methodName))) {
                if (methodDTO.getKnowSubLineNumber() == 0) {
                    methodDTO.setKnowSubLineNumber(lineCounter);
                    addMethods(methodDTO, lineCounter);
                }
            }
        });
    }

    private String getMethodName(String methodName)
    {
        try {
            return methodName.substring(0, methodName.indexOf("("));
        }
        catch (Exception e) {
            return methodName;
        }
    }

    public static String removeLastCharRegexOptional(String s) {
        return Optional.ofNullable(s)
            .map(str -> str.replaceAll(".$", ""))
            .orElse(s);
    }
}
