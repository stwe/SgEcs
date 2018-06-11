#pragma once

#include <cassert>
#include <boost/mpl/list.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/for_each.hpp>
#include <bitset>
#include <vector>
#include "Util.hpp"

namespace sg
{
    namespace ecs
    {
        //-------------------------------------------------
        // Constants
        //-------------------------------------------------

        static constexpr std::size_t DEFAULT_ENTITY_CAPACITY{ 100 };

        //-------------------------------------------------
        // Forward declaration
        //-------------------------------------------------

        template <typename TSettings>
        class SignatureBitsetsStorage;

        //-------------------------------------------------
        // Lists
        //-------------------------------------------------

        /*
         * ----------------
         * Example of usage
         * ----------------
         * using MyComponentsList = sg::ecs::ComponentList<HealthComponent, InputComponent, CircleComponent>;
         * using SignatureVelocity = sg::ecs::Signature<InputComponent, CircleComponent>;
         * using SignatureLife = sg::ecs::Signature<HealthComponent>;
         * using MySignaturesList = sg::ecs::SignatureList<SignatureVelocity, SignatureLife>;
         */

        /**
         * @brief List of all component types.
         * @tparam TComponentTypes Component types to list.
         */
        template <typename... TComponentTypes>
        using ComponentList = boost::mpl::list<TComponentTypes...>;

        /**
         * @brief A signature type.
         * @tparam TComponent Component types that describes the signature type.
         */
        template <typename... TComponent>
        using Signature = boost::mpl::list<TComponent...>;

        /**
         * @brief List of all signature types.
         * @tparam TSignatures Signature types to list.
         */
        template <typename... TSignatures>
        using SignatureList = boost::mpl::list<TSignatures...>;

        //-------------------------------------------------
        // Entity
        //-------------------------------------------------

        using DataIndex = std::size_t;
        using EntityIndex = std::size_t;

        /**
         * @brief Entity metadata.
         * @tparam TSettings The Ecs settings and wrapper for the `ComponentList` and `SignatureList`.
         */
        template <typename TSettings>
        struct Entity
        {
            using Settings = TSettings;

            /**
             * @brief Describes a `std::bitset` which size corresponds to the size of the `ComponentList`.
             */
            using Bitset = typename Settings::Bitset;

            DataIndex dataIndex{ 0 };
            Bitset bitset;
            bool alive{ false };
        };

        //-------------------------------------------------
        // ComponentStorage
        //-------------------------------------------------

        /**
         * @brief Creates a single `std::vector` for every component type and stored all
         *        vector types in a `std::tuple`.
         * @tparam TSettings The Ecs settings and wrapper for the `ComponentList` and `SignatureList`.
         */
        template <typename TSettings>
        class ComponentStorage
        {
        public:
            /**
             * @brief Grow every vector.
             * @param newCapacity
             */
            void GrowTo(std::size_t newCapacity)
            {
                boost::mpl::for_each<ComponentList>
                (
                    [&tupleOfComponentVectors = m_tupleOfComponentVectors, newCapacity](auto componentType)
                    {
                        std::get<std::vector<decltype(componentType)>>(tupleOfComponentVectors).resize(newCapacity);
                    }
                );
            }

            /**
             * @brief Get a component of a specific type via `DataIndex`.
             * @tparam TComponent The component type.
             * @param dataIndex The entity's `DataIndex`.
             * @return Reference to the component.
             */
            template <typename TComponent>
            auto& GetComponent(const DataIndex dataIndex) noexcept
            {
                return std::get<std::vector<TComponent>>(m_tupleOfComponentVectors)[dataIndex];
            }

        protected:

        private:
            using Settings = TSettings;
            using ComponentList = typename Settings::ComponentList;

            /**
             * @brief "Unpack" the types from `ComponentList` in `TupleOfComponentVectors`.
             */
            using TupleOfComponentVectors = typename TupleOfVectors<ComponentList>::type;

            TupleOfComponentVectors m_tupleOfComponentVectors;
        };

        //-------------------------------------------------
        // Settings
        //-------------------------------------------------

        /*
         * ----------------
         * Example of usage
         * ----------------
         * using MySettings = sg::ecs::Settings<MyComponentsList, MySignaturesList>;
         */

        /**
         * @brief Settings class with the custom `ComponentList` and `SignatureList`.
         * @tparam TComponentList The `ComponentList`.
         * @tparam TSignatureList The `SignatureList`.
         */
        template <typename TComponentList, typename TSignatureList>
        struct Settings
        {
            using ComponentList = TComponentList;
            using SignatureList = TSignatureList;
            using ThisType = Settings<ComponentList, SignatureList>;
            using Bitset = std::bitset<boost::mpl::size<ComponentList>::value>;
            using TupleOfSignatureBitsets = typename TupleTypeRepeater<boost::mpl::size<SignatureList>::value, Bitset>::type;
            using SignatureBitsetsStorage = SignatureBitsetsStorage<ThisType>;

            /**
             * @brief Determines the number of all component types.
             * @return std::size_t
             */
            static constexpr std::size_t ComponentCount() noexcept
            {
                return boost::mpl::size<ComponentList>();
            }

            /**
             * @brief Checks whether the passed component type is in the `ComponentList`.
             * @tparam TComponent The component type to be tested.
             * @return bool
             */
            template <typename TComponent>
            static constexpr bool IsValidComponent() noexcept
            {
                return boost::mpl::contains<ComponentList, TComponent>();
            }

            /**
             * @brief Returns the Id of the component type.
             * @tparam TComponent The component type.
             * @return std::size_t
             */
            template <typename TComponent>
            static constexpr std::size_t GetComponentId() noexcept
            {
                return boost::mpl::distance<typename boost::mpl::begin<ComponentList>::type, typename boost::mpl::find<ComponentList, TComponent>::type>::value;
            }

            /**
             * @brief Returns the bit index of the component type, which is the same as the Id.
             * @tparam TComponent The component type.
             * @return std::size_t
             */
            template <typename TComponent>
            static constexpr std::size_t GetComponentBit() noexcept
            {
                return GetComponentId<TComponent>();
            }

            /**
             * @brief Determines the number of all signature types.
             * @return std::size_t
             */
            static constexpr std::size_t SignatureCount() noexcept
            {
                return boost::mpl::size<SignatureList>();
            }

            /**
             * @brief Checks whether the passed signature type is in the `SignatureList`.
             * @tparam TSignature The signature type to be tested.
             * @return bool
             */
            template <typename TSignature>
            static constexpr bool IsValidSignature() noexcept
            {
                return boost::mpl::contains<SignatureList, TSignature>();
            }

            /**
             * @brief Returns the Id of the signature type.
             * @tparam TSignature The signature type.
             * @return std::size_t
             */
            template <typename TSignature>
            static constexpr std::size_t GetSignatureId() noexcept
            {
                return boost::mpl::distance<typename boost::mpl::begin<SignatureList>::type, typename boost::mpl::find<SignatureList, TSignature>::type>::value;
            }
        };

        //-------------------------------------------------
        // SignatureBitsetsStorage
        //-------------------------------------------------

        /**
         * @brief Creates and store the signature bitsets.
         * @tparam TSettings The Ecs settings and wrapper for the `ComponentList` and `SignatureList`.
         */
        template <typename TSettings>
        class SignatureBitsetsStorage
        {
        public:
            /**
             * @brief Get a bitset.
             * @tparam TSignature The signature type.
             * @return Reference to the bitset.
             */
            template <typename TSignature>
            auto& GetSignatureBitset() noexcept
            {
                static_assert(Settings::template IsValidSignature<TSignature>());

                return std::get<Settings::template GetSignatureId<TSignature>()>(m_tupleOfSignatureBitsets);
            }

            /**
             * @brief Get a bitset.
             * @tparam TSignature The signature type.
             * @return Const reference to the bitset.
             */
            template <typename TSignature>
            const auto& GetSignatureBitset() const noexcept
            {
                static_assert(Settings::template IsValidSignature<TSignature>());

                return std::get<Settings::template GetSignatureId<TSignature>()>(m_tupleOfSignatureBitsets);
            }

        protected:

        private:
            using Settings = TSettings;
            using SignatureList = typename TSettings::SignatureList;
            using TupleOfSignatureBitsets = typename Settings::TupleOfSignatureBitsets;

            TupleOfSignatureBitsets m_tupleOfSignatureBitsets;

            /**
             * @brief Initializing the bitset for a single signature.
             * @tparam TSignature Th signature type.
             */
            template <typename TSignature>
            void InitSignatureBitset() noexcept
            {
                auto& bitset{ GetSignatureBitset<TSignature>() };

                using SignatureComponents = TSignature;

                boost::mpl::for_each<SignatureComponents>([&bitset](auto componentType)
                {
                    bitset[Settings::template GetComponentBit<decltype(componentType)>()] = true;
                });
            }

        public:
            /**
             * @brief Initialize all bitsets on construction.
             */
            SignatureBitsetsStorage()
            {
                // Calls the `InitSignatureBitset()` method for each signature type from the `SignatureList`.
                boost::mpl::for_each<SignatureList>([this](auto signatureType)
                {
                    this->InitSignatureBitset<decltype(signatureType)>();
                });
            }
        };

        //-------------------------------------------------
        // Manager
        //-------------------------------------------------

        /*
         * ----------------
         * Example of usage
         * ----------------
         * using MyManager = sg::ecs::Manager<MySettings>;
         */

        /**
         * @brief Managed all entities and components at runtime.
         * @tparam TSettings The Ecs settings and wrapper for the `ComponentList` and `SignatureList`.
         */
        template <typename TSettings>
        class Manager
        {
        private:
            using Settings = TSettings;
            using ThisType = Manager<Settings>;
            using ComponentStorage = ComponentStorage<Settings>;
            using Bitset = typename Settings::Bitset;
            using Entity = Entity<Settings>;
            using SignatureBitsetsStorage = SignatureBitsetsStorage<Settings>;

            /**
             * @brief The entities are stored contiguously in a `std::vector`.
             */
            std::vector<Entity> m_entities;

            /**
             * @brief Size of allocated storage capacity for m_entities.
             */
            std::size_t m_capacity{ 0 };

            /**
             * @brief Current size.
             */
            std::size_t m_size{ 0 };

            /**
             * @brief Next size.
             */
            std::size_t m_sizeNext{ 0 };

            /**
             * @brief Wrapper of `TupleOfSignatureBitsets`.
             */
            SignatureBitsetsStorage m_signatureBitsetsStorage;

            /**
             * @brief Wrapper of `TupleOfComponentVectors`.
             */
            ComponentStorage m_componentStorage;

        public:
            Manager()
            {
                GrowTo(DEFAULT_ENTITY_CAPACITY);
            }

            /**
             * @brief Checks if the entity is alive.
             * @param entityIndex The entity index.
             * @return auto
             */
            auto IsAlive(const EntityIndex entityIndex) const noexcept
            {
                return GetEntity(entityIndex).alive;
            }

            /**
             * @brief Kills an entity.
             * @param entityIndex The entity index.
             */
            void Kill(const EntityIndex entityIndex) noexcept
            {
                GetEntity(entityIndex).alive = false;
            }

            /**
             * @brief Creates a new entity.
             * @return std::size_t
             */
            auto CreateIndex()
            {
                GrowIfNeeded();

                const auto freeIndex{ m_sizeNext++ };
                assert(!IsAlive(freeIndex));

                // the new created entity is alive
                auto& entity{ m_entities[freeIndex] };
                entity.alive = true;
                entity.bitset.reset();

                return freeIndex;
            }

            /**
             * @brief Clear the manager.
             */
            void Clear() noexcept
            {
                for (auto i{ 0u }; i < m_capacity; ++i)
                {
                    auto& entity(m_entities[i]);

                    entity.dataIndex = i;
                    entity.bitset.reset();
                    entity.alive = false;
                }

                m_size = m_sizeNext = 0;
            }

            /**
             * @brief Rearranges entities.
             */
            void Refresh() noexcept
            {
                // If no new entities have been created, set `m_size` to `0` and exit early.
                if (m_sizeNext == 0)
                {
                    m_size = 0;
                    return;
                }

                // Otherwise, get the new `m_size` by calling `ArrangeAliveEntitiesToLeft()`.
                // After refreshing, `m_size` will equal `m_sizeNext`.
                // The final value for these variables will be calculated
                // by re-arranging entity metadata in the `m_entities` vector.
                m_size = m_sizeNext = ArrangeAliveEntitiesToLeft();
            }

            /**
             * @brief Adds a component.
             * @tparam TComponent The component type.
             * @tparam TArgs The component parameter pack.
             * @param entityIndex The entity index.
             * @param args The component parameter pack.
             * @return Reference to the component.
             */
            template <typename TComponent, typename... TArgs>
            auto& AddComponent(const EntityIndex entityIndex, TArgs&&... args) noexcept
            {
                static_assert(Settings::template IsValidComponent<TComponent>());

                // update entity bitset
                auto& entity{ GetEntity(entityIndex) };
                entity.bitset[Settings::template GetComponentBit<TComponent>()] = true;

                // get component for re-construct
                auto& component{ m_componentStorage.template GetComponent<TComponent>(entity.dataIndex) };

                // placement new (construct an object on memory that's already allocated)
                new (&component) TComponent(std::forward<decltype(args)>(args)...);

                return component;
            }

            /**
             * @brief Checks if an entity is associated with a specific component type.
             * @tparam TComponent The component type.
             * @param entityIndex The entity index.
             * @return bool
             */
            template <typename TComponent>
            bool HasComponent(const EntityIndex entityIndex) const noexcept
            {
                static_assert(Settings::template IsValidComponent<TComponent>());

                return GetEntity(entityIndex).bitset[Settings::template GetComponentBit<TComponent>()];
            }

            /**
             * @brief Clears the association between the entity and the component type.
             * @tparam TComponent The component type.
             * @param entityIndex The entity index.
             */
            template <typename TComponent>
            void DeleteComponent(const EntityIndex entityIndex) noexcept
            {
                static_assert(Settings::template IsValidComponent<TComponent>());

                GetEntity(entityIndex).bitset[Settings::template GetComponentBit<TComponent>()] = false;
            }

            /**
             * @brief Returns a reference to the component.
             * @tparam TComponent The component type
             * @param entityIndex The entity index
             * @return Reference to the component.
             */
            template <typename TComponent>
            auto& GetComponent(const EntityIndex entityIndex) noexcept
            {
                static_assert(Settings::template IsValidComponent<TComponent>());

                assert(HasComponent<TComponent>(entityIndex));

                auto& entity{ GetEntity(entityIndex) };

                return m_componentStorage.template GetComponent<TComponent>(entity.dataIndex);
            }

            /**
             * @brief Checks if a entity matches a signature using `bitwise and` operation.
             * @tparam TSignature The signature type.
             * @param entityIndex The entity index.
             * @return bool
             */
            template <typename TSignature>
            auto MatchesSignature(const EntityIndex entityIndex) const noexcept
            {
                static_assert(Settings::template IsValidSignature<TSignature>());

                const auto& entityBitset{ GetEntity(entityIndex).bitset };
                const auto& signatureBitset{ m_signatureBitsetsStorage.template GetSignatureBitset<TSignature>() };

                return (signatureBitset & entityBitset) == signatureBitset;
            }

            /**
             * @brief Iterate over all alive entities.
             * @tparam TCallable A callable type.
             * @param callable A Closure to pass.
             */
            template <typename TCallable>
            void ForEntities(TCallable&& callable)
            {
                for (EntityIndex index{ 0 }; index < m_size; ++index)
                {
                    callable(index);
                }
            }

            /**
             * @brief Iterate over all alive entities matching a particular signature.
             * @tparam TSignature The signature type.
             * @tparam TCallable A callable type.
             * @param callable A Closure to pass.
             */
            template <typename TSignature, typename TCallable>
            void ForEntitiesMatching(TCallable&& callable)
            {
                static_assert(Settings::template IsValidSignature<TSignature>());

                ForEntities
                (
                    [this, &callable](auto entityIndex)
                    {
                        if (MatchesSignature<TSignature>(entityIndex))
                        {
                            this->template ExpandSignatureCall<TSignature>(entityIndex, callable);
                        }
                    }
                );
            }

            /**
             * @brief Returns the number of alive entities.
             * @return std::size_t
             */
            std::size_t GetEntityCount() const noexcept
            {
                return m_size;
            }

            /**
             * @brief Print the state of the entity metadata.
             * @param oss std::ostream
             */
            void PrintState(std::ostream& oss) const
            {
                oss << "\nsize: " << m_size
                    << "\nsizeNext: " << m_sizeNext
                    << "\ncapacity: " << m_capacity
                    << "\n";

                for (auto i{ 0u }; i < m_sizeNext; ++i)
                {
                    auto& e(m_entities[i]);
                    oss << (e.alive ? "A" : "D");
                }

                oss << "\n\n";
            }

        protected:

        private:
            /**
             * @brief Grow entity and component vectors.
             * @param newCapacity The new capacity.
             */
            void GrowTo(std::size_t newCapacity)
            {
                assert(newCapacity > m_capacity);

                m_entities.resize(newCapacity);
                m_componentStorage.GrowTo(newCapacity);

                // initialize the the entities to default values
                for (auto i{ m_capacity }; i < newCapacity; ++i)
                {
                    auto& entity{ m_entities[i] };
                    entity.dataIndex = i;
                    entity.bitset.reset();
                    entity.alive = false;
                }

                m_capacity = newCapacity;
            }

            /**
             * @brief Run `GrowTo()` with a fixed value, if needed.
             */
            void GrowIfNeeded()
            {
                if (m_capacity > m_sizeNext)
                {
                    return;
                }

                // `GrowTo()` with fixed value
                GrowTo((m_capacity + 10) * 2);
            }

            /**
             * @brief Get entity by index.
             * @param entityIndex The entity index.
             * @return Reference to the entity.
             */
            auto& GetEntity(const EntityIndex entityIndex) noexcept
            {
                assert(m_sizeNext > entityIndex);
                return m_entities[entityIndex];
            }

            /**
             * @brief Get entity by index.
             * @param entityIndex The entity index.
             * @return Const reference to the entity.
             */
            const auto& GetEntity(const EntityIndex entityIndex) const noexcept
            {
                assert(m_sizeNext > entityIndex);
                return m_entities[entityIndex];
            }

            /**
             * @brief Alive entities found on the right will be swapped with dead entities found on the left.
             * @return The number of alive entities, which is one-past the index of the last alive entity.
             */
            EntityIndex ArrangeAliveEntitiesToLeft() noexcept
            {
                // The algorithm is implemented using two indices.
                // * `iD` looks for dead entities, starting from the left.
                // * `iA` looks for alive entities, starting from the right.

                EntityIndex iD{ 0 }, iA{ m_sizeNext - 1 };

                while (true)
                {
                    // Find first dead entity from the left.
                    for (; true; ++iD)
                    {
                        // We have to immediately check if we have gone
                        // through the `iA` index. If that's the case, there
                        // are no more dead entities in the vector and we can
                        // return `iD` as our result.
                        if (iD > iA) return iD;

                        // If we found a dead entity, we break out of this
                        // inner for-loop.
                        if (!m_entities[iD].alive) break;
                    }

                    // Find first alive entity from the right.
                    for (; true; --iA)
                    {
                        // In the case of alive entities, we have to perform
                        // the checks done above in the reverse order.

                        // If we found an alive entity, we immediately break
                        // out of this inner for-loop.
                        if (m_entities[iA].alive) break;

                        // Otherwise, we acknowledge this is an entity that
                        // has been killed since last refresh.
                        // We will invalidate its handle here later.

                        // If we reached `iD` or gone through it, we are
                        // certain there are no more alive entities in the
                        // entity metadata vector, and we can return `iD`.
                        if (iA <= iD) return iD;
                    }

                    // If we arrived here, we have found two entities that
                    // need to be swapped.

                    // `iA` points to an alive entity, towards the right of
                    // the vector.
                    assert(m_entities[iA].alive);

                    // `iD` points to a dead entity, towards the left of the
                    // vector.
                    assert(!m_entities[iD].alive);

                    // Therefore, we swap them to arrange all alive entities
                    // towards the left.
                    std::swap(m_entities[iA], m_entities[iD]);

                    // After swapping, we will eventually need to refresh
                    // the alive entity's handle and invalidate the dead
                    // swapped entity's handle.

                    // Move both "iterator" indices.
                    ++iD; --iA;
                }
            }

            /**
             * @brief Inner helper class. It contains a single static `call` function.
             * @tparam TComponents A variadic number of component types.
             */
            template <typename... TComponents>
            struct ExpandCallHelper
            {
                /**
                 * @brief Expand the component types into component references.
                 * @tparam TCallable A callable type.
                 * @param entityIndex The index of the entity.
                 * @param manager A reference to the caller manager.
                 * @param callable The function to call.
                 */
                template<typename TCallable>
                static void Call(const EntityIndex entityIndex, ThisType& manager, TCallable&& callable)
                {
                    auto dataIndex{ manager.GetEntity(entityIndex).dataIndex };

                    callable
                    (
                        entityIndex,
                        manager.m_componentStorage.template GetComponent<TComponents>(dataIndex)...
                    );
                }
            };

            /**
             * @brief Rename TypeList to `ExpandCallHelper`.
             * @tparam TSignature A signature type.
             * @tparam TCallable A callable type.
             * @param entityIndex The entity index.
             * @param callable A Closure.
             */
            template <typename TSignature, typename TCallable>
            void ExpandSignatureCall(const EntityIndex entityIndex, TCallable&& callable)
            {
                static_assert(Settings::template IsValidSignature<TSignature>());

                using RequiredComponents = TSignature;
                using Helper = typename Rename<RequiredComponents, ExpandCallHelper>::type;

                Helper::Call(entityIndex, *this, callable);
            }
        };
    }
}
