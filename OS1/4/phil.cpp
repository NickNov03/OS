// http://howardhinnant.github.io/dining_philosophers.html

#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

size_t eat_sleep_interval = 100;
size_t think_sleep_interval = 10;
size_t max_eat_times = 10;

void message(const int x, const std::string& message)
{
  std::cout << std::setfill('0') << std::setw(2) << x << message << std::endl;
}

void sleep(int millisec)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(millisec));
}

template <class L0, class L1>
void lock_naive(int x, L0& left, L1& right)
{
  message(x, " will take left fork"); 
  left.lock();
  message(x, " will take right fork");
  right.lock();
  message(x, " will eat..."); 
  sleep(eat_sleep_interval);

  message(x, " will put left fork");
  right.unlock();
  message(x, " will put right fork");
  left.unlock();
}

template <class L0, class L1>
void lock_smart_polite(int x, L0& left, L1& right)
{
  while (true)
  {
    {
      std::unique_lock<L0> left_lock(left);
      message(x, " taken left fork...");
      if (right.try_lock())
      {
        message(x, " taken right fork...");
        sleep(eat_sleep_interval);
        message(x, " finished eating...");
        left_lock.release();
        message(x, " put back both forks");
        break;
      } else {
        message(x, " failed to take right fork, put left back");
      }
    }
    std::this_thread::yield();

    {
      std::unique_lock<L1> right_lock(right);
      message(x, " taken right fork...");
      if (left.try_lock())
      {
        message(x, " taken left fork...");
        sleep(eat_sleep_interval);
        message(x, " finished eating...");
        right_lock.release();
        message(x, " put back both forks");
        break;
      } else {
        message(x, " failed to take left fork, put right back");
      }
    }
    std::this_thread::yield();
  }
}

void philosopher(int x, std::mutex& left, std::mutex& right)
{
  for (int times = 0; times < max_eat_times; times++)
  {
    lock_smart_polite(x, left, right);
    left.unlock();
    right.unlock();
    message(x, " will think...");
    sleep(think_sleep_interval);
  }
}

int main(int argc, char* argv[])
{
  size_t phil_num = 5;
  
  if (argc > 1)
    phil_num = std::stoi(argv[1]);

  std::cout << "Will work with " << phil_num << " philosophers" << std::endl;

  std::vector<std::mutex> forks(phil_num);

  std::thread* phils = new std::thread [phil_num];
  for (int i = 0; i < phil_num; i++)
    phils[i] = std::thread(philosopher, i, std::ref(forks[i]), std::ref(forks[(i+1)%phil_num]));

  for (int i = 0; i < phil_num; i++)
    phils[i].join();

  delete [] phils;

  return 0;
}
