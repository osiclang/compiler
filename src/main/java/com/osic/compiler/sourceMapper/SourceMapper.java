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
import org.eclipse.jdt.core.dom.AST;
import org.eclipse.jdt.core.dom.ASTParser;
import org.eclipse.jdt.core.dom.ASTVisitor;
import org.eclipse.jdt.core.dom.Block;
import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.Expression;
import org.eclipse.jdt.core.dom.IMethodBinding;
import org.eclipse.jdt.core.dom.ITypeBinding;
import org.eclipse.jdt.core.dom.MethodDeclaration;
import org.eclipse.jdt.core.dom.MethodInvocation;

public class SourceMapper
{
    PackageDTO packageDTO = new PackageDTO();
    SourceDTO sourceDTO = new SourceDTO();
    ClassDTO classDTO = new ClassDTO();

    public SourceDTO mapClass(File file) throws IOException
    {
        parse(readFile(file.getAbsolutePath()), file.getName());
        sourceDTO.addPackageDTO(packageDTO);

        return sourceDTO;
    }

    private void parse(String str, String className)
    {
        classDTO.setClassName(className);
        ASTParser parser = ASTParser.newParser(AST.JLS4);
        parser.setSource(str.toCharArray());
        parser.setKind(ASTParser.K_COMPILATION_UNIT);
        parser.setResolveBindings(true);

        final CompilationUnit cu = (CompilationUnit) parser.createAST(null);

        cu.accept(new ASTVisitor()
        {

            public boolean visit(CompilationUnit node) {
                packageDTO.setPackageName(node.getPackage().getName().getFullyQualifiedName());

                node.imports().forEach(importPackage -> {
                    packageDTO.addImports(importPackage.toString());
                });

                return true;
            }

            public boolean visit(MethodDeclaration node) {
                MethodDTO methodDTO = new MethodDTO();
                methodDTO.setMethodName(node.getName().toString());

                List<CommandDTO> commands = new ArrayList<>();

                node.getBody().statements().forEach(statement -> {
                    CommandDTO commandDTO = new CommandDTO();
                    commandDTO.setCommand(statement.toString());
                    commands.add(commandDTO);
                });

                methodDTO.setCommands(commands);

                    Block block = node.getBody();
                    block.accept(new ASTVisitor() {
                        public boolean visit(MethodInvocation node) {
                            Expression expression = node.getExpression();
                            if (expression != null) {
                                System.out.println("Expr: " + expression.toString());
                                ITypeBinding typeBinding = expression.resolveTypeBinding();
                                if (typeBinding != null) {
                                    System.out.println("Type: " + typeBinding.getName());
                                }
                            }
                            IMethodBinding binding = node.resolveMethodBinding();
                            if (binding != null) {
                                ITypeBinding type = binding.getDeclaringClass();
                                if (type != null) {
                                    System.out.println("Decl: " + type.getName());
                                }
                            }

                            return true;
                        }
                    });
                    classDTO.addMethod(methodDTO);
                return true;
            }
        });
        packageDTO.addClass(classDTO);

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
