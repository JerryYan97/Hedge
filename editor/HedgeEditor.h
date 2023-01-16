namespace DearImGuiExt
{
    class CustomLayout;
};

class HedgeEditor
{
public:
    HedgeEditor();
    ~HedgeEditor();

    void BuildGame(const char* pPathFileName);

    void CreateGameProject(const char* pPath);

    void Run();

private:
    DearImGuiExt::CustomLayout* myLayout;
};
