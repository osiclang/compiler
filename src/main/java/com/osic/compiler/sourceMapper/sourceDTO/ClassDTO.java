package com.osic.compiler.sourceMapper.sourceDTO;

import java.util.ArrayList;
import java.util.List;

public class ClassDTO
{
    private String className;

    private List<ClassDTO> parentClasses = new ArrayList<>();

    private List<MethodDTO> methods = new ArrayList<>();

    public String getClassName()
    {
        return className;
    }

    public void setClassName(String className)
    {
        this.className = className;
    }

    public List<MethodDTO> getMethods()
    {
        return methods;
    }

    public void addMethod(MethodDTO methodDTO){
        this.methods.add(methodDTO);
    }

    public void setMethods(List<MethodDTO> methods)
    {
        this.methods = methods;
    }

    public List<ClassDTO> getParentClasses()
    {
        return parentClasses;
    }

    public void setParentClasses(List<ClassDTO> parentClasses)
    {
        this.parentClasses = parentClasses;
    }
}
