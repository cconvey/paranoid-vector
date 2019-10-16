#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    vector<string> v;
    v.push_back("Hi mom");

    for (auto item : v) {
        cout << item << endl;
    };

    string* p1 = &(v[0]);

    cout << "*p1 :" << endl;
    cout << *p1 << endl;
    cout << endl;

    v.clear();
    v.shrink_to_fit();

    cout << "*p1 :" << endl;
    cout << *p1 << endl;
    cout << endl;

    return 0;
}
