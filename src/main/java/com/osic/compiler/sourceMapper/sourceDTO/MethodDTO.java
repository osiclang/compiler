package com.osic.compiler.sourceMapper.sourceDTO;

import java.util.ArrayList;
import java.util.List;

public class MethodDTO
{
    private String methodName;

    private Integer knowSubLineNumber = 0;

    private List<ParameterDTO> paramenter = new ArrayList<>();

    private List<CommandDTO> commands = new ArrayList<>();

    public String getMethodName()
    {
        return methodName;
    }

    public void setMethodName(String methodName)
    {
        this.methodName = methodName;
    }

    public List<ParameterDTO> getParamenter()
    {
        return paramenter;
    }

    public void setParamenter(List<ParameterDTO> paramenter)
    {
        this.paramenter = paramenter;
    }

    public List<CommandDTO> getCommands()
    {
        return commands;
    }

    public void setCommands(List<CommandDTO> commands)
    {
        this.commands = commands;
    }

    public Integer getKnowSubLineNumber()
    {
        return knowSubLineNumber;
    }

    public MethodDTO setKnowSubLineNumber(Integer knowSubLineNumber)
    {
        this.knowSubLineNumber = knowSubLineNumber;
        return this;
    }
}
