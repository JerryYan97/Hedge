#include "HedgeEditor.h"
#include "util/Utils.h"
#include <iostream>
#include <cstdlib>

HedgeEditor::HedgeEditor()
{
}

HedgeEditor::~HedgeEditor()
{
}

void HedgeEditor::Run()
{
    std::cout << "Hello World From the Editor" << std::endl;
}

void HedgeEditor::BuildGame(
    const char* pPathFileName)
{
    std::system("cd");
}

void main(
    int    argc, 
    char** argv)
{
    HedgeEditor editor;
    editor.Run();
    UtilPrint();
    editor.BuildGame(nullptr);
}
