#include <iostream>
#include <fstream>
#include <string>

const std::string SHARED_FILE = "/tmp/shared_memory.txt";

int main() {
    std::ofstream outFile(SHARED_FILE);
    if (!outFile.is_open()) {
        std::cerr << "Не вдалося відкрити файл для запису: " << SHARED_FILE << std::endl;
        return 1;
    }

    std::string input;
    std::cout << "Введіть значення: ";
    std::getline(std::cin, input);

    outFile << input;
    std::cout << "Значення записано у файл." << std::endl;

    outFile.close();
    return 0;
}
