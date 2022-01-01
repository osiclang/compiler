#!/bin/bash

echo -e "OSIC Language build tool-set\n\n"
echo -e "all = Build for all below"
echo -e "osx = OSX (arm and intel builds)"
echo -e "linux = Linux binary build"
echo -e "bsd = FreeBSD binary build"
echo -e "win64 = Windows 64-Bit\n\n"

echo -e "Set arch for build: "
read dist

if [[ $dist == 'win64' || $dist == 'all' ]]; then
echo -e "Build binary for Windows 64"
GOOS=windows GOARCH=amd64 go build -o ../bin/v22/windows/occ.exe ../cmd/main.go
fi
if [[ $dist == 'osx' || $dist == 'all' ]]; then
echo -e "Build binary for OSX"
GOOS=darwin GOARCH=amd64 go build -o ../bin/v22/osx-amd/occ ../cmd/main.go
GOOS=darwin GOARCH=arm64 go build -o ../bin/v22/osx-arm/occ ../cmd/main.go
fi
if [[ $dist == 'linux' || $dist == 'all' ]]; then
echo -e "Build binary for linux"
GOOS=linux GOARCH=amd64 go build -o ../bin/v22/linux/occ ../cmd/main.go
fi
if [[ $dist == 'bsd'  || $dist == 'all' ]]; then
echo -e "Build binary for FreeBSD"
GOOS=freebsd GOARCH=amd64 go build -o ../bin/v22/freebsd/occ ../cmd/main.go
fi