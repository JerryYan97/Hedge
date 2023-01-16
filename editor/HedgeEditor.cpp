#include "HedgeEditor.h"
#include "util/Utils.h"
#include <iostream>

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

void main(int argc, char** argv)
{
    HedgeEditor editor;
    editor.Run();
    UtilPrint();
}
