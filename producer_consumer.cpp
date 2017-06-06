#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <string>
#include <algorithm>
#include <chrono>
#include <limits>
#include <cmath>
#include <unordered_set>
#include <set>
#include <thread>
#include <cassert>

using namespace std;

/*
 *   Cyclic buffer.
 *
 *   0 1 2 3 4 5 6 7 8 9 10 11
 *
 *   ^                    ^
 *   |                    |
 *  first (popped)    last (pushed)
 */
class Buffer {
    unique_ptr<int[]> ptr;
    const int cap;

    int size;
    int first;
    int last;

    mutex mtx;
    condition_variable cond_producer;
    condition_variable cond_consumer;

public:

    Buffer(int n) : ptr(new int[n]), cap(n), size(0), first(0), last(0) {}

    void push(int x) {
        unique_lock<mutex> lck(mtx);

        cond_producer.wait(lck, [this]() { return size < cap; });

        ptr[last] = x;
        ++size;
        last = (last + 1) % cap;

        cond_consumer.notify_one();
        lck.unlock();
    }

    int pop() {
        unique_lock<mutex> lck(mtx);

        cond_consumer.wait(lck, [this]() { return size > 0; });

        --size;
        auto ans = ptr[first];
        first = (first + 1) % cap;

        cond_producer.notify_one();
        lck.unlock();

        return ans;
    }
};

int main(int argc, char** argv)
{
    if (argc < 4) {
        cout << "Usage: " << argv[0] << " capacity prod_count cons_count\n";
        return 0;
    }

    cout << "The program shows current accumulated value in a cyclic buffer.\n";
    cout << "Producers push, consumers pop.\n";
    cout << "By tuning params you can see how the value changes.\n\n";

    srand(time(nullptr));
    int cap = stoi(argv[1]);
    int producer_count = stoi(argv[2]);
    int consumer_count = stoi(argv[3]);

    cout << "Capacity: " << cap << endl;
    cout << "Producer count: " << producer_count << endl;
    cout << "Consumer count: " << consumer_count << endl << endl;

    Buffer b(cap);

    atomic<long long> acc{0};

    vector<thread> producers(producer_count);
    vector<thread> consumers(consumer_count);
    for (auto& producer : producers) {
        producer = thread([&]() { 
                    while (true) {
                        auto n = 1 + rand() % 10;
                        b.push(n);
                        acc += n;
                    }
            });
    }
    for (auto& consumer : consumers) {
        consumer = thread([&]() { while (true) acc -= b.pop(); });
    }

    cout << "Accumulated value\n";

    while (true) {
        cout << acc << endl;
        this_thread::sleep_for(1s);
    }

    // Never runs
    for (auto& producer : producers) producer.join();
    for (auto& consumer : consumers) consumer.join();

    return 0;
}
