#include <future>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <memory>

#include <type_traits>



/* Non-thread-safe, affects readability. */
constexpr const bool PRINT_INFO = false;

class ThreadPool{
public:
    /* Construct function */
    ThreadPool(size_t nums_worker = std::thread::hardware_concurrency()): 
        should_stop_(false)
    {
        std::cout << "Thread number:" << nums_worker << std::endl;
        workers_.reserve(nums_worker);
        for (size_t i = 0; i < nums_worker; ++i){
            workers_.emplace_back([this, i](){
                while (true){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->task_mutex_);
                        this->cv_.wait(lock, [this](){
                            return this->should_stop_ || !this->tasks_.empty();
                        });
                        if (this->should_stop_ && this->tasks_.empty()){
                            return;
                        }
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop_front();
                        if constexpr (PRINT_INFO) std::cout << "task will be run at thrad:" << i << std::endl;
                    }
                    task();
                    if constexpr (PRINT_INFO) std::cout << "task finished at thread:" << i << std::endl;
                }
            });
        }
    }

    /* Destruct function*/
    ~ThreadPool(){
        {
            std::unique_lock<std::mutex> lock(this->task_mutex_);
            should_stop_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_){
            worker.join();
        }
    }

    /* Push task to the front of the task queue */
    template<typename Func, typename... Args>
    auto push_front(Func&& func, Args&&... args){
        return push<Func, true, Args...> (std::forward<Func>(func), std::forward<Args>(args)...);
    }

    /* Push task to the end of the task queue */
    template<typename Func, typename... Args>
    auto push_back(Func&& func, Args&&... args){
        return push<Func, false, Args...> (std::forward<Func>(func), std::forward<Args>(args)...); 
    }


private:
    bool                                should_stop_;
    std::vector<std::thread>            workers_;
    std::deque<std::function<void()>>   tasks_;
    std::mutex                          task_mutex_;
    std::condition_variable             cv_;
    

    /* Push function */
    template<typename Func, bool is_front,  typename... Args>
    auto push(Func&& func, Args&&... args) -> std::future<typename std::invoke_result<Func, Args...>::type>{
        using return_type = typename std:: invoke_result<Func, Args...>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
        [f = std::forward<Func>(func), tuple_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            return std::apply(f, std::move(tuple_args));
        });
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(task_mutex_);
            if (should_stop_){
                throw std::runtime_error("push task on stopped ThreadPool");
            }
            if constexpr (is_front){
                tasks_.emplace_front([task](){ (*task)(); });
            }
            else{
                tasks_.emplace_back([task](){ (*task)(); });
            }
        }
        cv_.notify_one();
        return res;
    };


};