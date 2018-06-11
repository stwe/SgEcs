#pragma once

#include <tuple>
#include <boost/mpl/fold.hpp>

namespace sg
{
    namespace ecs
    {
        //-------------------------------------------------
        // Tuple type repeater
        //-------------------------------------------------

        // Job: std::tuple<float, float, float, float>
        // Call: using MyTuple = typename TypeRepeater<4, float>::type;

        template <unsigned int N, typename T>
        struct TupleTypeRepeater
        {
            using type = decltype(
                std::tuple_cat(
                    std::declval<std::tuple<T>>(),
                    std::declval<typename TupleTypeRepeater<N - 1, T>::type>()
                )
            );
        };

        template <typename T>
        struct TupleTypeRepeater<0, T>
        {
            using type = std::tuple<>;
        };

        //-------------------------------------------------
        // Tuple of vectors
        //-------------------------------------------------

        // Job: std::tuple<std::vector<Pos>, std::vector<Foo>, std::vector<Bar>>
        // Call: using MyTupleOfComponentVectors = TupleOfVectors<MyList>::type;

        template <typename TList>
        struct TupleOfVectors
        {
            template <typename... Ts>
            using Tuple = std::tuple<std::vector<Ts>...>;

            template <typename Seq, typename T>
            struct AddTo;

            template <typename T, typename... Ts>
            struct AddTo<Tuple<Ts...>, T>
            {
                using type = Tuple<T, Ts...>;
            };

            using type = typename boost::mpl::fold<
                TList,
                Tuple<>,
                AddTo<boost::mpl::_1, boost::mpl::_2>
            >::type;
        };

        //-------------------------------------------------
        // Rename TypeList to a new type
        //-------------------------------------------------

        // Job:
        // Call:

        template <typename TList, template <typename...> typename TNewName>
        struct Rename
        {
            template <typename Seq, typename T>
            struct AddTo;

            template <typename T, typename... Ts>
            struct AddTo<TNewName<Ts...>, T>
            {
                using type = TNewName<T, Ts...>;
            };

            using type = typename boost::mpl::fold<
                TList,
                TNewName<>,
                AddTo<boost::mpl::_1, boost::mpl::_2>
            >::type;
        };
    }
}
