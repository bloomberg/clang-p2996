// p2996_examples.cpp
// Examples from Document #: P2996
// https://isocpp.org/files/papers/P2996R13.html#examples

// clang is installed in /opt/p2996/clang
// the directory /opt/p2996/clang is expected to have the following sub-directories
// bin	include	lib	libexec	share

/* To compile on macOS
/opt/p2996/clang/bin/clang++ \
    p2996_examples.cpp -o bin/p2996_examples \
    -std=c++26 \
    -freflection-latest \
    -I/opt/p2996/clang/include/c++/v1 \
    -isysroot $(xcrun --sdk macosx --show-sdk-path) \
    -L/opt/p2996/clang/lib \
    -lc++abi \
    -Wl,-rpath,/opt/p2996/clang/lib
*/
/* To run
bin/p2996_examples --file_name some-file.ext
*/

// To compile on Linux see README.md

/*
 * References:
 *  Trip report: June 2025 ISO C++ standards meeting (Sofia, Bulgaria),
 *      https://herbsutter.com/2025/06/
 *  Three Cool Things in C++26: Safety, Reflection & std::execution - Herb Sutter - C++ on Sea 2025
 *      https://www.youtube.com/watch?v=kKbT0Vg3ISw
 */


#include <iostream>
#include <cassert>

#include <experimental/meta>
#include <array>
#include <ranges>
#include <map>
#include <algorithm>

struct S { unsigned i:2, j:6; };

consteval auto member_number(int n) {
    if (n == 0) return ^^S::i;
    else if (n == 1) return ^^S::j;
}

consteval auto member_number_1(int n) {
    auto ctx = std::meta::access_context::current();
    return std::meta::nonstatic_data_members_of(^^S, ctx)[n];
}
consteval auto member_named(std::string_view name) {
    auto ctx = std::meta::access_context::current();
    for (std::meta::info field : nonstatic_data_members_of(^^S, ctx)) {
        if (has_identifier(field) && identifier_of(field) == name)
            return field;
    }
}

template<class...> struct list_of_types {};

// 3.4 Implementing make_integer_sequence
#include <utility>
#include <vector>

template<typename T>
consteval std::meta::info make_integer_seq_refl(T N) {
    std::vector args{^^T};
    for (T k = 0; k < N; ++k) {
        args.push_back(std::meta::reflect_constant(k));
    }
    return substitute(^^std::integer_sequence, args);
}

template<typename T, T N>
  using make_integer_sequence = [:make_integer_seq_refl<T>(N):];

template<int... Is>
void print_seq(std::integer_sequence<int, Is...>) {
    ((std::cout << Is << " "), ...);
    std::cout << "\n";
}
// 3.4 Implementing make_integer_sequence. End


// 3.5 Getting Class Layout
// returns std::array<member_descriptor, N>
struct member_descriptor
{
    std::size_t offset;
    std::size_t size;
    bool operator==(member_descriptor const&) const = default;
};

template <typename S>
consteval auto get_layout() {
    constexpr auto ctx = std::meta::access_context::current();
    constexpr size_t N = std::meta::nonstatic_data_members_of(^^S, ctx).size();
    auto members = std::meta::nonstatic_data_members_of(^^S, ctx);

    std::array<member_descriptor, N> layout;
    for (int i = 0; i < members.size(); ++i) {
        layout[i] = {
            .offset=static_cast<std::size_t>(std::meta::offset_of(members[i]).bytes),
            .size=std::meta::size_of(members[i])
        };
    }
    return layout;
}
// 3.5 Getting Class Layout. End

// 3.6 Enum to String
// Example enum_to_string using expansion statements
template<typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
  requires std::is_enum_v<E>
constexpr std::string_view enum_to_string(E value) {
    if constexpr (Enumerable)
        // C++26 introduces a new expansion statement syntax:
        // template for (...) { ... }—which allows compile-time looping over reflected entities, such as enum enumerators
        template for (constexpr auto e :
                      std::define_static_array(std::meta::enumerators_of(^^E)))
            if (value == [:e:])
                return std::meta::identifier_of(e);

    return "<unnamed>";
}

template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
  requires std::is_enum_v<E>
constexpr std::optional<E> string_to_enum(std::string_view name) {
    if constexpr (Enumerable)
        /* C++26 introduces a new expansion statement syntax:
         * `template for (...) { ... }`
         * which allows compile-time looping over reflected entities,
         * such as enum enumerators.
         * The `template for` expansion statement enables elegant compile-time iteration
         * over reflected entities
         * without manual repetition or macros —
         * particularly powerful for things like enum-to-string mappings.
         */
        template for (constexpr auto e :
                      std::define_static_array(std::meta::enumerators_of(^^E)))
            if (name == std::meta::identifier_of(e))
                return [:e:];

    return std::nullopt;
}

// Example enum_to_string using array and find_if
template <typename E>
  requires std::is_enum_v<E>
constexpr std::string enum_to_string_array_find_if(E value) {
    static constexpr auto get_pairs_array = []{
        // materialize reflection range into constexpr array of `std::meta::info`
        constexpr auto enums = std::define_static_array(
            std::meta::enumerators_of(^^E)
        );

        using pair_t = std::pair<E, std::string_view>;
        std::array<pair_t, enums.size()> result{};

        std::ranges::transform(enums, result.begin(),
            [](std::meta::info e) {
                return pair_t{ std::meta::extract<E>(e),
                               std::meta::identifier_of(e) };
            });

        return result;
    };
    constexpr auto get_name = [&](E value) -> std::optional<std::string> {
        constexpr auto enumerators = get_pairs_array();
        auto it = std::ranges::find_if(enumerators, [value](auto const& pr){
          return pr.first == value;
        });
        if (it == enumerators.end()) {
            return std::nullopt;
        } else {
            return std::optional(std::string(it->second));
        }
    };

    return get_name(value).value_or("<unnamed>");
}

// Example to use a compile-time map
template <typename E>
struct constexpr_enum_map {
    static_assert(std::is_enum_v<E>, "constexpr_enum_map requires an enum type");

    using pair_t = std::pair<E, std::string_view>;

    static consteval auto make_array() {
        constexpr auto enums = std::define_static_array(std::meta::enumerators_of(^^E));

        std::array<pair_t, enums.size()> result{};
        std::ranges::transform(enums, result.begin(),
            [](std::meta::info e) {
                return pair_t{ std::meta::extract<E>(e),
                               std::meta::identifier_of(e) };
            });
        std::ranges::sort(result, {}, &pair_t::first);
        return result;
    }

    static constexpr auto data_ = make_array();   // ← static consteval init

    constexpr_enum_map() = default;

    constexpr auto begin() const { return data_.begin(); }
    constexpr auto end()   const { return data_.end(); }

    constexpr auto find(E key) const {
        std::size_t left = 0, right = data_.size();
        while (left < right) {
            auto mid = (left + right) / 2;
            if (data_[mid].first == key)
                return data_.begin() + mid;
            else if (data_[mid].first < key)
                left = mid + 1;
            else
                right = mid;
        }
        return data_.end();
    }

    constexpr std::optional<std::string_view> at(E key) const {
        auto it = find(key);
        if (it == end())
            return std::nullopt;
        return it->second;
    }
};

template <typename E>
constexpr std::string enum_to_string_map_find(E value) {
    constexpr constexpr_enum_map<E> map{};
    return std::string(map.at(value).value_or("<unnamed>"));
}

// Combining enum_to_string_array_find_if and enum_to_string_map_find
template <typename E>
  requires std::is_enum_v<E>
constexpr std::string enum_to_string_alg(E value) {
    constexpr auto enums = std::define_static_array(std::meta::enumerators_of(^^E));
    constexpr auto size = enums.size();
    if constexpr (size <= 7) {
        return enum_to_string_array_find_if(value);
    } else {
        return enum_to_string_map_find(value);
    }
}
// 3.6 Enum to String. End

// 3.7 Parsing Command-Line Options
#include <sstream>
template<typename Opts>
auto parse_options(std::span<std::string_view const> args) -> Opts {
    Opts opts;

    constexpr auto ctx = std::meta::access_context::current();
    template for (constexpr auto dm : std::define_static_array(nonstatic_data_members_of(^^Opts, ctx))) {
        auto it = std::ranges::find_if(args,
          [](std::string_view arg){
            return arg.starts_with("--") && arg.substr(2) == identifier_of(dm);
          });

        if (it == args.end()) {
            // no option provided, use default
            continue;
        } else if (it + 1 == args.end()) {
            std::print(stderr, "Option {} is missing a value\n", *it);
            std::exit(EXIT_FAILURE);
        }

        using T = typename[:type_of(dm):];
        auto iss = std::stringstream(it[1]);
        if (iss >> opts.[:dm:]; !iss) {
            std::print(stderr, "Failed to parse option {} into a {}\n", *it, display_string_of(^^T));
            std::exit(EXIT_FAILURE);
        }
    }
    return opts;
}

struct MyOpts {
    std::string file_name = "input.txt";  // Option "--file_name <string>"
    int    count = 1;                     // Option "--count <int>"
};

// 3.7 Parsing Command-Line Options. End

// 3.8 A Simple Tuple Type
template<typename... Ts> struct Tuple {
    struct storage;
    consteval {
        define_aggregate(^^storage, {data_member_spec(^^Ts)...});
    }
    storage data;

    Tuple(): data{} {}
    Tuple(Ts const& ...vs): data{ vs... } {}
};

template<typename... Ts>
  struct std::tuple_size<Tuple<Ts...>>: public integral_constant<size_t, sizeof...(Ts)> {};

template<std::size_t I, typename... Ts>
  struct std::tuple_element<I, Tuple<Ts...>> {
    static constexpr std::array types = {^^Ts...};
    using type = [: types[I] :];
};

consteval std::meta::info get_nth_nsdm(std::meta::info r, std::size_t n) {
    return nonstatic_data_members_of(r, std::meta::access_context::current())[n];
}

template<std::size_t I, typename... Ts>
  constexpr auto get(Tuple<Ts...> &t) noexcept -> std::tuple_element_t<I, Tuple<Ts...>>& {
    return t.data.[:get_nth_nsdm(^^decltype(t.data), I):];
}

template<std::size_t I, typename... Ts>
  constexpr auto get(Tuple<Ts...> const&t) noexcept -> std::tuple_element_t<I, Tuple<Ts...>> const& {
    return t.data.[:get_nth_nsdm(^^decltype(t.data), I):];
}

template<std::size_t I, typename... Ts>
  constexpr auto get(Tuple<Ts...> &&t) noexcept -> std::tuple_element_t<I, Tuple<Ts...>> && {
    return std::move(t).data.[:get_nth_nsdm(^^decltype(t.data), I):];
}
// 3.8 A Simple Tuple Type. End

// 3.9 A Simple Variant Type
// requires boost/mp11.hpp thus omitted
// 3.9 A Simple Variant Type. End

// 3.10 Struct to Struct of Arrays
template <typename T, size_t N>
struct struct_of_arrays_impl {
    struct impl;

    consteval {
        auto ctx = std::meta::access_context::current();

        std::vector<std::meta::info> old_members = nonstatic_data_members_of(^^T, ctx);
        std::vector<std::meta::info> new_members = {};
        for (std::meta::info member : old_members) {
            auto array_type = substitute(^^std::array, {
                type_of(member),
                std::meta::reflect_constant(N),
            });
            auto mem_descr = data_member_spec(array_type, {.name = identifier_of(member)});
            new_members.push_back(mem_descr);
        }

        define_aggregate(^^impl, new_members);
    }
};

template <typename T, size_t N>
using struct_of_arrays = struct_of_arrays_impl<T, N>::impl;
// 3.10 Struct to Struct of Arrays. End

// 3.11 Parsing Command-Line Options II
namespace clap {
    struct Flags {
        bool use_short;
        bool use_long;
    };

    template <typename T, Flags flags>
    struct Option {
        std::optional<T> initializer;

        Option() = default;
        Option(T t) : initializer(t) { }

        static constexpr bool use_short = flags.use_short;
        static constexpr bool use_long = flags.use_long;
    };

    consteval auto spec_to_opts(std::meta::info opts, std::meta::info spec) -> std::meta::info {
        std::vector<std::meta::info> new_members;
        for (auto member :
              nonstatic_data_members_of(spec, std::meta::access_context::current())) {
            auto new_type = template_arguments_of(type_of(member))[0];
            new_members.push_back(data_member_spec(new_type, {.name=identifier_of(member)}));
        }
        return define_aggregate(opts, new_members);
    }

    struct Clap {
        template <typename Spec>
        auto parse(this Spec const& spec, int argc, const char** argv) {
            std::vector<std::string_view> cmdline(argv + 1, argv + argc);

            // check if cmdline contains --help, etc.

            struct Opts;
            consteval {
                spec_to_opts(^^Opts, ^^Spec);
            }
            Opts opts;

            constexpr auto ctx = std::meta::access_context::current();
            // The "unrolled" version - no compiler error
            constexpr auto spec_arr = std::define_static_array(nonstatic_data_members_of(^^Spec, ctx));
            constexpr auto opts_arr = std::define_static_array(nonstatic_data_members_of(^^Opts, ctx));
            constexpr auto pairs = std::views::zip(spec_arr, opts_arr);
            constexpr auto pairs_arr = std::define_static_array(
                std::views::transform(pairs, [](auto z) { return std::pair(get<0>(z), get<1>(z)); }));
            template for (constexpr auto Pair : pairs_arr) {

            /* The "rolled up" version of 'template for (constexpr auto Pair : ...' below gives compiler error:
              no matching function for call to object of type 'const __transform::__fn'
              std::views::transform(std::views::zip(
            */
            /* Thus, commented out the "rolled up" version and use the "unrolled" version instead.
            template for (constexpr auto Pair : std::define_static_array(
              std::views::transform(std::views::zip(
                std::define_static_array(nonstatic_data_members_of(^^Spec, ctx)),
                std::define_static_array(nonstatic_data_members_of(^^Opts, ctx))),
                [](auto z) { return std::pair(get<0>(z), get<1>(z)); }))) {
            */

                constexpr auto sm = Pair.first;
                constexpr auto om = Pair.second;

                auto& cur = spec.[:sm:];
                constexpr auto type = type_of(om);

                // find the argument associated with this option
                auto it = std::find_if(cmdline.begin(), cmdline.end(),
                    [&](std::string_view arg) {
                        return (cur.use_short && arg.size() == 2 && arg[0] == '-' &&
                                arg[1] == identifier_of(sm)[0])
                            || (cur.use_long && arg.starts_with("--") &&
                                arg.substr(2) == identifier_of(sm));
                    });

                // no such argument
                if (it == cmdline.end()) {
                    if constexpr (has_template_arguments(type) &&
                                template_of(type) == ^^std::optional) {
                        // the type is optional, so the argument is too
                        continue;
                    } else if (cur.initializer) {
                        // the type isn't optional, but an initializer is provided, use that
                        opts.[:om:] = *cur.initializer;
                        continue;
                    } else {
                        std::cerr << "Missing required option "
                                << display_string_of(sm) << '\n';
                        std::exit(EXIT_FAILURE);
                    }
                } else if (it + 1 == cmdline.end()) {
                    std::cout << "Option " << *it << " for " << display_string_of(sm)
                            << " is missing a value\n";
                    std::exit(EXIT_FAILURE);
                }

                // alright, found our argument, try to parse it
                std::stringstream iss;
                iss << it[1];
                if (iss >> opts.[:om:]; !iss) {
                    std::cerr << "Failed to parse " << it[1] << " into option "
                            << display_string_of(sm) << " of type "
                            << display_string_of(type_of(om)) << '\n';
                    std::exit(EXIT_FAILURE);
                }
            }

            return opts;
        }
    };
}
// 3.11 Parsing Command-Line Options II. End

// 3.12 A Universal Formatter
struct universal_formatter {
    constexpr auto parse(auto& ctx) { return ctx.begin(); }

    template <typename T>
    auto format(T const& t, auto& ctx) const {
        std::string_view type_label = "(unnamed-type)";
        if constexpr (has_identifier(^^T))
            type_label = identifier_of(^^T);
        auto out = std::format_to(ctx.out(), "{}{{", type_label);

        auto delim = [first=true, &out]() mutable {
            if (!first) {
                *out++ = ',';
                *out++ = ' ';
            }
            first = false;
        };

        constexpr auto access_ctx = std::meta::access_context::unchecked();

        template for (constexpr auto base : define_static_array(bases_of(^^T, access_ctx))) {
            delim();
            out = std::format_to(out, "{}", (typename [: type_of(base) :] const&)(t));
        }

        template for (constexpr auto mem :
                      define_static_array(nonstatic_data_members_of(^^T, access_ctx))) {
            delim();

            std::string_view mem_label = "unnamed-member";
            if constexpr (has_identifier(mem)) mem_label = identifier_of(mem);

            if constexpr (is_bit_field(mem) && !has_identifier(mem))
                out = std::format_to(out, "(unnamed-bitfield)");
            else
                out = std::format_to(out, ".{}={}", mem_label, t.[:mem:]);
        }

        *out++ = '}';
        return out;
    }
};

struct B { int m0 = 0; };
struct X : B { int m1 = 1; };
struct Y : B { int m2 = 2; };
class Z : public X, private Y {
    struct { int : 0; } s;
    int m3 = 3; int m4 = 4;
};

template <> struct std::formatter<B> : universal_formatter { };
template <> struct std::formatter<X> : universal_formatter { };
template <> struct std::formatter<Y> : universal_formatter { };
template <> struct std::formatter<Z> : universal_formatter { };
template <> struct std::formatter<decltype(Z::s)> : universal_formatter { };
// 3.12 A Universal Formatter. End

// 3.13 Implementing member-wise hash_append
// Array-aware append (covers char[N], T[N], etc.)
template <typename H, typename T, std::size_t N>
void hash_append_one(H& h, const T (&arr)[N]) {
    for (const auto& x : arr) h.append(x);
}

template <typename H, typename T>
void hash_append_one(H& h, const T& v) {
    h.append(v);
}
template <typename H, typename T> requires std::is_standard_layout_v<T>
void hash_append(H& algo, T const& t) {
    constexpr auto ctx = std::meta::access_context::unchecked();
    template for (constexpr auto mem : define_static_array(nonstatic_data_members_of(^^T, ctx))) {
        hash_append_one(algo, t.[:mem:]);
    }
}

// Simple stateful combiner that uses std::hash<T> per value and mixes it.
// This is not cryptographic; it’s just a practical combiner.
struct StdHashCombiner {
    std::size_t state = 0xcbf29ce484222325ull; // FNV offset basis

    template <class T>
    void append(const T& v) noexcept {
        using U = std::remove_cvref_t<T>;
        std::size_t h = std::hash<U>{}(v);
        // mix: similar to boost::hash_combine
        state ^= h + 0x9e3779b97f4a7c15ull + (state << 6) + (state >> 2);
    }

    std::size_t finish() const noexcept { return state; }
};

template <class T>
  std::size_t hash_value(const T& t) {
    StdHashCombiner h;
    hash_append(h, t);
    return h.finish();
}
// 3.13 Implementing member-wise hash_append. End

// 3.14 Converting a Struct to a Tuple
// This approach requires allowing packs in structured bindings [P1061R10]
/*
template <typename T>
constexpr auto struct_to_tuple(T const& t) {
    constexpr auto ctx = std::meta::access_context::current();

    constexpr std::size_t N = nonstatic_data_members_of(^^T, ctx).size();
    auto members = nonstatic_data_members_of(^^T, ctx);

    constexpr auto indices = []{
        std::array<int, N> indices;
        std::ranges::iota(indices, 0);
        return indices;
    }();

    constexpr auto [...Is] = indices;
    return std::make_tuple(t.[: members[Is] :]...);
}
*/

// An alternative approach is:
consteval auto type_struct_to_tuple(std::meta::info type) -> std::meta::info {
    constexpr auto ctx = std::meta::access_context::current();
    return substitute(^^std::tuple,
                      nonstatic_data_members_of(type, ctx)
                      | std::views::transform(std::meta::type_of)
                      | std::views::transform(std::meta::remove_cvref)
                      | std::ranges::to<std::vector>());
}

template <typename To, typename From, std::meta::info ... members>
constexpr auto struct_to_tuple_helper(From const& from) -> To {
    return To(from.[:members:]...);
}

template<typename From>
consteval auto get_struct_to_tuple_helper() {
    using To = [: type_struct_to_tuple(^^From) :];
    auto ctx = std::meta::access_context::current();

    std::vector args = {^^To, ^^From};
    for (auto mem : nonstatic_data_members_of(^^From, ctx)) {
        args.push_back(reflect_constant(mem));
    }

    /*
    Alternatively, with Ranges:
    args.append_range(
      nonstatic_data_members_of(^^From, ctx)
      | std::views::transform(std::meta::reflect_constant)
      );
    */

    return extract<To(*)(From const&)>(
      substitute(^^struct_to_tuple_helper, args));
}

template <typename From>
constexpr auto struct_to_tuple(From const& from) {
    return get_struct_to_tuple_helper<From>()(from);
}

// Implementation from Compiler Explorer: https://godbolt.org/z/dn58s5Pvz
namespace __impl {
    template<auto... vals>
    struct replicator_type {
        template<typename F>
          constexpr auto operator>>(F body) const -> decltype(auto) {
            return body.template operator()<vals...>();
        }
    };

    template<auto... vals>
    replicator_type<vals...> replicator = {};
}

template<typename R>
consteval auto expand_all(R range) {
    std::vector<std::meta::info> args;
    for (auto r : range) {
        args.push_back(reflect_constant(r));
    }
    return substitute(^^__impl::replicator, args);
}

template <typename T>
constexpr auto struct_to_tuple_compiler_explorer(T const& t) {
    constexpr auto ctx = std::meta::access_context::current();
    return [: expand_all(nonstatic_data_members_of(^^T, ctx)) :] >> [&]<auto... members>{
        return std::make_tuple(t.[:members:]...);
    };
}
// 3.14 Converting a Struct to a Tuple. End

// 3.15 Implementing tuple_cat (tuple concatenation)
template<std::pair<std::size_t, std::size_t>... indices>
struct Indexer {
    template<typename Tuples>
    // Can use tuple indexing instead of tuple of tuples
    auto operator()(Tuples&& tuples) const {
        using ResultType = std::tuple<
          std::tuple_element_t<
            indices.second,
            std::remove_cvref_t<std::tuple_element_t<indices.first, std::remove_cvref_t<Tuples>>>
          >...
        >;
        return ResultType(std::get<indices.second>(std::get<indices.first>(std::forward<Tuples>(tuples)))...);
    }
};

template <class T>
consteval auto subst_by_value(std::meta::info tmpl, std::vector<T> args)
    -> std::meta::info
{
    std::vector<std::meta::info> a2;
    for (T x : args) {
        a2.push_back(std::meta::reflect_constant(x));
    }

    return substitute(tmpl, a2);
}

consteval auto make_indexer(std::vector<std::size_t> sizes)
    -> std::meta::info
{
    std::vector<std::pair<int, int>> args;

    for (std::size_t tidx = 0; tidx < sizes.size(); ++tidx) {
        for (std::size_t eidx = 0; eidx < sizes[tidx]; ++eidx) {
            args.push_back({tidx, eidx});
        }
    }

    return subst_by_value(^^Indexer, args);
}

template<typename... Tuples>
auto my_tuple_cat(Tuples&&... tuples) {
    constexpr typename [: make_indexer({tuple_size(remove_cvref(^^Tuples))...}) :] indexer;
    return indexer(std::forward_as_tuple(std::forward<Tuples>(tuples)...));
}
// 3.15 Implementing tuple_cat (tuple concatenation). End

// 3.16 Named Tuple
template <size_t N>
struct fixed_string {
    char data[N];

    constexpr fixed_string(char const (&s)[N]) {
        std::copy(s, s+N, data);
    }

    constexpr auto view() const -> std::string_view { return data; }
};

template <class T, fixed_string Name>
struct pair {
    static constexpr auto name() -> std::string_view { return Name.view(); }
    using type = T;
};

template <class... Tags>
consteval auto make_named_tuple(std::meta::info type, Tags... tags) {
    std::vector<std::meta::info> nsdms;
    auto f = [&]<class Tag>(Tag tag){
        nsdms.push_back(data_member_spec(
            dealias(^^typename Tag::type),
            {.name=Tag::name()}));

    };
    (f(tags), ...);
    return define_aggregate(type, nsdms);
}

// Alternatively, can side-step the question of non-type template parameters entirely by keeping everything in the value domain:
consteval auto make_named_tuple_value_domain(std::meta::info type,
                                std::initializer_list<std::pair<std::meta::info, std::string_view>> members) {
    std::vector<std::meta::info> nsdms;
    for (auto [member_type, member_name] : members) {
        nsdms.push_back(data_member_spec(member_type, {.name=member_name}));
    }
    return define_aggregate(type, nsdms);
}
// 3.16 Named Tuple. End

// 3.17 Compile-Time Ticket Counter
template<int N> struct Helper;

struct TU_Ticket {
    static consteval int latest() {
        int k = 0;
        while (is_complete_type(
                substitute(^^Helper,
                    { std::meta::reflect_constant(k) })))
            ++k;
        return k;
    }

    static consteval void increment() {
        define_aggregate(
            substitute(^^Helper,
                { std::meta::reflect_constant(latest())}),
            {});
    }
};

constexpr int x = TU_Ticket::latest();  // x initialized to 0.

consteval { TU_Ticket::increment(); }
constexpr int y = TU_Ticket::latest();  // y initialized to 1.

consteval { TU_Ticket::increment(); }
constexpr int z = TU_Ticket::latest();  // z initialized to 2.
// 3.17 Compile-Time Ticket Counter. End

int main(int argc, const char *argv[]) {
    std::cout << "P2996 examples\n";

    // 3.1 Back-And-Forth
    {
        std::cout << "3.1 Back-And-Forth\n";
        constexpr auto r = ^^int;
        typename[:r:] x = 42;       // Same as: int x = 42;
        static_assert(std::same_as<decltype(x), int>);
        assert(x == 42);
        std::cout << "\treflected x: " << x << '\n';

        typename[:^^char:] c = '*';  // Same as: char c = '*';
        static_assert(std::same_as<decltype(c), char>);
        assert(c == '*');
        std::cout << "\treflected c: " << c << '\n';

        using MyType = [:sizeof(int)<sizeof(long)? ^^long : ^^int:];  // Implicit "typename" prefix.
        MyType i;
        constexpr auto typeMyType = std::meta::type_of(^^i);
        constexpr auto nameMyType = std::meta::display_string_of(typeMyType);
        std::cout << "\tMyType: " << nameMyType << "\n";
    }

    // 3.2 Selecting Members
    {
        std::cout << "3.2 Selecting Members\n";
        S s{0, 0};
        s.[:member_number(1):] = 42;  // Same as: s.j = 42;
        assert(s.j == 42);

        // line below gives
        // error: splice operand must be a constant expression
        // because member_number(5) is not defined
        // s.[:member_number(5):] = 0;   // Error (member_number(5) is not a constant).
    }
    {
        S s{0, 0};
        s.[:member_number_1(1):] = 42;  // Same as: s.j = 42;
        assert(s.j == 42);
        // s.[:member_number_1(5):] = 0;   // Error (member_number(5) is not a constant).
    }
    {
        S s{0, 0};
        s.[:member_named("j"):] = 42;  // Same as: s.j = 42;
        assert(s.j == 42);
        // s.[:member_named("x"):] = 0;   // Error (member_named("x") is not a constant).
    }

    // 3.3 List of Types to List of Sizes
    { // reflection-based
        std::cout << "3.3 List of Types to List of Sizes, reflection-based\n";
        constexpr std::array types = {^^int, ^^float, ^^double};
        constexpr std::array sizes = [&]{
            std::array<std::size_t, types.size()> r;
            std::ranges::transform(types, r.begin(), std::meta::size_of);
            return r;
        }();
        static_assert(sizes[0] == sizeof(int));
        static_assert(sizes[1] == sizeof(float));
        static_assert(sizes[2] == sizeof(double));
        std::cout << "\tList of Types to List of Sizes using reflection\n\t";
        for(auto x : sizes) std::cout << x << " ";
        std::cout << "\n";
    }
    { // template-based
        std::cout << "3.3 List of Types to List of Sizes, template-based\n";
        using types = list_of_types<int, float, double>;

        constexpr auto sizes = []<template<class...> class L, class... T>(L<T...>) {
            return std::array<std::size_t, sizeof...(T)>{{ sizeof(T)... }};
        }(types{});
        static_assert(sizes[0] == sizeof(int));
        static_assert(sizes[1] == sizeof(float));
        static_assert(sizes[2] == sizeof(double));
        std::cout << "\tList of Types to List of Sizes using templates\n\t";
        for(auto x : sizes) std::cout << x << " ";
        std::cout << "\n";
    }

    // 3.4 Implementing make_integer_sequence
    {
        std::cout << "3.4 Implementing make_integer_sequence\n";
        static_assert(std::same_as<
            make_integer_sequence<int, 10>,
            std::make_integer_sequence<int, 10>
            >);

        // using integer_sequence_10 = make_integer_sequence<int, 10>;
        // integer_sequence_10 seq;
        make_integer_sequence<int, 10> seq;
        std::cout << "\tinteger sequence size: " << seq.size() << "\n";

        constexpr auto typeSeq = std::meta::type_of(^^seq);
        constexpr auto nameSeq = std::meta::display_string_of(typeSeq);
        std::cout << "\ttype of integer sequence: " << nameSeq << "\n";

        std::cout << "\tinteger sequence: ";
        print_seq(seq);
    }

    // 3.5 Getting Class Layout
    {
        std::cout << "3.5 Getting Class Layout\n";
        struct X
        {
            char a;
            int b;
            double c;
        };

        // /*constexpr*/ auto Xd = get_layout<X>();
        constexpr auto Xd = get_layout<X>();

        /*
        where Xd would be std::array<member_descriptor, 3>{{
          { 0, 1 }, { 4, 4 }, { 8, 8 }
        }}
        */
        static_assert(Xd.size() == 3);
        static_assert(Xd[0] == member_descriptor{.offset=0, .size=1});
        static_assert(Xd[1] == member_descriptor{.offset=4, .size=4});
        static_assert(Xd[2] == member_descriptor{.offset=8, .size=8});

        std::cout << "\tClass Layout\n\t";
        for(auto x : Xd) std::cout << "{" << x.offset << ", " << x.size << "}" << ", ";
        std::cout << "\n";
    }

    // 3.6 Enum to String
    {
        std::cout << "3.6 Enum to String\n";
        enum Color : int;

        // Example enum_to_string using expansion statements
        static_assert(enum_to_string(Color(0)) == "<unnamed>");
        std::println("\tColor 0: {}", enum_to_string(Color(0)));  // prints '<unnamed>'

        enum Color : int { red, green, blue };
        static_assert(enum_to_string(Color::red) == "red");
        static_assert(enum_to_string(Color(42)) == "<unnamed>");
        std::println("\tColor 0: {}", enum_to_string(Color(0)));  // prints 'red'

        constexpr std::optional<Color> c = string_to_enum<Color>("red");
        static_assert(c == Color::red);
        std::println("\tColor: {}", enum_to_string(c.value()));  // prints 'red'

        // Example enum_to_string using array and find_if
        constexpr auto array_find_if_red = enum_to_string_array_find_if(Color::red);
        std::println("\tarray_find_if_red: {}", array_find_if_red);  // prints 'red'
        constexpr auto array_find_if_42 = enum_to_string_array_find_if(Color(42));
        std::println("\tarray_find_if_42: {}", array_find_if_42);  // prints '<unnamed>'
        static_assert(enum_to_string_array_find_if(Color::red) == "red");
        static_assert(enum_to_string_array_find_if(Color(42)) == "<unnamed>");

        // Example to use a compile-time map
        constexpr auto map_find_red = enum_to_string_map_find(Color::red);
        std::println("\tmap_find_red: {}", map_find_red);  // prints 'red'
        constexpr auto map_find_42 = enum_to_string_map_find(Color(42));
        std::println("\tmap_find_42: {}", map_find_42);  // prints '<unnamed>'
        static_assert(enum_to_string_map_find(Color::red) == "red");
        static_assert(enum_to_string_map_find(Color(42)) == "<unnamed>");

        // Combining enum_to_string_array_find_if and enum_to_string_map_find
        constexpr auto alg_find_red = enum_to_string_alg(Color::red);
        std::println("\talg_find_red: {}", alg_find_red);  // prints 'red'
        constexpr auto alg_find_42 = enum_to_string_alg(Color(42));
        std::println("\talg_find_42: {}", alg_find_42);  // prints '<unnamed>'
        static_assert(enum_to_string_alg(Color::red) == "red");
        static_assert(enum_to_string_alg(Color(42)) == "<unnamed>");
    }

    // 3.7 Parsing Command-Line Options
    {
        std::cout << "3.7 Parsing Command-Line Options\n";
        MyOpts opts = parse_options<MyOpts>(std::vector<std::string_view>(argv+1, argv+argc));
        std::cout << "\topts.file=" << opts.file_name << '\n';
        std::cout << "\topts.count=" << opts.count << '\n';
    }

    // 3.8 A Simple Tuple Type
    {
        std::cout << "3.8 A Simple Tuple Type\n";
        auto [x, y, z] = Tuple{1, 'c', 3.14};
        assert(x == 1);
        assert(y == 'c');
        assert(z == 3.14);
    }

    // 3.9 A Simple Variant Type
    {
        // requires boost/mp11.hpp thus omitted
    }

    // 3.10 Struct to Struct of Arrays
    {
        std::cout << "3.10 Struct to Struct of Arrays\n";
        struct point {
            float x;
            float y;
            float z;
        };

        using points = struct_of_arrays<point, 2>;

        points p = {
            .x={1.1, 2.2},
            .y={3.3, 4.4},
            .z={5.5, 6.6}
        };
        static_assert(p.x.size() == 2);
        static_assert(p.y.size() == 2);
        static_assert(p.z.size() == 2);

        for (size_t i = 0; i != 2; ++i) {
            std::cout << "\tp[" << i << "] = (" << p.x[i] << ", " << p.y[i] << ", " << p.z[i] << ")\n";
        }
    }

    // 3.11 Parsing Command-Line Options II
    {
        std::cout << "3.11 Parsing Command-Line Options II\n";
        using namespace clap;
        struct Args : Clap {
            Option<std::string, Flags{.use_short=true, .use_long=true}> name;
            Option<int, Flags{.use_short=true, .use_long=true}> count = 1;
        };
        auto opts = Args{}.parse(argc, argv);

        for (int i = 0; i < opts.count; ++i) {         // opts.count has type int
            std::cout << "\tHello " << opts.name << "!\n"; // opts.name has type std::string
        }
    }

    // 3.12 A Universal Formatter
    {
        std::cout << "3.12 A Universal Formatter\n";
        std::println("\t{}", Z());
        // Z{X{B{.m0=0}, .m1=1}, Y{B{.m0=0}, .m2=2}, .s=(unnamed-type){}, .m3=3, .m4=4}
    }

    // 3.13 Implementing member-wise hash_append
    {
        std::cout << "3.13 Implementing member-wise hash_append\n";
        struct Record {
            std::string name;
            int         count;
            double      price;
        };
        Record r1{"widget", 3, 19.99};
        Record r2{"widget", 3, 19.99};
        Record r3{"widget", 4, 19.99};
        std::size_t h1 = hash_value(r1);
        std::size_t h2 = hash_value(r2);
        std::size_t h3 = hash_value(r3);

        std::cout << "\tr1 == r2? " << std::boolalpha << (h1 == h2) << " ("
                  << h1 << " vs " << h2 << ")\n";
        std::cout << "\tr1 != r3? " << std::boolalpha << (h1 != h3) << " ("
                  << h1 << " vs " << h3 << ")\n";
    }

    // 3.14 Converting a Struct to a Tuple
    {
        std::cout << "3.14 Converting a Struct to a Tuple\n";
        struct Record {
            std::string name;
            int         count;
            double      price;
        };
        Record r{"widget", 3, 19.99};
        auto t = struct_to_tuple(r);
        std::cout << "\tr: {" << r.name << ", " << r.count << ", " << r.price << "}\n";
        std::cout << "\tt: {" << get<0>(t) << ", " << get<1>(t) << ", " << get<2>(t) << "}\n";

        // Implementation from Compiler Explorer
        struct S {
            int x;
            char y;
            std::string z;
        };
        auto parts = struct_to_tuple_compiler_explorer(S{.x=1, .y='x', .z="hello"});
        static_assert(std::same_as<decltype(parts), std::tuple<int, char, std::string>>);
        assert(std::get<0>(parts) == 1);
        assert(std::get<1>(parts) == 'x');
        assert(std::get<2>(parts) == "hello");
    }

    // 3.15 Implementing tuple_cat (tuple concatenation)
    {
        std::cout << "3.15 Implementing tuple_cat (tuple concatenation)\n";
        int r = 37;
        auto x = my_tuple_cat(std::make_tuple(10, std::ref(r)), std::make_tuple(21.0, 22, 23, 24));
        static_assert(std::same_as<decltype(x), std::tuple<int, int&, double, int, int, int>>);
        assert(std::get<0>(x) == 10);
        assert(std::get<1>(x) == r);
        assert(std::get<2>(x) == 21.0);
        assert(std::get<3>(x) == 22);
        assert(std::get<4>(x) == 23);
        assert(std::get<5>(x) == 24);
        std::cout << "\tx: {" << get<0>(x) << ", " << get<1>(x) << ", " << get<2>(x) << ", "
            << get<3>(x) << ", " << get<4>(x) << ", " << get<5>(x) << "}\n";
    }

    // 3.16 Named Tuple
    {
        std::cout << "3.16 Named Tuple\n";
        struct R;
        consteval {
            make_named_tuple(^^R, pair<int, "x">{}, pair<double, "y">{});
        }

        constexpr auto ctx = std::meta::access_context::current();
        static_assert(type_of(nonstatic_data_members_of(^^R, ctx)[0]) == ^^int);
        static_assert(type_of(nonstatic_data_members_of(^^R, ctx)[1]) == ^^double);

        auto r = R{.x=1, .y=2.3};
        std::cout << "\tr: {" << r.x << ", " << r.y << "}\n";

        // Alternatively, can side-step the question of non-type template parameters entirely by keeping everything in the value domain:
        struct R_value_domain;
        consteval {
            make_named_tuple_value_domain(^^R_value_domain, {{^^int, "x"}, {^^double, "y"}});
        }

        constexpr auto ctx_value_domain = std::meta::access_context::current();
        static_assert(type_of(nonstatic_data_members_of(^^R_value_domain, ctx_value_domain)[0]) == ^^int);
        static_assert(type_of(nonstatic_data_members_of(^^R_value_domain, ctx_value_domain)[1]) == ^^double);

        auto r_value_domain = R_value_domain{.x=11, .y=12.3};
        std::cout << "\tr_value_domain: {" << r_value_domain.x << ", " << r_value_domain.y << "}\n";
    }

    // 3.17 Compile-Time Ticket Counter
    {
        std::cout << "3.17 Compile-Time Ticket Counter\n";

        static_assert(x == 0);
        static_assert(y == 1);
        static_assert(z == 2);

        std::cout << "\tx = " << x << ", " << "y = " << y << ", " << "z = " << z << '\n';
    }

    return 0;
}
