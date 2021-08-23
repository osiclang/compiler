cd ..

go test ./...

$Env:GOOS = "windows"; $Env:GOARCH = "amd64"; go build -o bin/windows/occ.exe cmd/main.go
$Env:GOOS = "darwin"; $Env:GOARCH = "amd64"; go build -o bin/osx-amd/occ cmd/main.go
$Env:GOOS = "darwin"; $Env:GOARCH = "arm64"; go build -o bin/osx-arm/occ cmd/main.go
$Env:GOOS = "linux"; $Env:GOARCH = "amd64"; go build -o bin/linux/occ cmd/main.go
$Env:GOOS = "freebsd"; $Env:GOARCH = "amd64"; go build -o bin/freebsd/occ cmd/main.go

