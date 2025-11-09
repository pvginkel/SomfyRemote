@echo off

setlocal enableextensions enabledelayedexpansion

pushd "%~dp0"

cd ..\..

docker build ^
    --tag somfy-remote-uploader ^
    . ^
    --file tools/dev-upload/Dockerfile 

echo "%~dp0"

docker run ^
    --rm ^
    -v %CD%/build/somfy-remote.bin:/workspace/app/somfy-remote-ota.bin ^
    -v %CD%/../HelmCharts/assets:/workspace/keys ^
    --add-host iotsupport.home:192.168.178.62 ^
    somfy-remote-uploader ^
    /workspace/keys/kubernetes-signing-key ^
    /workspace/app/somfy-remote-ota.bin ^
    iotsupport.home

popd
