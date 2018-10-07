package com.osic.compiler.sourceMapper.sourceDTO;

import java.util.ArrayList;
import java.util.List;

public class PackageDTO
{
    private List<ClassDTO> classes = new ArrayList<>();

    private List<String> imports = new ArrayList<>();

    private String packageName;

    public List<ClassDTO> getClasses()
    {
        return classes;
    }

    public void setClasses(List<ClassDTO> classes)
    {
        this.classes = classes;
    }

    public void addClass(ClassDTO classDTO)
    {
        this.classes.add(classDTO);
    }

    public String getPackageName()
    {
        return packageName;
    }

    public void setPackageName(String packageName)
    {
        this.packageName = packageName;
    }

    public List<String> getImports()
    {
        return imports;
    }

    public void setImports(List<String> imports)
    {
        this.imports = imports;
    }

    public void addImports(String importPackage)
    {
        this.imports.add(importPackage);
    }
}
