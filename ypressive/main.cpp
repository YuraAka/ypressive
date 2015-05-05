#include <iostream>
#include <memory>
#include <vector>
#include <set>
#include <string>

struct TQueryBackend {
    void requestAttr(const char* attr) { std::cout << "Request attr: " << attr << std::endl; }
    void requestGroup(const char* group, size_t docsToFetch, size_t groupsToFetch) {
        std::cout << "Group by " << group << ", fetch groups:" << groupsToFetch;
        std::cout << ", docs:" << docsToFetch << std::endl;
    }

    const char* readProperty(const char* group, size_t groupIdx, const char* attr, size_t docIdx, const char* col) {
        std::cout << "Read " << col << "::doc[" << docIdx << "]." << attr << " group=";
        std::cout << group << "[" << groupIdx << "]" << std::endl;
        return "0";
    }

    // ytodo default behaviour -- return only prefetched
    size_t totalDocs(const char* groupName, size_t groupIdx) {
        (void)groupName;
        (void)groupIdx;
        return 2;
    }

    size_t totalGroups(const char* groupName) {
        (void)groupName;
        return 1;
    }

    void addCondition(const char* attr, const char* val, const char* col) {
        std::cout << "Filter " << attr << ":" << col << " = " << val << std::endl;
    }

    void addLogicOr(const char* col) {
        std::cout << col << ":OR" << std::endl;
    }

    void addLogicAnd(const char* col) {
        std::cout << col << ":AND" << std::endl;
    }

public:
    void search() { std::cout << "Perform search" << std::endl; }
};

struct TProperty {
    TProperty(TQueryBackend* backend, const char* group, size_t groupIdx, size_t docIdx)
        : Backend(backend)
        , GroupName(group)
        , GroupIdx(groupIdx)
        , DocIdx(docIdx)
    {}

    TProperty() = default;

    TQueryBackend* Backend = nullptr;
    const char* GroupName = nullptr;
    size_t GroupIdx = 0;
    size_t DocIdx = 0;

    const char* read(const char* name, const char* collection) const {
        return Backend->readProperty(GroupName, GroupIdx, name, DocIdx, collection);
    }
};

#define DECLARE_FIELD(NAME) static struct T ## NAME {} NAME;

#define COLLECTION_BEGIN(NAME, TEXT)                                                    \
namespace N ## NAME ## Detail {                                                         \
    struct T ## NAME {                                                                  \
        static constexpr const char* Name = TEXT;                                       \
                                                                                        \
        template <typename TField>                                                      \
        struct TLink;                                                                   \
    };                                                                                  \
}                                                                                       \
static N ## NAME ## Detail::T ## NAME NAME;                                             \
namespace N ## NAME ## Detail {                                                         \
    using TCol = T ## NAME;

#define COLLECTION_END() }

#define FIELD_BEGIN(NAME)                                                               \
template<>                                                                              \
struct TCol::TLink<T ## NAME> {

#define FIELD_END() };
#define FIELD_NAME(NAME) static constexpr const char* Name = NAME;
#define FIELD_READER(NAME)                                                              \
struct TReader : private virtual TProperty {                                            \
    const char* NAME () const { return read(Name, TCol::Name); }                        \
};                                                                                  

#define FIELD_REQUEST() \
static void Request(TQueryBackend* backend) {                                           \
    backend->requestAttr(Name);                                                         \
}                                                                                       \

#define FIELD_GROUP() \
static void Group(TQueryBackend* backend, size_t docsToFetch, size_t groupsToFetch) {   \
    backend->requestGroup(Name, docsToFetch, groupsToFetch);                            \
}                                                                                   

#define FIELD_FILTER() \
static void Filter(TQueryBackend* backend, const char* value) {                         \
    backend->addCondition(Name, value, TCol::Name);                                     \
}

#define GROUPPING_ATTR(NAME, TEXT)                                                      \
    FIELD_BEGIN(NAME)                                                                   \
        FIELD_NAME(TEXT)                                                                \
        FIELD_READER(NAME)                                                              \
        FIELD_REQUEST()                                                                 \
        FIELD_GROUP()                                                                   \
        FIELD_FILTER()                                                                  \
    FIELD_END()

#define SEARCH_LITERAL(NAME, TEXT)                                                      \
    FIELD_BEGIN(NAME)                                                                   \
        FIELD_NAME(TEXT)                                                                \
        FIELD_READER(NAME)                                                              \
        FIELD_REQUEST()                                                                 \
        FIELD_FILTER()                                                                  \
    FIELD_END()

#define PLAIN_PROPERTY(NAME, TEXT)                                                      \
    FIELD_BEGIN(NAME)                                                                   \
        FIELD_NAME(TEXT)                                                                \
        FIELD_READER(NAME)                                                              \
        FIELD_REQUEST()                                                                 \
    FIELD_END()

DECLARE_FIELD(OfferId)
DECLARE_FIELD(ShopId)
DECLARE_FIELD(Title)
DECLARE_FIELD(SalesFlag)
DECLARE_FIELD(Region)
DECLARE_FIELD(MagicId)

COLLECTION_BEGIN(Offers, "SHOP")
    GROUPPING_ATTR(OfferId, "offer_id")
    PLAIN_PROPERTY(Title, "title")
    SEARCH_LITERAL(SalesFlag, "sales")
    SEARCH_LITERAL(ShopId, "shop_id")
    SEARCH_LITERAL(Region, "region")
    GROUPPING_ATTR(MagicId, "magic_id")
COLLECTION_END()

COLLECTION_BEGIN(Models, "MODEL")
    SEARCH_LITERAL(ShopId, "shop_id")
    GROUPPING_ATTR(MagicId, "magic_id")
COLLECTION_END()

/// end of custom schema

template <typename TCondition1, typename TCondition2>
struct TConditionOr {
    template <typename TCollection>
    void Apply(TQueryBackend* backend) const {
        Lhs.template Apply<TCollection>(backend);
        backend->addLogicOr(TCollection::Name);
        Rhs.template Apply<TCollection>(backend);
    }

    TConditionOr(TCondition1 lhs, TCondition2 rhs)
        : Lhs(lhs)
        , Rhs(rhs)
    {
    }

    TCondition1 Lhs;
    TCondition2 Rhs;
};

template <typename TCondition1, typename TCondition2>
struct TConditionAnd {
    template <typename TCollection>
    void Apply(TQueryBackend* backend) const {
        Lhs.template Apply<TCollection>(backend);
        backend->addLogicAnd(TCollection::Name);
        Rhs.template Apply<TCollection>(backend);
    }

    TConditionAnd(TCondition1 lhs, TCondition2 rhs)
        : Lhs(lhs)
        , Rhs(rhs)
    {
    }

    TCondition1 Lhs;
    TCondition2 Rhs;
};

template <typename TField, typename TValue>
struct TConditionField {
    using TKey = TField;

    TConditionField(const TValue& value) : Value(value) {}

    template <typename TCollection>
    void Apply(TQueryBackend* backend) const {
        TCollection::template TLink<TField>::Filter(backend, Value);
    }

    const TValue& Value;
};

template <typename TField, typename TValue>
TConditionField<TField, TValue> operator==(TField, const TValue& value) {
    return {value};
}

template <typename TCondition1, typename TCondition2>
TConditionOr<TCondition1, TCondition2> operator||(TCondition1 lhs, TCondition2 rhs) {
    return {lhs, rhs};
}

template <typename TCondition1, typename TCondition2>
TConditionAnd<TCondition1, TCondition2> operator&&(TCondition1 lhs, TCondition2 rhs) {
    return {lhs, rhs};
}

struct TSearchServiceBase {
    TSearchServiceBase(TQueryBackend* backend) : Impl(backend) {}
    TQueryBackend* Impl;
};

template <typename TRequestAttrs>
struct TDocAccessor : public TRequestAttrs {
    TDocAccessor(TQueryBackend* backend, const char* name, size_t groupIdx, size_t docIdx)
        : TProperty(backend, name, groupIdx, docIdx) {}
};

template <typename TRequestAttrs>
struct TDocIterator {
    using TElement = TDocAccessor<TRequestAttrs>;
    using TThis = TDocIterator<TRequestAttrs>;

    TDocIterator(TQueryBackend* backend, const char* groupName, size_t groupIdx, size_t start)
        : Impl(backend)
        , Current(backend, groupName, groupIdx, start)
        , Counter(start)
        , GroupName(groupName)
        , GroupIdx(groupIdx)
    {
    }

    const TElement& operator*() const {
        return Current;
    }

    const TElement* operator->() const {
        return &Current;
    }

    TDocIterator& operator++() {
        Current = TElement(Impl, GroupName, GroupIdx, ++Counter);
        return *this;
    }

    bool operator!=(const TThis& other) const {
        return Counter != other.Counter;
    }

    bool operator==(const TThis& other) const {
        return Counter == other.Counter;
    }

    /// use optional
    TQueryBackend* Impl;
    TElement Current;
    size_t Counter = 0;
    const char* GroupName = nullptr;
    size_t GroupIdx = 0;
};

template <typename TRequestAttrs>
struct TDocRange {
    using TIterator = TDocIterator<TRequestAttrs>;

    TDocRange(TQueryBackend* backend, const char* groupName, size_t groupIdx)
        : Begin(backend, groupName, groupIdx, 0)
        , End(backend, groupName, groupIdx, backend->totalDocs(groupName, groupIdx))
    {
    }

    TIterator begin() const {
        return Begin;
    }

    TIterator end() const {
        return End;
    }

    TIterator Begin;
    TIterator End;
};

template <typename TRequestAttrs>
struct TDocRangeIterator {
    using TElement = TDocRange<TRequestAttrs>;
    using TThis = TDocRangeIterator<TRequestAttrs>;

    TDocRangeIterator(TQueryBackend* backend, const char* groupName, size_t start)
        : Impl(backend)
        , Current(backend, groupName, start)
        , Counter(start)
        , GroupName(groupName)
    {
    }

    const TElement& operator*() const {
        return Current;
    }

    const TElement* operator->() const {
        return &Current;
    }

    TThis& operator++() {
        Current = TElement(Impl, GroupName, ++Counter);
        return *this;
    }

    bool operator!=(const TThis& other) const {
        return Counter != other.Counter;
    }

    bool operator==(const TThis& other) const {
        return Counter == other.Counter;
    }

    TQueryBackend* Impl;
    TElement Current;
    size_t Counter = 0;
    const char* GroupName = nullptr;
};

template <typename TRequestAttrs>
struct TGroupRange {
    using TIterator = TDocRangeIterator<TRequestAttrs>;

    TGroupRange(TQueryBackend* backend, const char* groupName)
        : GroupsCount(backend->totalGroups(groupName))
        , Begin(backend, groupName, 0)
        , End(backend, groupName, GroupsCount)
    {
    }

    TIterator begin() const{
        return Begin;
    }

    TIterator end() const{
        return End;
    }

    bool Single() const { return GroupsCount == 1;}
    bool Empty() const { return GroupsCount == 0;}
    size_t Size() const { return GroupsCount; }

    size_t GroupsCount = 0;
    TIterator Begin;
    TIterator End;
};

template <typename TRequestAttrs, typename TGroupAttrs, typename TCollection>
struct TSearchResult {
    TSearchResult(TQueryBackend* backend) : Impl(backend) {
    }

    template <typename TGroup>
    TGroupRange<TRequestAttrs> Group(TGroup) {
        /// is parent of trait
        /// check group match requested
        TGroupAttrs* fake;
        (void)static_cast<TGroup*>(fake);

        return TGroupRange<TRequestAttrs>(Impl, TCollection::template TLink<TGroup>::Name);
    }

    TQueryBackend* Impl;
};

/// rename Attr -> Field
template <typename TParent, typename TAttr, typename TCollection, typename TRequestAttrs>
struct TGroupAttr : public TParent, public TAttr {
    using TThis = TGroupAttr<TParent, TAttr, TCollection, TRequestAttrs>;

    TGroupAttr(TQueryBackend* backend) : TParent(backend) {}

    template <typename TGroup>
    TGroupAttr<TThis, TGroup, TCollection, TRequestAttrs> By(TGroup, size_t docsToFetch, size_t groupsToFetch = 1) {
        TCollection::template TLink<TGroup>::Group(TSearchServiceBase::Impl, docsToFetch, groupsToFetch);
        return {TSearchServiceBase::Impl};
    }

    template <typename TCondition>
    TSearchResult<TRequestAttrs, TThis, TCollection> Where(TCondition cond) {
        cond.template Apply<TCollection>(TSearchServiceBase::Impl);
        return {TSearchServiceBase::Impl};
    }
};

template <typename TCollection, typename TRequestAttrs>
struct TGroupContext {
    TGroupContext(TQueryBackend* backend) : Impl(backend) {}

    template <typename TAttr>
    TGroupAttr<TSearchServiceBase, TAttr, TCollection, TRequestAttrs>
    By(TAttr, size_t docsToFetch, size_t groupsToFetch = 1) {
        TCollection::template TLink<TAttr>::Group(Impl, docsToFetch, groupsToFetch);
        return {Impl};
    }

    TQueryBackend* Impl;
};

template <typename... TArgs>
struct TSelectTypes : public TArgs... {
};

/// rename
template <typename TCollection, typename... TArgs>
struct TSelectNames {
    /// todo how to typedef variadic template???
    using TThisTypes = TSelectTypes<typename TCollection::template TLink<TArgs>::TReader...>;

    TSelectNames(TQueryBackend* backend) : Impl(backend) {}

    TGroupContext<TCollection, TThisTypes> Group() {
        [](...){}((TCollection::template TLink<TArgs>::Request(Impl), 0)...);
        return {Impl};
    }

    TQueryBackend* Impl;
};

template <typename TCollection>
struct TSelectContext {
    /// make a request with attrs dependent

    template <typename... TArgs>
    TSelectNames<TCollection, TArgs...> Fields(TArgs...) {
        /// check args from collection
        return {&Impl}; /// pass backend to ctor
    }

public:
    TSelectContext(TQueryBackend& backend) : Impl(backend) {}

private:
    TQueryBackend& Impl;
};

struct TSearchSession {
    template <typename TCollection>
    TSelectContext<TCollection> Select(TCollection) {
        return {Impl};
    }

    void Fetch() {
        Impl.search();
    }

    TQueryBackend Impl;
};

/* todo:
 - passing remaining attrs: many sp
*/

int main(int argc, const char * argv[])
{
    const auto rids = std::set<std::string>{"213", "225"};
    TSearchSession ctx;
    auto offers = ctx
        .Select(Offers)
            .Fields(OfferId, ShopId, Title)
        .Group()
            .By(MagicId, 10, 2)
            .By(OfferId, 3)
        .Where(
            (SalesFlag == "1" || SalesFlag == "2")
            && ShopId == "123"
            && Region == "213"
        );

    auto models = ctx
        .Select(Models)
            .Fields(ShopId)
        .Group()
            .By(MagicId, 1, 1)
        .Where(ShopId == "123");

    ctx.Fetch();

    for (auto group : offers.Group(MagicId)) {
        for (auto offer : group) {
            offer.OfferId();
        }
    }

    auto groups = offers.Group(OfferId);
    if (groups.Single()) {
        for (auto group : *groups.begin()) {
            (void) group;
        }
    }

    models.Group(MagicId).begin()->begin()->ShopId();

    return 0;
}
