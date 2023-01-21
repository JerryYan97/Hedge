#pragma once

namespace Hedge
{
    class HBaseGuiRenderer
    {
    public:
        HBaseGuiRenderer();
        ~HBaseGuiRenderer();

        virtual void Render() = 0;
    private:

    };
}