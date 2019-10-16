#include "paranoia_pool.h"
#include "paranoia_allocator.h"

#include <memory>
#include <iostream>
#include <iomanip>

using namespace std;

using FooAllocatorCore = paranoia_allocator<int>;

int main() {
#if 1
    {
        paranoia_allocator<int> my_alloc;
        my_alloc.ppool_ = std::make_shared<ParanoiaPool>(10*1000*1000);

        {
            //vector<int,FooAllocatorCore> v(my_alloc);
            vector<int,paranoia_allocator<int>> v(my_alloc);
            v.push_back(10);

            cout << "v.data() = " << hex << showbase << v.data() << dec
                << endl;

            cout << "v[0] = " << v[0] << endl;

            //my_alloc.ppool_->set_prot(v.data(), PROT_READ|PROT_WRITE);
            //my_alloc.ppool_->set_prot(v.data(), PROT_READ);
            //my_alloc.ppool_->set_prot(v.data(), PROT_NONE);

            cout << "v.data() = " << hex << showbase << v.data() << dec
                << endl;

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
    return 0;
}
