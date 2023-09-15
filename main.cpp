/**
 * @file main.cpp
 * @author Simon Ahlström (simon.ahlstrom91@gmail.com)
 * @brief  This project simulates the manufacturing and distribution of vehicles.
 * The vehicles are created and stored in a warehouse by a separate thread (manufacturer) at varying speeds,
 *  and consumed by multiple separate threads (dealers) also operating at different speeds.
 * Compile command: g++ -DDEALER=(>2) main.cpp -o main
 * @version 0.1
 * @date 2023-04-19
 *
 *
 */

#if DEALER < 2
#error "DEALER value is too low, must be at least 2."
#endif

#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <string>
#include <cstdlib>

#define CAPACITY 8

#define MAX_PASSANGER 5
#define MIN_PASSANGER 2
#define MAX_WEIGHT 1000
#define MIN_WEIGHT 500

#define MODEL_INIT_CHAR 'A'
#define MODEL_MAX_CHAR 'Z'

#define MODEL_MAX_NUM 90
#define MODEL_MIN_NUM 30

#define DEALER_SLEEP 5
#define MANUFACTURER_SLEEP 2

#define SERIAL_INIT 1000

std::mutex m;
std::condition_variable cv;

template <typename T, int SIZE>
class Warehouse
{
public:
    Warehouse() : m_readIndex(0), m_writeIndex(0), m_size(0)
    {
    }

    Warehouse &operator=(const Warehouse &) = delete;

    void push(const T &item)
    {
        buffer[m_writeIndex] = item;
        m_writeIndex = (m_writeIndex + 1) % SIZE;
        ++m_size;
    }

    T pop()
    {
        T item = buffer[m_readIndex];
        m_readIndex = (m_readIndex + 1) % SIZE;
        --m_size;

        return item;
    }
    bool isEmpty() const
    {
        return m_size == 0;
    }

    bool isFull() const
    {
        return m_size == SIZE;
    }

private:
    T buffer[SIZE];
    size_t m_readIndex;
    size_t m_writeIndex;
    int m_size;
};

class Vehicle
{

public:
    Vehicle(std::string a, std::string b) : model{a}, type{b} { serial = ++serial_generator; }
    virtual void print_properties() = 0;
    virtual ~Vehicle() = default;

protected:
    static int serial_generator;
    int serial;
    std::string model;
    std::string type;
};

class Car : public Vehicle
{
public:
    Car(std::string a, std::string b, int c) : Vehicle(a, b), max_passengers{c} {};

    void print_properties() override
    {
        std::cout << "Serial: " << serial << std::endl;
        std::cout << "Model: " << model << std::endl;
        std::cout << "Type: " << type << std::endl;
        std::cout << "Max Passengers: " << max_passengers << std::endl;
    }

private:
    int max_passengers;
};

class Truck : public Vehicle
{
public:
    Truck(std::string a, std::string b, int c) : Vehicle(a, b), max_weight{c} {};

    void print_properties() override
    {
        std::cout << "Serial: " << serial << std::endl;
        std::cout << "Model: " << model << std::endl;
        std::cout << "Type: " << type << std::endl;
        std::cout << "Max Weight: " << max_weight << std::endl;
    }

private:
    int max_weight;
};

int Vehicle::serial_generator{SERIAL_INIT};

Warehouse<Vehicle *, CAPACITY> warehouse;

void manufacturer(void)
{
    int picker{0};
    int rand_num{0};
    char rand_alph{0};
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(rand() % MANUFACTURER_SLEEP + 1));

        std::unique_lock lk(m);
        cv.wait(lk, []
                { return !warehouse.isFull(); });

        std::cout << "I have manufactered a new vehicle" << std::endl
                  << std::endl;

        rand_num = rand() % (MODEL_MAX_NUM - MODEL_MIN_NUM) + MODEL_MIN_NUM;
        rand_alph = rand() % (MODEL_MAX_CHAR - MODEL_INIT_CHAR) + MODEL_INIT_CHAR;

        picker = rand() % 2;

        if (picker == 1)
        {
            std::string random_modell_truck = rand_alph + std::to_string(rand_num);

            try
            {
                warehouse.push(new Truck(random_modell_truck, "Truck", rand() % (MAX_WEIGHT - MIN_WEIGHT + 1) + MIN_WEIGHT));
            }
            catch (const std::bad_alloc &e)
            {
                std::cerr << "Memory allocation failed: " << e.what() << '\n';
            }
        }
        else
        {
            std::string random_model_car = rand_alph + std::to_string(rand_num);

            try
            {
                warehouse.push(new Car(random_model_car, "car", rand() % (MAX_PASSANGER - MIN_PASSANGER + 1) + MIN_PASSANGER));
            }
            catch (const std::bad_alloc &e)
            {
                std::cerr << "Memory allocation failed: " << e.what() << '\n';
            }
        };

        lk.unlock();
        cv.notify_all();
    }
}

void dealer()
{
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(rand() % DEALER_SLEEP + 1));
        std::unique_lock lk(m);
        cv.wait(lk, []
                { return !warehouse.isEmpty(); });
        std::cout << "I have acquired a Vehicle from the Warehouse" << std::endl
                  << std::endl;

        Vehicle *v = warehouse.pop();
        v->print_properties();
        delete v;

        lk.unlock();
        cv.notify_all();
    }
}
int main(void)
{
    srand(time(NULL));
    std::thread manu(manufacturer);
    std::vector<std::thread> threadsVec;

    for (size_t i = 0; i < DEALER; i++)
    {
        threadsVec.push_back(std::thread{dealer});
    }

    manu.join();

    for (size_t i{0}; i < DEALER; ++i)
    {
        threadsVec[i].join();
    }

    return 0;
}
