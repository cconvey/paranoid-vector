#include "util.h"
#include "paranoia_pool.h"
#include "paranoia_allocator.h"
#include "paranoid_vector.h"

#include <memory>
#include <iostream>
#include <string>
#include <limits>

using namespace std;

void test1() {
    using FooAllocatorCore = paranoia_allocator<int>;
#if 1
    {
        paranoia_allocator<int> my_alloc;
        my_alloc.ppool_ = std::make_shared<ParanoiaPool>(10000, 100);

        {
            //vector<int,FooAllocatorCore> v(my_alloc);
            vector<int,paranoia_allocator<int>> v(my_alloc);
            v.push_back(10);

            cout << "v.data() = " << HexPtr(v.data()) << dec
                << endl;

            cout << "v[0] = " << v[0] << endl;

            //my_alloc.ppool_->set_prot(v.data(), PROT_READ|PROT_WRITE);
            //my_alloc.ppool_->set_prot(v.data(), PROT_READ);
            //my_alloc.ppool_->set_prot(v.data(), PROT_NONE);

            cout << "v.data() = " << HexPtr(v.data()) << endl;
            cout << "v[0] = " << v[0] << endl;
            v[0] = 20;
            cout << "v[0] = " << v[0] << endl;
        }
    }
#endif
#if 0
    {
        cout << "LINE: " << __LINE__ << endl;
        vector<Foo,FooAllocatorCore> v2;
        cout << "LINE: " << __LINE__ << endl;
        v2.push_back({});
        cout << "LINE: " << __LINE__ << endl;
    }
    cout << "LINE: " << __LINE__ << endl;
#endif
}

void test2() {
    cout << endl;

    paranoid_vector<int> v1;

    cout << "v1.size() = " << v1.size() << endl;
    cout << "v1.data() = " << v1.data() << endl;
    cout << endl;

    v1.push_back(42);

    cout << "v1.size() = " << v1.size() << endl;
    cout << "v1.data() = " << v1.data() << endl;
    cout << "v1.at(0) = " << v1.at(0) << endl;
    cout << endl;

    auto v2 = v1;

    v2.push_back(100);
    v1.push_back(200);

    cout << "v1.size() = " << v1.size() << endl;
    cout << "v1.data() = " << v1.data() << endl;
    cout << "v1.at(0) = " << v1.at(0) << endl;
    cout << "v2.size() = " << v2.size() << endl;
    cout << "v2.data() = " << v2.data() << endl;
    cout << "v2.at(0) = " << v2.at(0) << endl;
    cout << endl;
}

void test3() {
    cout << endl;

    shared_ptr<paranoia_allocator<int>> my_alloc = make_shared<paranoia_allocator<int>>();
    my_alloc->ppool_ = std::make_shared<ParanoiaPool>(10000, 1000);
    paranoid_vector<int> v1(my_alloc);
    paranoid_vector<int> v2(my_alloc);

    for (int i = 0; i < 10; ++i) {
        v1.push_back(i);
    }

    v2.assign(v1.begin()+2, v1.begin()+5);
    v2.at(1) = 42;

    cout << "v1:";
    for (auto & x : v1) {
        cout << " " << x;
    }
    cout << endl;

    cout << "v2:";
    for (auto & x : v2) {
        cout << " " << x;
    }
    cout << endl;
}

void test4() {
    cout << endl;

    paranoid_vector<int> v1{1,2,3,4,5};
    cout << "v1:";
    for (auto & x : v1) {
        cout << " " << x;
    }
    cout << endl;

    v1.insert(v1.begin(), -1);
    v1.insert(v1.begin()+3, -2);
    v1.insert(v1.end(), -3);

    cout << "v1:";
    for (auto & x : v1) {
        cout << " " << x;
    }
    cout << endl;
}

void test5() {
    cout << endl;

    paranoid_vector<string> v1{"one", "two", "three"};

    cout << "v1:" << endl;
    for (auto & x : v1) {
        cout << " " << x << endl;
    }
    cout << endl;

    v1.emplace_back("four");

    cout << "v1:" << endl;
    for (auto & x : v1) {
        cout << " " << x << endl;
    }
    cout << endl;

    cout << "v1 (non-const reverse):" << endl;
    for (auto p = v1.rbegin(); p != v1.rend(); ++p) {
        cout << " " << *p << endl;
    }
    cout << endl;

    const auto & v1_const = v1;
    cout << "v1_const (reverse):" << endl;
    for (auto p = v1.rbegin(); p != v1.rend(); ++p) {
        cout << " " << *p << endl;
    }
    cout << endl;

    cout << "v1 (before erase):" << endl;
    for (auto & x : v1) {
        cout << " " << x << endl;
    }
    cout << endl;

    cout << "v1 (after erase):" << endl;
    v1.erase(v1.begin());
    for (auto & x : v1) {
        cout << " " << x << endl;
    }
    cout << endl;

    cout << "v1 (after pop_back):" << endl;
    v1.pop_back();
    for (auto & x : v1) {
        cout << " " << x << endl;
    }
    cout << endl;
}

int main() {
    //test1();
    //test2();
    //test3();
    //test4();
    test5();
}
