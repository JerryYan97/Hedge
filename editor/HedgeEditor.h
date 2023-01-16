
namespace DearImGuiExt
{
    class CustomLayout;
};

class HedgeEditor
{
public:
    HedgeEditor();
    ~HedgeEditor();

    void Run();

private:
    DearImGuiExt::CustomLayout* myLayout;
};
