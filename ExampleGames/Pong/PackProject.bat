@echo off
REM Do necessary steps to prepare and copy files to executable folder so that we can debug the game.
REM The first argument is the game binary directory
set gameBinDir=%1

REM Get the current directory of the batch file
set currentDir=%~dp0

echo Game binary directory is: %gameBinDir%
echo Delete the old assets/scene folders and copy the new ones...

REM Check if the game assets/scene/config folders exist. We will delete the old ones and copy the new ones.

REM Check and delete the assets/scene folder
if exist %gameBinDir%/assets (
    echo Assets folder found
    rmdir /s /q "%gameBinDir%/assets"
)

if exist %gameBinDir%/scene (
    echo Scene folder found
    rmdir /s /q "%gameBinDir%/scene"
)

@REM if exist "%gameBinDir%/GameConfig.yml" (
@REM     echo Config file found
@REM     del "%gameBinDir%/GameConfig.yml"
@REM )

if not exist "%gameBinDir%" (
    echo Game binary directory not found. Create the directory.
    mkdir /p "%gameBinDir%"
)

echo Copy the new assets/scene folders...

REM Copy the new assets/scene folders
xcopy "%currentDir%assets" "%gameBinDir%/assets" /s /e /i
xcopy "%currentDir%scene" "%gameBinDir%/scene" /s /e /i
copy "%currentDir%GameConfig.yml" "%gameBinDir%/GameConfig.yml" /y