#pragma once

namespace Hedge
{
    class HFrameListener
    {
    public:
        HFrameListener();
        ~HFrameListener();

        virtual void FrameStarted() = 0;
        virtual void FrameEnded()   = 0;
    };
}
