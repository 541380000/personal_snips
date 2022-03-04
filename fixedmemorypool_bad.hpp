/**
 * Author: xch
 * Date: 2022-03-04 @ Ruitian Capital
 * Fix Size Memory Pool
 * This implementation is very slow (maybe 10 times slow than malloc and free) in the simple test below.
 * Use it carefully.
 *
 * BTW, this memory pool does not align allocated memory, which will lead to bad performance.
 */
#include <iostream>
#include <cstdio>
#include <dirent.h>
#include <mutex>
#include <memory>
#include <utility>
#include <vector>
#include <chrono>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstring>
#include <thread>
#include "pathutil.hpp"
#include "measureutil.hpp"
using namespace std;

static size_t SYSTEM_PAGE_SIZE = getpagesize();

template<typename T>
class FixedMemoryPool{
public:
    explicit FixedMemoryPool(uint32_t nElements){
        this->applyNewFreePage(nElements);
    }
    ~FixedMemoryPool(){
        for (auto& i : this->vmPages){
            this->returnPageToKernel(i.page, i.size);
        }
    }
    T* malloc(){
        SpinLockGuard guard(this->mutex);
        if (this->freeList == nullptr)
            this->applyNewFreePage(1);
        this->allocatedUnits += 1;
        // cout << this->allocatedUnits << ' ';
        auto res =  (T*)((char*)removeFromHead(this->freeList) + sizeof(FixedMemoryUnit));
        return res;
    }

    void free(T* ptr){
        auto* unit = reinterpret_cast<FixedMemoryUnit*>(((char*)ptr) - sizeof(FixedMemoryUnit));
        SpinLockGuard guard(this->mutex);
        this->allocatedUnits -= 1;
        this->insertToHead(this->freeList, unit);
    }

    void printStatistic(){
        cout << this->allocatedUnits << " " << this->allUnits << endl;
    }

private:
    void applyNewFreePage(uint32_t nElements){
        uint32_t totalUnitSize = sizeof(T) + sizeof(FixedMemoryUnit);
        uint32_t totalInitSize = nElements * totalUnitSize;
        // get page from kernel
        uint32_t cnt = 0;
        while (++cnt * SYSTEM_PAGE_SIZE < totalInitSize);
        auto pages = this->getNPageFromKernel(cnt);
        this->allocatedPages += cnt;
        this->vmPages.emplace_back(VMPageMeta{pages, cnt});
        char* ptr = static_cast<char *>(pages);
        for (int i=0; i<cnt*SYSTEM_PAGE_SIZE/totalUnitSize; i++, ptr+=totalUnitSize) {
            auto unit = reinterpret_cast<FixedMemoryUnit*>(ptr);
            memset(unit, 0, sizeof(totalUnitSize));
            this->insertToHead(this->freeList, unit);
        }
        this->allUnits += cnt*SYSTEM_PAGE_SIZE/totalUnitSize;
    }

    class VMPageMeta{
    public:
        void* page;
        VMPageMeta(void *page, uint32_t size) : page(page), size(size) {}
        uint32_t size;
    };

    class FixedMemoryUnit{
        static const uint32_t elementSize = sizeof(T);
    public:
        FixedMemoryUnit* next;
    };


    static void insertToHead(FixedMemoryUnit*& head, FixedMemoryUnit* unit){
        if (head != nullptr){
            unit->next = head;
            head = unit;
        }else{
            unit->next = nullptr;
            head = unit;
        }
    }

    static FixedMemoryUnit* removeFromHead(FixedMemoryUnit*& head){
        if (head == nullptr) return nullptr;
        auto res = head;
        head = head->next;
        return res;
    }


    class SpinLock{
        std::atomic_flag locked = ATOMIC_FLAG_INIT ;
    public:
        void lock() {
            while (locked.test_and_set(std::memory_order_acquire)) { ; }
        }
        void unlock() {
            locked.clear(std::memory_order_release);
        }
    };
    class SpinLockGuard{
    public:
        SpinLock& lock;
        SpinLockGuard(SpinLock& lock): lock(lock){this->lock.lock();}
        ~SpinLockGuard(){lock.unlock();}
    };

    static void* getNPageFromKernel(uint32_t nUnits){
        if (nUnits == 0) return nullptr;
        char *vm_page = static_cast<char *>(mmap(
                nullptr,
                nUnits * SYSTEM_PAGE_SIZE,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_ANON | MAP_PRIVATE,
                0,
                0));
        if (vm_page == MAP_FAILED){
            printf("Error: VM Page allocation failed!\n");
            return nullptr;
        }
        memset(vm_page, 0, nUnits * SYSTEM_PAGE_SIZE);
        return vm_page;

    }
    static bool returnPageToKernel(void* page, uint32_t nUnits){
        if (munmap(page, SYSTEM_PAGE_SIZE * nUnits)){
            printf("Error: VM Page de-allocation failed!\n");
            return false;
        }
        return true;
    }
public:

    SpinLock mutex;
    FixedMemoryUnit* freeList = nullptr;
    vector<VMPageMeta> vmPages;
    uint32_t allocatedPages = 0;
    uint32_t allUnits = 0;
    uint32_t allocatedUnits = 0;
};

struct Person{
    int32_t age;
};

int test(){
#define THREAD_NUM 128
    FixedMemoryPool<Person> pool(10);
    Measure(
            vector<std::thread> vec;
            vec.reserve(THREAD_NUM);
            for (int i=0; i<THREAD_NUM; i++)
                vec.emplace_back(thread(
            []() {
                vector<Person*> vec;
                for (int i = 0; i < 100000; i++) {
                    auto res = (Person *) malloc(sizeof(Person));
                    vec.push_back(res);
                    // cout << *vec.rbegin() << ' ' << (*vec.rbegin()) - (bool*)pool.freeList->ptr << endl;
                }
                for (auto &i: vec) {
                    free(i);
                }
            }));
            for (auto& t: vec)
                t.join();
    )

    vec.clear();
    Measure(
            vector<std::thread> vec2;
    vec2.reserve(THREAD_NUM);
    for (int i=0; i<THREAD_NUM; i++)
        vec2.emplace_back(thread(

            [&]() {
                vector<Person*> vec;
        for (int i = 0; i < 100000; i++) {
            auto res = pool.malloc();
            res->age = 32;
            //cout << res << endl;
            vec.push_back(res);
            // cout << *vec.rbegin() << ' ' << (*vec.rbegin()) - (bool*)pool.freeList->ptr << endl;
        }
        //cout << "free" << endl;
        //pool.printStatistic();
        for (auto &i: vec) {
            //cout << "free -> " << i << endl;
            pool.free(i);
        }
    }
    ));
            for (auto& t: vec2)
                t.join();
    )
    return 0;
}




