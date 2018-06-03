#include <iostream>
#include "Ecs.hpp"

struct HealthComponent
{
    float value{ 0 };
};

struct CircleComponent
{
    float value{ 0 };
};

struct InputComponent
{
    float value{ 0 };
};

using MyComponentsList = sg::ecs::ComponentList<HealthComponent, InputComponent, CircleComponent>;
using SignatureVelocity = sg::ecs::Signature<InputComponent, CircleComponent>;
using SignatureLife = sg::ecs::Signature<HealthComponent>;

using MySignaturesList = sg::ecs::SignatureList<SignatureVelocity, SignatureLife>;
using MySettings = sg::ecs::Settings<MyComponentsList, MySignaturesList>;

using MyManager = sg::ecs::Manager<MySettings>;

int main()
{
    std::cout << "ComponentCount: " << MySettings::ComponentCount() << std::endl;
    std::cout << "IsComponent InputComponent: " << MySettings::IsValidComponent<InputComponent>() << std::endl;
    std::cout << "GetComponentId CircleComponent: " << MySettings::GetComponentId<CircleComponent>() << std::endl;

    std::cout << "SignatureCount: " << MySettings::SignatureCount() << std::endl;
    std::cout << "IsSignature SignatureVelocity: " << MySettings::IsSignature<SignatureVelocity>() << std::endl;

    MyManager manager;

    for (auto index{ 0 }; index < 32; ++index)
    {
        auto entity{ manager.CreateIndex() };

        auto& health{ manager.AddComponent<HealthComponent>(entity).value };
        health = static_cast<float>(index);
    }

    return 0;
}
