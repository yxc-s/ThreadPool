#include "ThreadPool.h"
#include <iostream>

using namespace std;
int main(){

    ThreadPool pool{};
    /* Task1 : Calculate sum */
    auto task1 = [](int x){
        int res = 0;
        for (int i = 1; i <= x; ++i){
            res += i;
        }
        return res;
    };
    std::future<int> task1_res = pool.push_back(task1, 10000);
    std::cout << "task1 result:" <<task1_res.get() << std::endl;


    /* Task2 : Count odd number*/
    auto task2 = [](int x, int y){
        if (x > y){
            return -1;
        }
        int res = 0;
        while (x <= y){
            if (x & 1){
                res ++;
            }
            x ++;
        }
        return res;
    };
    std::future<int> task2_res = pool.push_back(task2, 10000, 12345);
    std::cout << "task2 result:" <<task2_res.get() << std::endl;


    /* Task 3 : Calculate factorial*/
    auto task3 = [&](auto &&self, long long x) -> long long {
        if (x == 1){
            return x;
        }
        return x * self(self, x - 1);
    };
    std::future<long long> task3_res = pool.push_back(task3, task3, 15);
    std::cout << "task3 result:" << task3_res.get() << std::endl;


    /* Task 4 : Output args */
    auto create_task4 = []() {
        return [](auto&& self, auto&& value, auto&&... args)->bool {
            std::cout << value << ' ';
            if constexpr (sizeof...(args) > 0) {
                return self(self, std::forward<decltype(args)>(args)...);
            }
            else{
                std::cout << ' ';
                return true;
            }
        };
    };
    auto task4 = create_task4();
    std::future<bool> task4_res = pool.push_back(task4, task4, "The", "world", "will", "be", "more", "beautiful", 0, "w", 0);
    std::cout << "task4 result:" << (task4_res.get() ? "Output over" : "Output unfinished") << std::endl;


    /* Task 5 : Check variable type */
    auto task5 = [](auto&& self, auto&& pre, auto&& cur, auto&&... args) -> bool {
        if constexpr (!std::is_same_v<std::decay_t<decltype(pre)>, std::decay_t<decltype(cur)>>) {
            return false;
        }
        if constexpr (sizeof...(args) <= 1) {
            return std::is_same_v<std::decay_t<decltype(cur)>, std::decay_t<decltype(args)>...>;
        }
        else{
            return self(self, std::forward<decltype(cur)>(cur), std::forward<decltype(args)>(args)...);
        }
    };
    std::future<bool> task5_res = pool.push_front(task5, task5, "The", "world", "will", "be", "more", "beautiful", 0, "w", 0);
    std::cout << "task5 result:" << (!task5_res.get() ? "Variables are different type" : "Variables are Same type") << std::endl;


    return 0;
}