#pragma once

#include "core/HFrameListener.h"
#include "core/HGpuRsrcManager.h"

namespace Hedge
{
    class MyGame : public HFrameListener
    {
    public:
        MyGame();
        virtual ~MyGame();
    };
}
