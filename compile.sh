#!/usr/bin/env bash
java -jar target/OSICCompiler-0.0.2-BETA-jar-with-dependencies.jar --input=$1 --output=${1/%.*/.asm}
nasm -felf64 ${1/%.*/.asm} -o ${1/%.*/.o}
gcc -fPIC -no-pie -lc ${1/%.*/.o} -o ${1/%.*/}