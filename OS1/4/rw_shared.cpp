#include <iostream>
#include <string>
#include <thread>
#include <shared_mutex>
#include <chrono>
#include <vector>
#include <random>

std::shared_mutex mtx;

double number = 0;

void sleep(int millisec)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(millisec));
}

void writer()
{
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(1.0, 10.0);

  while (true)
  {
    mtx.lock();

    // Writing...
    number = dist(mt);
    std::cout << "Writing " << number << std::endl;

    mtx.unlock();

    sleep(1000);
  }
}

void reader()
{
  while (true)
  {
    mtx.lock_shared();
  
    // Reading... 
    std::cout << "Reading " << number << std::endl;
    
    mtx.unlock_shared();
    sleep(1000);
  }
}

int main(int argc, char* argv[])
{
  size_t count = 5;
  
  if (argc > 1)
    count = std::stoi(argv[1]);

  std::vector <std::thread> threads(count);

  for (int i = 0; i < count; i++)
    threads[i] = std::thread(i&3 ? reader : writer);

  for (std::thread& t : threads)
    t.join();

  return 0;
}
