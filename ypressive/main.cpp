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

class A
{
public:
    A() {}
    ~A() {}

private:
    int i;
};

template <typename T>
class msp
{
public:
    template <typename Y>
    explicit msp(Y* y) // exceptions????
        : ptr(y)
    {
        cb = new Cb;
        cb->count = 1;
        cb->ptr = y;
    }

    ~msp()
    {
        if (--cb->count)
        {
            std::cout << "we don't leak" << std::endl;
            delete cb->ptr;
            delete cb;
        }
    }

    msp(const msp<T>& c)
    {
        cb = c.cb;
        ptr = c.ptr;
        if (cb)
        {
            ++cb->count;
        }
    }

    void swap(msp<T>& o)
    {
        std::swap(ptr, o.ptr);
        std::swap(cb, o.cb);
    }

    msp<T>& operator=(const msp<T>& o)
    {
        msp(o).swap(*this);
        return *this;
    }

    T* operator->() const
    {
        return ptr;
    }

    T& operator*() const
    {
        return *ptr;
    }

private:
    struct Cb
    {
        int count = 0;
        T* ptr = nullptr;
    };

    T* ptr = nullptr;
    Cb* cb = nullptr;
};

struct Node
{
    Node* next = nullptr;
    int data;
    Node(int d, Node* n)
        : data(d)
        , next(n)
    {
    }
};

Node* invert(Node& start)
{
    Node* prev = nullptr;
    Node* cur = &start;
    Node* nxt = start.next;

    while (nxt)
    {
        Node* t = nxt->next;
        nxt->next = cur;
        cur->next = prev;

        prev = cur;
        cur = nxt;
        nxt = t;
    }

    return cur;
}

void sortints(std::vector<int> in)
{
    int counts[130] = {};
    for (auto i : in)
    {
        ++counts[i];
    }

    for (size_t i = 0; i < 130; ++i)
    {
        for (size_t p = 0; p < counts[i]; ++p)
        {
            std::cout << i << " ";
        }
    }

    std::cout << std::endl;
}

/// validate search tree
struct TreeNode
{
    TreeNode* lh = nullptr;
    TreeNode* rh = nullptr;
    TreeNode* p = nullptr;
    int value = 0;

    TreeNode(int v) : value(v) {}
};

TreeNode* min(TreeNode* root)
{
    TreeNode* m = root->lh;
    if (!m)
    {
        return root;
    }

    while (m->lh)
    {
        m = m->lh;
    }

    return m;
}

TreeNode* next(TreeNode* cur)
{
    if (cur->rh)
    {
        return min(cur->rh);
    }

    while (cur->p && cur == cur->p->rh)
    {
        cur = cur->p;
    }

    if (cur->p)
    {
        return cur->p;
    }

    return nullptr;
}

bool validate(TreeNode* root)
{
    int v = min(root)->value;
    for (TreeNode* m = min(root); m; m = next(m))
    {
        if (v > m->value)
        {
            std::cout << std::endl;
            return false;
        }

        v = m->value;
        std::cout << v << " ";
    }

    std::cout << std::endl;
    return true;
}

/// spinlock
class Spinlock
{
public:
    void Lock()
    {
        while (!TryLock()) // todo see boost::spinlock
        {
            /// no sleep because of spinlock
        }
    }

    bool TryLock()
    {
        /// force applying invalidate msgs, we clear our cache
        /// read value from remote cache (not from store buffer) via read msg if we were invalidated, from local cache if not
        /// put 1 to store buffer, send invalidate msg and receive immediate acknoledge
        /// RMW operations guarantee global visibility and serialized with another RMW despite of memory ordering
        ///
        return !locked.test_and_set(std::memory_order_acquire);  // sync with prev release
    }

    void Unlock()
    {
        /// force applying stores in store buffer to cache ( after it locked and prev stores are guaranteed in cache)
        /// send invalidates to other CPUs
        locked.clear(std::memory_order_release);    // sync with next acquire-ops
    }

private:
    std::atomic_flag locked;
};

int global;
Spinlock sl;
void SpinlockTest()
{
    int t = 0;
    //global = 3;     // write
    sl.Lock();  // mo::acquire: all reads after
    global = 3;      // write
    t = global;      // read
    sl.Unlock();// mo::release: all writes before
    //t = global;    // read

    std::cout << "Spinlock done" << std::endl;
}

struct ControlBlock
{
    int counter = 1;
    int weakCounter = 1;
    int* ptr = nullptr;

    void AddRef()
    {
        ++counter;
    }

    void AddRefWeak()
    {
        ++weakCounter;
    }

    void Release()
    {
        if (!--counter) // atomic
        {
            Dispose();
            ReleaseWeak();
        }
    }

    void ReleaseWeak()
    {
        if (!--weakCounter)
        {
            delete this;
        }
    }

    void Dispose()
    {
        delete ptr;
    }
};

template <typename one_type, typename two_type>
bool dcas(
    one_type& a, one_type& expecteda, one_type vala,
    two_type& b, two_type& expectedb, two_type valb
)
{
    return true;
}

struct SharedPtr
{
    SharedPtr() = default;

    SharedPtr(const SharedPtr& copy)
        : ctrl(copy.ctrl)
        , ptr(copy.ptr)
    {
        ctrl->AddRef();
    }

    SharedPtr(int* data)
        : ctrl(new ControlBlock)
        , ptr(data)
    {
        ctrl->ptr = ptr;
    }

    ~SharedPtr()
    {
        if (ctrl)
        {
            ctrl->Release();
        }
    }

    void reset(int* data)
    {
        SharedPtr copy(data);
        swap(copy);
    }

    SharedPtr& operator=(const SharedPtr& copy)
    {
        SharedPtr temp(copy);
        swap(temp);
        return *this;
    }

    int operator*()
    {
        return *ptr;
    }

    int* operator->()
    {
        return ptr;
    }

    void swap(SharedPtr& copy)
    {
        std::swap(copy.ctrl, ctrl);
        std::swap(copy.ptr, ptr);
    }

    void atomic_swap(SharedPtr copy)
    {
        ControlBlock* tctrl = nullptr;
        int* tptr = nullptr;
        /// else sp is null

        /// problem: ctrl could have been deleted while manipulating
        bool res = false;
        do {
            tctrl = ctrl;
            if (tctrl)
            {
                tctrl->AddRef();
            }

            tptr = tctrl ? tctrl->ptr : nullptr;
            res = dcas(ctrl, tctrl, copy.ctrl,
                       ptr, tptr, copy.ptr);

            if (tctrl)
            {
                tctrl->Release();
            }
        } while(res);
    }

    ControlBlock* ctrl = nullptr;
    int* ptr = nullptr;
};

/*
    ctx.Search().Offers(ShopId, CMagicId, OfferId).GroupBy(ShopId, OfferId)
        .Where(Text.Contains("hello") && OfferId.OneOf(1,2,3));
        
    ctx.Search().Clusters(Id, Title).GroupBy(Id)
        .Where(
            Text.Contains(cgi.Text) 
            && Params.Match(cgi.GLFilters)
            && Region.OneOf(cgi.Rids)
            && Category.OneOf(cgi.hid)
        );
        
    ctx.Search().Models(Id, Title, Picture).GroupBy(Id)
    
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
    int readProperty(int id, const char* attr) {
        if (!search_finished) {
            search_finished = true;
            search();
        }

        std::cout << "Read property " << attr << " from doc 1" << std::endl; return 0;
    }

public:
    void search() { std::cout << "Perform search" << std::endl; }

    bool search_finished = false;
};

struct TProperty {
    TProperty(TSearchImpl* impl = nullptr) : Impl(impl) {}
    TSearchImpl* Impl;
};

struct TOfferId : public virtual TProperty {
    static void Request(TSearchImpl* impl) { impl->requestAttr("offer_id");}

    /// have only attrs with support groupping
    void Group(int cache) { Impl->requestGroup("offer_id", cache);}

    /// ytodo support index
    int OfferId() { return Impl->readProperty(0, "offer_id"); }
};

struct TShopId : public virtual TProperty {
    static void Request(TSearchImpl* impl) { impl->requestAttr("shop_id"); }
    int ShopId() { return Impl->readProperty(0, "shop_id"); }
};

struct TMagicId : public virtual TProperty {
    static void Request(TSearchImpl* impl) { impl->requestAttr("magic_id"); }
    int MagicId() { return Impl->readProperty(0, "magic_id"); }
    void Group(int cache) { Impl->requestGroup("magic_id", cache);}
};

/// can be ptr, but need remove_pointer treat
static TShopId ShopId;
static TOfferId OfferId;
static TMagicId MagicId;

template <typename TGroup>
struct TGroupWrapper : public TGroup {
};

template <typename TAttrs>
struct TSearchResult {
    TSearchResult(TSearchImpl* impl) : Impl(impl) {}
    TSearchImpl* Impl;

    template <typename TGroup>
    void GrouppedBy(TGroup);
};

template <typename TSource, typename TGroup>
struct TWithGroup : public TSource, public TGroupWrapper<TGroup> {
    using TThisType = TWithGroup<TSource, TGroup>;
    TWithGroup(TSearchImpl* impl, int cache) : TSource(impl)
    {
        TGroupWrapper<TGroup>::Group(cache);
    }

    template <typename TAttr>
    TWithGroup<TThisType, TAttr> GroupBy(TAttr, int cache) {
        return TWithGroup<TThisType, TAttr>(TProperty::Impl, cache);
    }

    TSearchResult<TThisType> Done() {
        return {TProperty::Impl};
    }
};

template <typename TAttrs>
template <typename TGroup>
void TSearchResult<TAttrs>::GrouppedBy(TGroup) {
    TAttrs* fake = nullptr;
    (void)static_cast<TGroupWrapper<TGroup>*>(fake);
}

template <typename TSource, typename TGroup>
TWithGroup<TSource, TGroup> AddGroupping(TSearchImpl* impl, int cache) {
    return {impl, cache};
}

template <typename... TArgs>
struct TGroupAttributes : public TArgs... {
    TGroupAttributes(TSearchImpl* impl) : TProperty(impl) {
        [=](...){ }((TArgs::Group(1), 0)...);
    }
};

template <typename TRequestAttrs, typename TGroupAttrs>
struct TSearchResult2 : public TRequestAttrs, public TGroupAttrs {
    TSearchResult2(TSearchImpl* impl) : TRequestAttrs(impl), TGroupAttrs(impl) {}

    template <typename TGroup>
    void GrouppedBy(TGroup) {
        TGroupAttrs* fake;
        (void)static_cast<TGroup*>(fake);
    }

};

/// rename
template <typename... TArgs>
struct TAttrubutes : public TArgs... {
    using TThis = TAttrubutes<TArgs...>;
    TAttrubutes(TSearchImpl* impl) : TProperty(impl) {}
    static void Request(TSearchImpl* impl) {
        [=](...){ }((TArgs::Request(impl), 0)...);  /// make nonstatic: how to invoke base method with dup name?
    }

    /*template <typename TAttr>
    TWithGroup<TAttrubutes<TArgs...>, TAttr> GroupBy(TAttr, int cache) {
        using TThisType = TAttrubutes<TArgs...>;
        return AddGroupping<TThisType, TAttr>(TProperty::Impl, cache);
    }*/

    template <typename... TGrAttrs>
    TSearchResult2<TThis, TGroupAttributes<TGrAttrs...>> Group(TGrAttrs...) {
        return {TProperty::Impl};
    }
};

struct TSearchCollections {
    /// make a request with attrs
    template <typename... TArgs>
    TAttrubutes<TArgs...> Offers(TArgs...) {
        auto res = TAttrubutes<TArgs...>(&Impl); /// pass impl to ctor
        res.Request(&Impl); // can pass flag of offer owner here
        Impl.search_finished = false;
        return res;
    }

    template <typename... TArgs>
    TAttrubutes<TArgs...> Models(TArgs...) {
        auto res = TAttrubutes<TArgs...>(&Impl); /// pass impl to ctor
        res.Request(&Impl); // can pass flag of model owner here
        Impl.search_finished = false;
        return res;
    }

    int Clusters;

public:
    TSearchCollections(TSearchImpl& impl) : Impl(impl) {}

private:
    TSearchImpl& Impl;
};

struct TSearchSession {
    TSearchCollections Select() {
        return {Impl};
    }

    TSearchImpl Impl;
};

/// todo:
/// return document iterator
/// add groupping
/// optimize query tree

int main(int argc, const char * argv[])
{
/// search concept

    TSearchSession ctx;
    auto result = ctx.Select().Offers(OfferId, ShopId).Group(MagicId, OfferId);
    result.GrouppedBy(OfferId);
    result.GrouppedBy(MagicId);

    //auto offers = result.GrouppedBy(MagicId);
    //offers.OfferId();
    (void)result;

//    auto models = ctx.Search().Models(ShopId);
//    models.ShopId();

/// end search concept

    A a;
    A b(a);
    a = b;

    msp<int> i(new int(5));
    msp<int> j = i;
    std::cout << "j: " << *j << std::endl;

    Node n3(3, nullptr);
    Node n2(2, &n3);
    Node n1(1, &n2);
    Node n0(0, &n1);

    for (Node* i = &n0; i; i = i->next)
    {
        std::cout << i->data << ",";
    }

    std::cout << std::endl;

    Node* inv = invert(n0);
    for (Node* i = inv; i; i = i->next)
    {
        std::cout << i->data << ",";
    }

    std::cout << std::endl;

    std::vector<int> ints = {1,1,3,1,4,4,4,5,2,1};
    sortints(ints);

    /// tree
    TreeNode three(3);
    TreeNode one(1);
    TreeNode two(2);
    TreeNode four(4);

    /*
        2
       / \
      1   3
           \
            4
    */
    one.p = &two;
    two.lh = &one;

    three.p = &two;
    two.rh = &three;

    four.p = &three;
    three.rh = &four;

    bool valid = validate(&two);
    std::cout << "Tree is valid: " << valid << std::endl;
    four.value = 0;
    valid = validate(&two);
    std::cout << "Tree is valid: " << valid << std::endl;

    SpinlockTest();

    // dynamic cast from void* is not working
    /*A1* b1 = new B1;
    void* bv = b1;
    B1* b11 = dynamic_cast<B1*>(bv);
    if (b11)
    {
        std::cout << "Cast is successful" << std::endl;
    }
    else
    {
        std::cout << "Bad cast" << std::endl;
    }*/

    return 0;
}

