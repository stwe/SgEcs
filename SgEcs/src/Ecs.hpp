#pragma once

#include <cassert>
#include <boost/mpl/list.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/for_each.hpp>
#include <bitset>
#include <vector>
#include <tuple>

namespace sg
{
    namespace ecs
    {
        static constexpr std::size_t INITIAL_MANAGER_GROW_TO{ 100 };

        //-------------------------------------------------
        // Listen
        //-------------------------------------------------

        // aus allen Komponenten eine Liste erstellen
        template <typename... TComponentTypes>
        using ComponentList = boost::mpl::list<TComponentTypes...>;

        // aus ein oder mehreren Komponenten eine Signatur erstellen
        template <typename... TComponent>
        using Signature = boost::mpl::list<TComponent...>;

        // aus allen Signaturen eine Liste erstellen
        template <typename... TSignatures>
        using SignatureList = boost::mpl::list<TSignatures...>;

        //-------------------------------------------------
        // Entity
        //-------------------------------------------------

        using DataIndex = std::size_t;
        using EntityIndex = std::size_t;

        template <typename TSettings>
        struct Entity
        {
            using Settings = TSettings;
            using Bitset = typename Settings::Bitset;

            DataIndex dataIndex{ 0 };
            Bitset bitset;
            bool alive{ false };
        };

        //-------------------------------------------------
        // Components Storage
        //-------------------------------------------------

        template <typename TSettings>
        class ComponentStorage
        {
        public:
            void Grow(std::size_t newCapacity)
            {
                // die Größe eines std::vector entspricht der Größe von m_entities
                boost::mpl::for_each<ComponentList>
                (
                    [&tupleOfComponentVectors = m_tupleOfComponentVectors, newCapacity](auto arg)
                    {
                        std::get<std::vector<decltype(arg)>>(tupleOfComponentVectors).resize(newCapacity);
                    }
                );
            }

            template <typename TComponent>
            auto& GetComponent(const DataIndex dataIndex) noexcept
            {
                return std::get<std::vector<TComponent>>(m_tupleOfComponentVectors)[dataIndex];
            }

        protected:

        private:
            using Settings = TSettings;
            using ComponentList = typename Settings::ComponentList;

            // Die Komponenten sind zur Kompilierzeit bekannt, daher kann
            // ein std::tuple verwendet werden.

            template <typename... Ts>
            using TupleOfVectors = std::tuple<std::vector<Ts>...>;

            template <typename Seq, typename T>
            struct AddTo;

            template <typename T, typename... Ts>
            struct AddTo<TupleOfVectors<Ts...>, T>
            {
                using type = TupleOfVectors<T, Ts...>;
            };

            using TupleOfComponentVectors = typename boost::mpl::fold<
                ComponentList,
                TupleOfVectors<>,
                AddTo<boost::mpl::_1, boost::mpl::_2>
            >::type;

            TupleOfComponentVectors m_tupleOfComponentVectors;
        };

        //-------------------------------------------------
        // Settings
        //-------------------------------------------------

        template <typename TComponentList, typename TSignatureList>
        struct Settings
        {
            using ComponentList = TComponentList;
            using SignatureList = TSignatureList;
            using ThisType = Settings<ComponentList, SignatureList>;
            using Bitset = std::bitset<boost::mpl::size<ComponentList>::value>;

            // Komponenten

            static constexpr std::size_t ComponentCount() noexcept
            {
                return boost::mpl::size<ComponentList>();
            }

            template<typename TComponent>
            static constexpr bool IsValidComponent() noexcept
            {
                return boost::mpl::contains<ComponentList, TComponent>();
            }

            template<typename TComponent>
            static constexpr std::size_t GetComponentId() noexcept
            {
                return boost::mpl::distance<typename boost::mpl::begin<ComponentList>::type, typename boost::mpl::find<ComponentList, TComponent>::type>::value;
            }

            template<typename TComponent>
            static constexpr std::size_t GetComponentBit() noexcept
            {
                return GetComponentId<TComponent>();
            }

            // Signaturen

            static constexpr std::size_t SignatureCount() noexcept
            {
                return boost::mpl::size<SignatureList>();
            }

            template<typename TSignature>
            static constexpr bool IsSignature() noexcept
            {
                return boost::mpl::contains<SignatureList, TSignature>();
            }

            template<typename TSignature>
            static constexpr std::size_t GetSignatureId() noexcept
            {
                return boost::mpl::distance<typename boost::mpl::begin<SignatureList>::type, typename boost::mpl::find<SignatureList, TSignature>::type>::value;
            }
        };

        //-------------------------------------------------
        // Manager
        //-------------------------------------------------

        template <typename TSettings>
        class Manager
        {
        public:
            Manager()
            {
                GrowTo(INITIAL_MANAGER_GROW_TO);
            }

            auto IsAlive(const EntityIndex entityIndex) const noexcept
            {
                return GetEntity(entityIndex).alive;
            }

            auto CreateIndex()
            {
                GrowIfNeeded();

                // nächsten freien Index holen und `m_sizeNext` danach erhöhen
                const auto freeIndex{ m_sizeNext++ };

                // dort muss sich eine "tote" Entity befinden
                assert(!IsAlive(freeIndex));

                // die neue Entity aktivieren und alle component Bits auf 0 setzen
                auto& entity{ m_entities[freeIndex] };
                entity.alive = true;
                entity.bitset.reset();

                return freeIndex;
            }

            template <typename TComponent, typename... TArgs>
            auto& AddComponent(const EntityIndex entityIndex, TArgs&&... args) noexcept
            {
                // prüft, ob die die Komponente in der Komponentenliste ist
                static_assert(Settings::template IsValidComponent<TComponent>());

                // entsprechendes Bit im Bitset der Entity setzen
                auto& entity{ GetEntity(entityIndex) };
                entity.bitset[Settings::template GetComponentBit<TComponent>()] = true;

                auto& component{ m_components.template GetComponent<TComponent>(entity.dataIndex) };
                new (&component) TComponent(std::forward<decltype(args)>(args)...);
                return component;
            }

        protected:

        private:
            using Settings = TSettings;
            using Bitset = typename Settings::Bitset;
            using Entity = Entity<Settings>;
            using ComponentStorage = ComponentStorage<Settings>;

            std::vector<Entity> m_entities;

            std::size_t m_capacity{ 0 };
            std::size_t m_size{ 0 };
            std::size_t m_sizeNext{ 0 };

            ComponentStorage m_components;

            void GrowTo(std::size_t newCapacity)
            {
                assert(newCapacity > m_capacity);

                // hängt zusätzliche Enities an den vector
                m_entities.resize(newCapacity);

                // jede Komponente hat einen eigenen std::vector; der Entity `dataIndex` wird
                // dort als Index verwendet; also muss die Größe = m_entities entsprechen
                m_components.Grow(newCapacity);

                // initialisiert die neuen Entities
                for (auto i{ m_capacity }; i < newCapacity; ++i)
                {
                    auto& entity{ m_entities[i] };
                    entity.dataIndex = i;  // den aktuelle Index als `dataIndex` setzen
                    entity.bitset.reset(); // setzt alle Bits auf 0
                    entity.alive = false;  // setzt den Status auf "dead"
                }

                m_capacity = newCapacity;
            }

            void GrowIfNeeded()
            {
                if (m_capacity > m_sizeNext)
                {
                    return;
                }

                // um einen festen Wert erhöhen
                GrowTo((m_capacity + 10) * 2);
            }

            auto& GetEntity(const EntityIndex entityIndex) noexcept
            {
                assert(m_sizeNext > entityIndex);
                return m_entities[entityIndex];
            }

            const auto& GetEntity(const EntityIndex entityIndex) const noexcept
            {
                assert(m_sizeNext > entityIndex);
                return m_entities[entityIndex];
            }
        };
    }
}
