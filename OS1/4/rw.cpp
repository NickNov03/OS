#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <random>

std::mutex no_writers, no_readers, counter_mutex;
size_t nreaders = 0;

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
    no_writers.lock();
    no_readers.lock();
    no_writers.unlock();

    // Writing...
    number = dist(mt);
    std::cout << "Writing " << number << std::endl;

    no_readers.unlock();
    sleep(1000);
  }
}

void reader()
{
  size_t prev, current;

  while (true)
  {
    no_writers.lock();
    counter_mutex.lock();
      if (nreaders++ == 0)
        no_readers.lock();
    counter_mutex.unlock();
    no_writers.unlock();
  
    // Reading... 
    std::cout << "Reading " << number << std::endl;
    
    counter_mutex.lock();
      if (--nreaders == 0)
        no_readers.unlock();
    counter_mutex.unlock();
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
