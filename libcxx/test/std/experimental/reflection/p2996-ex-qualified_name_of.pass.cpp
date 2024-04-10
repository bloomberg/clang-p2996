#include <experimental/meta>

#include <string>
#include <string_view>

namespace outer_namespace{
namespace a_namespace {
namespace an_inner_namespace
{
struct ClassInsideSomeNamespaces {
    int k;
    int j;
    using type = int;
};

typedef ClassInsideSomeNamespaces ClassInsideSomeNamespacesAlias;

template <typename T>
class SomeTemplateClass
{
    std::vector<T> stuff;
    public:
    SomeTemplateClass(const T& t)
    {
        stuff.push_back(t);
    }
};

class DerivedClass: public ClassInsideSomeNamespaces
{
    int boopTheBloop;
};

const std::string aString = "abcde";
} // an_inner_namespace
} // a_namespace
} // outer_namespace

int main()
{
    using namespace outer_namespace;
    using namespace a_namespace;
    using namespace an_inner_namespace;

    constexpr auto expectedQNofType = "outer_namespace::a_namespace::an_inner_namespace::ClassInsideSomeNamespaces";
    static_assert(expectedQNofType==std::meta::qualified_name_of(^ClassInsideSomeNamespaces)); 
    
    constexpr auto expectedQNofDeclaration = "outer_namespace::a_namespace::an_inner_namespace::aString";
    static_assert(expectedQNofDeclaration==std::meta::qualified_name_of(^aString));

    constexpr auto expectedQNofNamespace = "outer_namespace::a_namespace::an_inner_namespace";
    static_assert(expectedQNofNamespace==std::meta::qualified_name_of(^an_inner_namespace));

    constexpr auto expectedQNofTemplate = "outer_namespace::a_namespace::an_inner_namespace::SomeTemplateClass";
    static_assert(expectedQNofTemplate==std::meta::qualified_name_of(^SomeTemplateClass));    

    constexpr auto expectedQNofTypeAlias = "outer_namespace::a_namespace::an_inner_namespace::ClassInsideSomeNamespacesAlias";
    static_assert(expectedQNofTypeAlias==std::meta::qualified_name_of(^ClassInsideSomeNamespacesAlias));

    constexpr auto expectedQNofDerivedType = "outer_namespace::a_namespace::an_inner_namespace::DerivedClass";
    static_assert(expectedQNofDerivedType==std::meta::qualified_name_of(^DerivedClass));

    constexpr auto expectedQNofBaseSpecifier="outer_namespace::a_namespace::an_inner_namespace::ClassInsideSomeNamespaces";
    constexpr auto base = [:(std::meta::reflect_value(std::meta::bases_of(^DerivedClass)[0])):];    
    static_assert(expectedQNofBaseSpecifier==std::meta::qualified_name_of(base));

    return 0;
}