#pragma once

namespace Hedge
{
    class HScene;

    class HFrameListener
    {
    public:
        HFrameListener();
        ~HFrameListener();

        virtual void FrameStarted() = 0;
        virtual void FrameEnded()   = 0;
        virtual HScene& GetActiveScene() = 0;
    };
}
