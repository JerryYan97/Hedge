#pragma once
#include "HObject.h"

namespace Hedge
{
    class HActorComponent : public HObject 
    {
    public:
        HActorComponent();
        ~HActorComponent();


    };

    class HSceneComponent : public HActorComponent
    {
    public:
        HSceneComponent();
        ~HSceneComponent();
    };

    class HPrimitiveComponent : public HSceneComponent
    {
    public:
        HPrimitiveComponent();
        ~HPrimitiveComponent();
    };
}
