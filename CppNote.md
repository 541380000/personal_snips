# Special member functions

```
class Demo{
    Demo() noexcept = default;
    virtual ~Demo() noexcept = default;

    Demo(const Demo& demo) noexcept = default;
    Demo& operator= (const Demo& demo) = default;

    Demo(Demo&& demo) = default;
    Demo& operator= (Demo&& demo) = default;
}
```

# structured binding(or unpacking)

```
struct Person{
    string name;
    int age;
    bool gender;
};

int main(){
    // unpack a struct
    Person p{"William", 16, true};
    auto const & [name, age, gender] = p;
    cout << name << ' ' << age << ' ' << gender << endl;

    // unpack a tuple
    auto t = tuple<string, double>("000001.SZ", 10.9);
    auto const& [stock, price] = t;
    cout << stock << ' ' << price << endl;

    //unpack a pair
    map<int, int> square{{1,1}, {2,4}, {3,9}, {4,16}, {5,25}};
    for (auto const& [a, s] : square)
        cout << "square of " << a << " is " << s << endl;

    // also we can unpack an array

    // before C++17 we can do it with std::tie
}
```

# using if when limiting variable scope

```
int main(){
    map<int, int> square{{1,1}, {2,4}, {3,9}, {4,16}, {5,25}};
    if (auto itr (square.find(3)); itr != square.end()){
        cout << "find square of " << itr->first << " is " << itr->second << endl;
    }else{
        cout << "Cannot find square" << endl;
    }

    mutex lock{};
    if (auto const& guard ((lock_guard<mutex>(lock))); 1 != 1){

    }else{

    }   // guard is deconstructed here
}
```

# vector: remove items

```
int main(){
    vector<int> v {1, 2, 3, 2, 5, 2, 6, 2, 4, 8};
    const auto newEnd(remove(begin(v), end(v), 2));
    v.erase(newEnd, end(v));
    for_each(v.begin(), v.end(), [](const auto & item){ cout << item << ' ';});
    cout << endl;

    v.erase(remove_if(v.begin(), v.end(), [](auto const& item){return item % 2 == 0;}), v.end());
    for_each(v.begin(), v.end(), [](const auto & item){ cout << item << ' ';});
    cout << endl;
}
```

# map: insert into map

```
// using insert and emplace will immediately copy the key and value, then try to insert
// try_emplace will first find it in the map, if failed, it returns without the copy
int main(){

    int arr[1000000];
    srand(time(nullptr));
    int cnt = 0;
    for (auto& i : arr){
        i = random()%100;
    }
    auto t1 = chrono::high_resolution_clock::now().time_since_epoch().count();
    map<int, int> map1;
    for (auto const& i : arr){
        auto [itr, success] = map1.insert( pair(i,1));
        if (!success) {
            itr->second += 1;
        }
    }
    auto t2 = chrono::high_resolution_clock::now().time_since_epoch().count();
    cout << "using insert cost: " << t2 - t1 << endl;

    t1 = chrono::high_resolution_clock::now().time_since_epoch().count();
    map<int, int> map2;
    for (auto const& i : arr){
        auto [itr, success] = map2.emplace( pair(i,1));
        if (!success) {
            itr->second += 1;
        }
    }
    t2 = chrono::high_resolution_clock::now().time_since_epoch().count();
    cout << "using emplace cost: " << t2 - t1 << endl;



    t1 = chrono::high_resolution_clock::now().time_since_epoch().count();
    map<int, int> map3;
    for (auto const& i : arr){
        auto [itr, success] = map3.try_emplace(i, 1);
        if (!success) {
            itr->second += 1;
        }
    }
    t2 = chrono::high_resolution_clock::now().time_since_epoch().count();
    cout << "using try_emplace cost: " << t2 - t1 << endl;

}
```

# map: insert effecitively

```
vector<string> vec;
    vec.reserve(27*27*27);
    for (char i='a'; i<='z'; i++)
        for (char j='a'; j<='z'; j++)
            for (char k='a'; k<='z'; k++){
                vec.emplace_back(string{i,j,k});
            }
    reverse(vec.begin(), vec.end());
    for_each(vec.begin(), vec.end(), [](auto const& item){
        cout << item << ' ';
    });

    cout << endl;
    map<string, int> map1;

    auto t1 = chrono::high_resolution_clock::now().time_since_epoch().count();
    for (auto const& i : vec){
        map1.insert(pair(i, 1));
    }
    auto t2 = chrono::high_resolution_clock::now().time_since_epoch().count();
    cout << "using insert cost: " << t2 - t1 << endl;


    map<string, int> map2;
    t1 = chrono::high_resolution_clock::now().time_since_epoch().count();
    auto itr = map2.end();
    for (auto const& i : vec){
        itr = map2.insert(itr, pair(i, 1));
    }
    t2 = chrono::high_resolution_clock::now().time_since_epoch().count();
    cout << "using insert with hint cost: " << t2 - t1 << endl;


    map<string, int> map3;
    t1 = chrono::high_resolution_clock::now().time_since_epoch().count();
    itr = map3.end();
    for (auto const& i : vec){
        itr = map3.insert(map3.end(), pair(i, 1));
    }
    t2 = chrono::high_resolution_clock::now().time_since_epoch().count();
    cout << "using insert with wrong hint cost: " << t2 - t1 << endl;

    // using insert cost: 27574000
    // using insert with hint cost: 9268400
    // using insert with wrong hint cost: 30095600
```
