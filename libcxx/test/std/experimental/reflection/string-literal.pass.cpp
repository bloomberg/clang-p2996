// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

#include <meta>

constexpr char msg[] = "hello";
constexpr char const* p = "hello";
constexpr char const* q = p + 1;

static_assert(std::is_string_literal("hello"));
static_assert(std::is_string_literal(p));
static_assert(std::is_string_literal(p + 1));
static_assert(std::is_string_literal(q));
static_assert(std::is_string_literal(q + 1));
static_assert(!std::is_string_literal(msg));
static_assert(!std::is_string_literal(msg + 1));

static_assert(std::string_literal_from(p) == p);
static_assert(std::string_literal_from(p + 1) == p);
static_assert(std::string_literal_from(q) == p);
static_assert(std::string_literal_from(q + 1) == p);
static_assert(std::string_literal_from(msg) == nullptr);

int main() { }