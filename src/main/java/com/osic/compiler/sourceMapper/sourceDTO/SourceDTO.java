package com.osic.compiler.sourceMapper.sourceDTO;

import java.util.ArrayList;
import java.util.List;

public class SourceDTO
{
    private List<PackageDTO> packageDTOS = new ArrayList<>();

    public List<PackageDTO> getPackageDTOS()
    {
        return packageDTOS;
    }

    public void setPackageDTOS(List<PackageDTO> packageDTOS)
    {
        this.packageDTOS = packageDTOS;
    }

    public void addPackageDTO(PackageDTO packageDTO)
    {
        this.packageDTOS.add(packageDTO);
    }
}
