import os
import argparse
import shutil
from pathlib import Path

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--tarDir", help="The target root directory that will hold hedge header.",
                                    dest='tarDir',
                                    default='.')
    args = parser.parse_args()

    pathNameList = []

    for (root, dirs, files) in os.walk("..\\engine"):
        if "..\\engine\\build" != root[0:15] and "..\\engine\\CMakeUtils" not in root[0:20]:
            for name in files:
                if name[-2:] == ".h":
                    pathNameList.append([root[9:], os.path.join(root, name), name])

    if os.path.exists(args.tarDir):
        headerPath = args.tarDir + "\\headers"
        if os.path.exists(headerPath):
            shutil.rmtree(headerPath)
        Path(headerPath).mkdir(parents=True, exist_ok=True)
        for copyFile in pathNameList:
            newPath = headerPath + copyFile[0]
            Path(newPath).mkdir(parents=True, exist_ok=True)
            shutil.copyfile(copyFile[1], newPath + "\\" + copyFile[2])
