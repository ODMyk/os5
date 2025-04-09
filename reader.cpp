#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

const std::string SHARED_FILE = "/tmp/shared_memory.txt";
const int CHECK_INTERVAL_MS = 1000;

int main() {
    std::string lastValue;

    while (true) {
        std::ifstream inFile(SHARED_FILE);
        if (!inFile.is_open()) {
            std::cerr << "Очікування появи файлу..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
            continue;
        }

        std::string currentValue;
        std::getline(inFile, currentValue);
        inFile.close();

        if (currentValue != lastValue) {
            std::cout << "Зміна значення: " << currentValue << std::endl;
            lastValue = currentValue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
    }

    return 0;
}
