#include <cassert>
#include <iostream>
#include "Ecs.hpp"

namespace sg
{
    namespace ecs
    {
        namespace test
        {
            //-------------------------------------------------
            // Define components && component list
            //-------------------------------------------------

            struct HealthComponent
            {
                float health{ 0 };
            };

            struct CircleComponent
            {
                float radius{ 0 };
            };

            struct InputComponent
            {
                float key{ 0 };
            };

            using MyComponentsList = ComponentList<HealthComponent, CircleComponent, InputComponent>;

            //-------------------------------------------------
            // Define signatures && signature list
            //-------------------------------------------------

            using SignatureVelocity = Signature<InputComponent, CircleComponent>;
            using SignatureLife = Signature<HealthComponent>;

            using MySignaturesList = SignatureList<SignatureVelocity, SignatureLife>;

            //-------------------------------------------------
            // Alias `Settings` with above two compile-time lists
            //-------------------------------------------------

            using MySettings = Settings<MyComponentsList, MySignaturesList>;

            //-------------------------------------------------
            // Alias `Manager` with above compile-time `Settings`
            //-------------------------------------------------

            using MyManager = Manager<MySettings>;

            //-------------------------------------------------
            // Run compile-time tests
            //-------------------------------------------------

            static_assert(MySettings::ComponentCount() == 3);
            static_assert(MySettings::SignatureCount() == 2);

            static_assert(MySettings::GetComponentId<HealthComponent>() == 0);
            static_assert(MySettings::GetComponentId<CircleComponent>() == 1);
            static_assert(MySettings::GetComponentId<InputComponent>() == 2);

            static_assert(MySettings::GetSignatureId<SignatureVelocity>() == 0);
            static_assert(MySettings::GetSignatureId<SignatureLife>() == 1);

            //-------------------------------------------------
            // Runtime tests
            //-------------------------------------------------

            void RuntimeTests()
            {
                MyManager manager;

                // init state
                std::cout << "After manager instantiated" << std::endl;
                manager.PrintState(std::cout);

                assert(manager.GetEntityCount() == 0);

                // create entity
                const auto i0 = manager.CreateIndex();

                std::cout << "After the entity with index 0 is created.\n";
                manager.PrintState(std::cout);

                // add component
                auto& healthComponent{ manager.AddComponent<HealthComponent>(i0) };
                assert(healthComponent.health == 0);
                healthComponent.health = static_cast<float>(80);

                // check `has` and `delete`
                assert(manager.HasComponent<HealthComponent>(i0));
                assert(!manager.HasComponent<InputComponent>(i0));

                manager.DeleteComponent<HealthComponent>(i0);
                assert(!manager.HasComponent<HealthComponent>(i0));

                // `GetEntityCount()` should still be 0 because `Refresh()` has not been called yet.
                assert(manager.GetEntityCount() == 0);

                // refresh
                manager.Refresh();

                assert(manager.GetEntityCount() == 1);

                std::cout << "After refresh\n";
                manager.PrintState(std::cout);

                // clear
                manager.Clear();

                std::cout << "After clear\n";
                manager.PrintState(std::cout);

                assert(manager.GetEntityCount() == 0);
            }
        }
    }
}

int main()
{
    sg::ecs::test::RuntimeTests();
    std::cout << "Tests passed!" << std::endl;

    return 0;
}
