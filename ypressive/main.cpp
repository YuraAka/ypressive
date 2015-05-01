//
//  main.cpp
//  playground
//
//  Created by Юра on 07.06.14.
//  Copyright (c) 2014 yura. All rights reserved.
//

#include <iostream>
#include <memory>
#include <vector>

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

struct TSearchImpl {
    void requestAttr(const char* attr) { std::cout << "Request attr: " << attr << std::endl; }
    void requestGroup(const char* group, int cache) { std::cout << "Request group (" << cache << "): " << group << std::endl; }

    int readProperty(int idx, const char* group, const char* attr) {
        if (!search_finished) {
            search_finished = true;
            search();
        }

        std::cout << "Read property " << attr << " from doc " << idx << ":" << group << std::endl; return 0;
    }

    int totalDocs(const char* group) {
        return 3;
    }

public:
    void search() { std::cout << "Perform search" << std::endl; }

    bool search_finished = false;
};

struct TProperty {
    TProperty(TSearchImpl* impl = nullptr, const char* group = nullptr) : Impl(impl), GroupName(group) {}
    TSearchImpl* Impl;
    const char* GroupName;

    int read(int idx, const char* name) {
        return Impl->readProperty(idx, GroupName, name);
    }
};

struct TOfferId : public virtual TProperty
{
    static void Request(TSearchImpl* impl) { impl->requestAttr("offer_id");}

    /// have only attrs with support groupping
    static void Group(TSearchImpl* impl, int cache) { impl->requestGroup("offer_id", cache);}

    constexpr static const char* Name = "offer_id";

    /// ytodo support index
    int OfferId() { return read(0, "offer_id"); }
};

struct TShopId : public virtual TProperty {
    static void Request(TSearchImpl* impl) { impl->requestAttr("shop_id"); }
    int ShopId() { return read(0, "shop_id"); }
    constexpr static const char* Name = "shop_id";
};

struct TTitle : public virtual TProperty {
    static void Request(TSearchImpl* impl) { impl->requestAttr("title"); }
    int Title() { return read(0, "title"); }
    constexpr static const char* Name = "title";
};

struct TMagicId : public virtual TProperty {
    static void Request(TSearchImpl* impl) { impl->requestAttr("magic_id"); }
    static void Group(TSearchImpl* impl, int cache) { impl->requestGroup("magic_id", cache);}
    constexpr static const char* Name = "magic_id";

    /// todo separate to another class
    int MagicId() { return read(0, "magic_id"); }
};

template <typename TAttr>
struct TAttrType {
    using Type = TAttr;
};

/// can be ptr, but need remove_pointer treat
static TAttrType<TShopId> ShopId;
static TAttrType<TOfferId> OfferId;
static TAttrType<TMagicId> MagicId;
static TAttrType<TTitle> Title;

struct TGroupBase {
    TGroupBase(TSearchImpl* impl) : Impl(impl) {}
    TSearchImpl* Impl;
};

template <typename TRequestAttrs>
struct TDocAccessor : public TRequestAttrs {
    TDocAccessor(TSearchImpl* impl, const char* name) : TProperty(impl, name) {}

};

template <typename TRequestAttrs, typename TGroupAttrs>
struct TSearchResult {
    TSearchResult(TSearchImpl* impl) : Impl(impl) {
    }

    template <typename TGroup>
    TDocAccessor<TRequestAttrs> Group(TGroup) {
        /// is parent of trait
        TGroupAttrs* fake;
        (void)static_cast<TGroup*>(fake);

        return TDocAccessor<TRequestAttrs>(Impl, TGroup::Type::Name);
    }

    TSearchImpl* Impl;
};

template <typename TParent, typename TAttr, typename TPrev>
struct TGroupAttr : public TParent, public TAttr {
    using TThis = TGroupAttr<TParent, TAttr, TPrev>;

    TGroupAttr(TSearchImpl* impl) : TParent(impl) {}

    template <typename TGroup>
    TGroupAttr<TThis, TGroup, TPrev> By(TGroup, int cache) {
        TGroup::Type::Group(TGroupBase::Impl, cache);
        return {TGroupBase::Impl};
    }

    TSearchResult<TPrev, TThis> End() {
        return {TGroupBase::Impl};
    }
};

template <typename TPrev>
struct TGroupContext {
    TGroupContext(TSearchImpl* impl) : Impl(impl) {}

    template <typename TAttr>
    TGroupAttr<TGroupBase, TAttr, TPrev> By(TAttr, int cache) {
        TAttr::Type::Group(Impl, cache);
        return {Impl};
    }

    TSearchImpl* Impl;
};

template <typename... TArgs>
struct TRequestAttrTypes : public TArgs... {
};

/// rename
template <typename... TArgs>
struct TRequestAttrubutes : public TArgs... {
    using TThis = TRequestAttrubutes<TArgs...>;
    using TTypedThis = TRequestAttrTypes<typename TArgs::Type...>;
    TRequestAttrubutes(TSearchImpl* impl) : Impl(impl) {}

    static void Apply(TSearchImpl* impl) {
        [=](...){ }((TArgs::Type::Request(impl), 0)...);  /// make nonstatic: how to invoke base method with dup name?
    }

    TGroupContext<TTypedThis> Group() {
        Apply(Impl);
        return {Impl};
    }

    TSearchImpl* Impl;
};

struct TSelectContext {
    /// make a request with attrs
    template <typename... TArgs>
    TRequestAttrubutes<TArgs...> Offers(TArgs...) {
        return TRequestAttrubutes<TArgs...>(&Impl); /// pass impl to ctor
    }

    template <typename... TArgs>
    TRequestAttrubutes<TArgs...> Models(TArgs...) {
        return TRequestAttrubutes<TArgs...>(&Impl); /// pass impl to ctor
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

    TSearchImpl Impl;
};

/// todo:
/// return document iterator
/// add groupping with cache
/// optimize query tree

int main(int argc, const char * argv[])
{
    TSearchSession ctx;
    auto result = ctx
        .Select()
            .Offers(OfferId, ShopId, Title)
        .Group()
            .By(MagicId, 1)
            .By(OfferId, 2)
        .End();

    result.Group(OfferId).ShopId();
    result.Group(MagicId).Title();

    /*auto groups = result.Group(MagicId);
    for (auto group : groups) {
        for (auto offer : group) {
            offer.OfferId();
        }
    }
    
    if (groups.Single() || groups.Empty()) {
    }
    */

    return 0;
}

