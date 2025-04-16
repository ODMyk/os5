#include <iostream>
#include <unistd.h>

#define INVITATION "Введіть ціле числове значення для його збереження або щось інше для виходу: "
#define ENDING "Виконання завершено"

extern "C" int input = 0;

int main() {
    std::cout << "PID: " << getpid() << std::endl;
    do {
        std::cout << INVITATION;
    } while ((std::cin >> input));

    std::cout << ENDING << std::endl;

    return 0;
}
