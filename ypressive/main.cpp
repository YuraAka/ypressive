#include <iostream>
#include <memory>
#include <vector>
#include <set>
#include <string>

/*
    ctx.Search().Offers(ShopId, CMagicId, OfferId).Group(ShopId, OfferId)
        .Where(Text.Contains("hello") && OfferId.OneOf(1,2,3));
        
    ctx.Search().Clusters(Id, Title).Group(Id)
        .Where(
            Text.Contains(cgi.Text) 
            && Params.Match(cgi.GLFilters)
            && Region.OneOf(cgi.Rids)
            && Category.OneOf(cgi.hid)
        );
        
    ctx.Search().Models(Id, Title, Picture).Group(Id)
    
    result = ctx.Execute();
    
    /// if not searched, do not show
    range = result.Give().Clusters.GroupedBy.Id
    for (const auto& cluster : range) {
        cluster.Id
        cluster.Title
        /// no more then ask
    }
    
    /// by text
    /// by category
    /// by region
    /// by glfilters

*/

/// TQueryBackend
struct TSearchImpl {
    void requestAttr(const char* attr) { std::cout << "Request attr: " << attr << std::endl; }
    void requestGroup(const char* group, size_t docsToFetch, size_t groupsToFetch) {
        std::cout << "Group by " << group << ", fetch groups:" << groupsToFetch;
        std::cout << ", docs:" << docsToFetch << std::endl;
    }

    int readProperty(const char* group, size_t groupIdx, const char* attr, size_t docIdx) {
        std::cout << "Read property " << attr << " from doc(" << docIdx;
        std::cout << ") in " << group << "[" << groupIdx << "]" << std::endl;
        return 0;
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

    void addCondition(const char* attr, const char* val) {
        std::cout << "Filter " << attr << " = " << val << std::endl;
    }

    void addLogicOr() {
        std::cout << "OR" << std::endl;
    }

    void addLogicAnd() {
        std::cout << "AND" << std::endl;
    }

public:
    void search() { std::cout << "Perform search" << std::endl; }
};

struct TConditionContains {
    void Apply(TSearchImpl* ) const {}
};

struct TConditionMatch {
    void Apply(TSearchImpl*) const {
        //impl.sp.setgl(Filter);
    }

    const char* Filter;
};

struct TConditionEq {
    void Apply(TSearchImpl* impl) const {
        impl->addCondition(Attr, Val);
    }

    TConditionEq(const char* attr, const char* val) : Attr(attr), Val(val) {}
    const char* Attr = nullptr;
    const char* Val = nullptr;
};

template <typename TCondition1, typename TCondition2>
struct TConditionOr {
    void Apply(TSearchImpl* impl) const {
        Lhs.Apply(impl);
        impl->addLogicOr();
        Rhs.Apply(impl);
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
    void Apply(TSearchImpl* impl) const {
        Lhs.Apply(impl);
        impl->addLogicAnd();
        Rhs.Apply(impl);
    }

    TConditionAnd(TCondition1 lhs, TCondition2 rhs)
        : Lhs(lhs)
        , Rhs(rhs)
    {
    }

    TCondition1 Lhs;
    TCondition2 Rhs;
};

template <typename TAttr>
struct TCanFilterValue {
    static TConditionEq Equal(const char* val) {
        return {TAttr::Name, val};
    }
};

template <typename TAttr>
struct TCanFilterList {
    static TConditionContains Contains(const char* ) { return {};}
};

template <typename TAttr>
struct TCanFilterSet {
    static TConditionContains OneOf(std::set<std::string> ) { return {};}
};

template <typename TAttr>
struct TCanFilterMatch {
    static TConditionMatch Match(const char* filter) {
        return {filter};
    }
};

template <typename TAttr>
struct TCanRequest {
    static void Request(TSearchImpl* impl) { impl->requestAttr(TAttr::Name); }
};

template <typename TAttr>
struct TCanGroup {
    static void Group(TSearchImpl* impl, size_t docsToFetch, size_t groupsToFetch) {
        impl->requestGroup(TAttr::Name, docsToFetch, groupsToFetch);
    }
};

/// compaund select, group and filter abilities of attribute
template <typename TAttr, template <typename> class... TTraits>
struct TAttrTraits : public TTraits<TAttr>... {
    using Type = TAttr;
};

/// ytodo check virtual inheritance and default ctor
struct TProperty {
    TProperty(TSearchImpl* impl, const char* group, size_t groupIdx, size_t docIdx)
        : Impl(impl)
        , GroupName(group)
        , GroupIdx(groupIdx)
        , DocIdx(docIdx)
    {}

    TProperty() = default;

    TSearchImpl* Impl = nullptr;
    const char* GroupName = nullptr;
    size_t GroupIdx = 0;
    size_t DocIdx = 0;


    int read(const char* name) const {
        return Impl->readProperty(GroupName, GroupIdx, name, DocIdx);
    }
};

struct TOfferId {
    constexpr static const char* Name = "offer_id";

    struct TReader : private virtual TProperty {
        int OfferId() const { return read(Name); }
    };
};

/// todo get rid on methods
struct TShopId {
    constexpr static const char* Name = "shop_id";

    struct TReader : private virtual TProperty {
        int ShopId() const { return read(Name); }
    };
};

struct TTitle {
    constexpr static const char* Name = "title";

    struct TReader : private virtual TProperty {
        int Title() const { return read(Name); }
    };
};

struct TSalesFlag {
    constexpr static const char* Name = "sales";
};

struct TRegion {
    constexpr static const char* Name = "region";
};

// relevance time check
struct TGLParams {
    constexpr static const char* Name = "glparams";
};

struct TMagicId {
    constexpr static const char* Name = "magic_id";

    struct TReader : private virtual TProperty {
        int MagicId() const { return read(Name); }
    };
};

static TAttrTraits<TShopId, TCanRequest, TCanGroup, TCanFilterValue> ShopId;
static TAttrTraits<TOfferId, TCanRequest, TCanGroup> OfferId;
static TAttrTraits<TMagicId, TCanRequest, TCanGroup> MagicId;
static TAttrTraits<TTitle, TCanRequest> Title;
static TAttrTraits<TSalesFlag, TCanFilterValue> SalesFlag;
static TAttrTraits<TRegion, TCanFilterSet, TCanFilterValue> Region;
static TAttrTraits<TGLParams, TCanFilterMatch> GLParams;

template <typename TComparable>
TConditionEq operator==(TComparable, const char* val) {
    /// ytodo check for compliance
    return TComparable::Equal(val);
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
    TSearchServiceBase(TSearchImpl* impl) : Impl(impl) {}
    TSearchImpl* Impl;
};

template <typename TRequestAttrs>
struct TDocAccessor : public TRequestAttrs {
    TDocAccessor(TSearchImpl* impl, const char* name, size_t groupIdx, size_t docIdx)
        : TProperty(impl, name, groupIdx, docIdx) {}
};

template <typename TRequestAttrs>
struct TDocIterator {
    using TElement = TDocAccessor<TRequestAttrs>;
    using TThis = TDocIterator<TRequestAttrs>;

    TDocIterator(TSearchImpl* impl, const char* groupName, size_t groupIdx, size_t start)
        : Impl(impl)
        , Current(impl, groupName, groupIdx, start)
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
    TSearchImpl* Impl;
    TElement Current;
    size_t Counter = 0;
    const char* GroupName = nullptr;
    size_t GroupIdx = 0;
};

template <typename TRequestAttrs>
struct TDocRange {
    using TIterator = TDocIterator<TRequestAttrs>;

    TDocRange(TSearchImpl* impl, const char* groupName, size_t groupIdx)
        : Begin(impl, groupName, groupIdx, 0)
        , End(impl, groupName, groupIdx, impl->totalDocs(groupName, groupIdx))
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

    TDocRangeIterator(TSearchImpl* impl, const char* groupName, size_t start)
        : Impl(impl)
        , Current(impl, groupName, start)
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

    TSearchImpl* Impl;
    TElement Current;
    size_t Counter = 0;
    const char* GroupName = nullptr;
};

template <typename TRequestAttrs>
struct TGroupRange {
    using TIterator = TDocRangeIterator<TRequestAttrs>;

    TGroupRange(TSearchImpl* impl, const char* groupName)
        : GroupsCount(impl->totalGroups(groupName))
        , Begin(impl, groupName, 0)
        , End(impl, groupName, GroupsCount)
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

template <typename TRequestAttrs, typename TGroupAttrs>
struct TSearchResult {
    TSearchResult(TSearchImpl* impl) : Impl(impl) {
    }

    /// why do we need TDocAccessor?
    template <typename TGroup>
    TGroupRange<TRequestAttrs> Group(TGroup) {
        /// is parent of trait
        TGroupAttrs* fake;
        (void)static_cast<TGroup*>(fake);

        return TGroupRange<TRequestAttrs>(Impl, TGroup::Type::Name);
    }

    TSearchImpl* Impl;
};

template <typename TParent, typename TAttr, typename TRequestAttrs>
struct TGroupAttr : public TParent, public TAttr {
    using TThis = TGroupAttr<TParent, TAttr, TRequestAttrs>;

    TGroupAttr(TSearchImpl* impl) : TParent(impl) {}

    template <typename TGroup>
    TGroupAttr<TThis, TGroup, TRequestAttrs> By(TGroup, size_t docsToFetch, size_t groupsToFetch = 1) {
        TGroup::Group(TSearchServiceBase::Impl, docsToFetch, groupsToFetch);
        return {TSearchServiceBase::Impl};
    }

    template <typename TCondition>
    TSearchResult<TRequestAttrs, TThis> Where(TCondition cond) {
        cond.Apply(TSearchServiceBase::Impl);
        return {TSearchServiceBase::Impl};
    }
};

template <typename TRequestAttrs>
struct TGroupContext {
    TGroupContext(TSearchImpl* impl) : Impl(impl) {}

    template <typename TAttr>
    TGroupAttr<TSearchServiceBase, TAttr, TRequestAttrs> By(TAttr, size_t docsToFetch, size_t groupsToFetch = 1) {
        TAttr::Group(Impl, docsToFetch, groupsToFetch);
        return {Impl};
    }

    TSearchImpl* Impl;
};

template <typename... TArgs>
struct TSelectTypes : public TArgs... {
};

/// rename
template <typename... TArgs>
struct TSelectNames {
    using TThisTypes = TSelectTypes<typename TArgs::Type::TReader...>;

    TSelectNames(TSearchImpl* impl) : Impl(impl) {}

    static void Apply(TSearchImpl* impl) {
        [=](...){ }((TArgs::Request(impl), 0)...);  /// make nonstatic: how to invoke base method with dup name?
    }

    TGroupContext<TThisTypes> Group() {
        Apply(Impl);
        return {Impl};
    }

    TSearchImpl* Impl;
};

struct TSelectContext {
    /// make a request with attrs dependent
    template <typename... TArgs>
    TSelectNames<TArgs...> Offers(TArgs...) {
        return {&Impl}; /// pass impl to ctor
    }

    template <typename... TArgs>
    TSelectNames<TArgs...> Models(TArgs...) {
        return {&Impl}; /// pass impl to ctor
    }

    int Clusters;

public:
    TSelectContext(TSearchImpl& impl) : Impl(impl) {}

private:
    TSearchImpl& Impl;
};

struct TSearchSession {
    TSelectContext Select() {
        return {Impl};
    }

    void FetchResult() {
        Impl.search();
    }

    TSearchImpl Impl;
};

/// todo:
/// return document iterator
/// add groupping with cache
/// optimize query tree

int main(int argc, const char * argv[])
{
    const auto rids = std::set<std::string>{"213", "225"};
    const char* glFilter = "1234:345";
    TSearchSession ctx;
    auto offers = ctx
        .Select()
            .Offers(OfferId, ShopId, Title)
        .Group()
            .By(MagicId, 10, 2)
            .By(OfferId, 3)
        .Where(
            (SalesFlag == "1" || SalesFlag == "2")
            && ShopId == "123"
            && (Region.OneOf(rids) || Region == "213")
            && GLParams.Match(glFilter)
        );

    auto models = ctx
        .Select()
            .Models(ShopId)
        .Group()
            .By(MagicId, 1, 1)
        .Where(ShopId == "123");

    ctx.FetchResult();

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

