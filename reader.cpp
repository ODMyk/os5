#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <gelf.h>
#include <libelf.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>

// Константи для загальних налаштувань
#define MILISECOND 1000
#define SLEEP_INTERVAL 100 * MILISECOND
#define PATH_MAX_SIZE 1024
#define BUFFER_SIZE 1024
#define PERMISSIONS_MASK "rw-p"
#define ADDRESS_MASK "Address: (0x[0-9a-fA-F]+)"

// Константи для роботи з адресами
#define INPUT_OFFSET 0x27c
#define HEX_BASE 16

// Константи для повідомлень
#define ERR_ELF_INIT "ELF init error\n"
#define ERR_OPEN_BINARY "open binary"
#define ERR_ELF_BEGIN "elf_begin failed\n"
#define ERR_OPEN_MAPS "Не вдалося відкрити "
#define ERR_FIND_SYMBOL "Не вдалося знайти символ 'input'\n"
#define ERR_FIND_DATA_SEGMENT "Не вдалося знайти сегмент даних\n"
#define ERR_FIND_ADDRESS "Не вдалося знайти адресу змінної 'input'\n"
#define ERR_POPEN "popen"
#define ERR_READLINK "readlink"
#define ERR_OPEN_MEM "open mem"
#define ERR_LSEEK "lseek"
#define ERR_READ "read"
#define ERR_NO_PID_START "Помилка: процес з PID "
#define ERR_NO_PID_END " не знайдений.\n"

#define MSG_USAGE "Використання: ./reader <pid>\n"
#define MSG_CHANGE "Зміна: "
#define START_MESSAGE "Спостереження розпочато...\n"
#define END_MESSAGE "Досягнуто кінця файлу пам'яті або сталася помилка.\n"

bool check_process_exists(pid_t pid) {
    std::string path = "/proc/" + std::to_string(pid);
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);  // Перевіряємо, чи існує директорія процесу
}

// Функція для знаходження зсуву символу "input" в ELF файлі
intptr_t find_input_offset(const char* binary_path) {
    if (elf_version(EV_CURRENT) == EV_NONE) {
        std::cerr << ERR_ELF_INIT;
        return 0;
    }

    int fd = open(binary_path, O_RDONLY);
    if (fd < 0) {
        perror(ERR_OPEN_BINARY);
        return 0;
    }

    Elf* elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) {
        std::cerr << ERR_ELF_BEGIN;
        close(fd);
        return 0;
    }

    Elf_Scn* scn = nullptr;
    GElf_Shdr shdr;
    while ((scn = elf_nextscn(elf, scn))) {
        gelf_getshdr(scn, &shdr);
        if (shdr.sh_type == SHT_SYMTAB) {
            Elf_Data* data = elf_getdata(scn, nullptr);
            int symbols = shdr.sh_size / shdr.sh_entsize;

            for (int i = 0; i < symbols; ++i) {
                GElf_Sym sym;
                gelf_getsym(data, i, &sym);

                const char* name = elf_strptr(elf, shdr.sh_link, sym.st_name);
                if (name && strcmp(name, "input") == 0) {
                    intptr_t offset = sym.st_value;
                    elf_end(elf);
                    close(fd);
                    return offset;
                }
            }
        }
    }

    elf_end(elf);
    close(fd);
    return 0;
}

// Функція для пошуку сегмента даних, де зберігається змінна input
intptr_t find_data_segment(pid_t pid, const std::string& binary_path) {
    std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps(maps_path);
    if (!maps) {
        std::cerr << ERR_OPEN_MAPS << maps_path << "\n";
        return 0;
    }

    // Спочатку знайдемо всі сегменти, що належать нашому бінарному файлу
    std::vector<std::pair<intptr_t, intptr_t>> segments;
    std::string line;

    while (std::getline(maps, line)) {
        if (line.find(binary_path) != std::string::npos) {
            std::stringstream ss(line);
            std::string addr_range;
            ss >> addr_range;
            
            size_t dash_pos = addr_range.find('-');
            if (dash_pos == std::string::npos) continue;
            
            std::string start_addr_str = addr_range.substr(0, dash_pos);
            std::string end_addr_str = addr_range.substr(dash_pos + 1);
            
            intptr_t start_addr = std::stoll(start_addr_str, nullptr, HEX_BASE);
            intptr_t end_addr = std::stoll(end_addr_str, nullptr, HEX_BASE);
            
            // Перевіряємо чи сегмент має права на читання/запис
            if (line.find(PERMISSIONS_MASK) != std::string::npos) {
                segments.push_back({start_addr, end_addr});
            }
        }
    }
    
    // Повертаємо початкову адресу першого сегмента з даними
    if (!segments.empty()) {
        return segments[0].first;
    }
    
    return 0;
}

// Пошук адреси змінної input у процесі за PID
intptr_t find_input_address(pid_t pid) {
    // Отримуємо вивід команди cat /proc/[pid]/maps | grep rw-p
    std::stringstream cmd;
    cmd << "cat /proc/" << pid << "/maps | grep " << PERMISSIONS_MASK;
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        perror(ERR_POPEN);
        return 0;
    }
    
    char buffer[BUFFER_SIZE];
    std::vector<std::string> lines;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        lines.push_back(buffer);
    }
    pclose(pipe);
    
    // Отримуємо вивід команди cat /proc/[pid]/status
    cmd.str("");
    cmd << "cat /proc/" << pid << "/status | grep VmData";
    
    pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        perror(ERR_POPEN);
        return 0;
    }
    
    std::string vm_data;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        vm_data = buffer;
    }
    pclose(pipe);
    
    // Тепер спробуємо знайти адресу зі змінної
    // Отримуємо вивід програми
    cmd.str("");
    cmd << "cat /proc/" << pid << "/cmdline";
    
    pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        perror(ERR_POPEN);
        return 0;
    }
    
    std::string cmdline;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        cmdline = buffer;
    }
    pclose(pipe);
    
    // Тепер спробуємо отримати адресу зі стандартного виводу програми
    cmd.str("");
    cmd << "ps -p " << pid << " -o comm=";
    
    pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        perror(ERR_POPEN);
        return 0;
    }
    
    std::string process_name;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        process_name = buffer;
        // Видаляємо символ переносу рядка
        process_name.erase(std::remove(process_name.begin(), process_name.end(), '\n'), process_name.end());
    }
    pclose(pipe);
    
    // Спробуємо знайти адресу у файлі з виходом процесу
    std::string output_file = "/tmp/" + process_name + "_output.txt";
    std::ifstream output(output_file);
    std::string line;
    intptr_t address = 0;
    
    if (output.is_open()) {
        while (std::getline(output, line)) {
            if (line.find("Address:") != std::string::npos) {
                std::regex addr_regex(ADDRESS_MASK);
                std::smatch match;
                if (std::regex_search(line, match, addr_regex) && match.size() > 1) {
                    std::string addr_str = match[1].str();
                    address = std::stoll(addr_str, nullptr, 0);
                    break;
                }
            }
        }
        output.close();
    }
    
    return address;
}

// Функція для парсингу виводу програми напряму для пошуку адреси
intptr_t find_address_from_output(pid_t pid) {
    // Спочатку спробуємо отримати прямий доступ до стандартного виводу
    std::string cmd = "lsof -p " + std::to_string(pid) + " -n 2>/dev/null | grep txt";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        perror(ERR_POPEN);
        return 0;
    }
    
    char buffer[BUFFER_SIZE];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    // Отримуємо шлях до виконуваного файлу процесу
    std::string exe_link = "/proc/" + std::to_string(pid) + "/exe";
    char exe_path[PATH_MAX_SIZE];
    ssize_t len = readlink(exe_link.c_str(), exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror(ERR_READLINK);
        return 0;
    }
    exe_path[len] = '\0';
    
    // Знаходимо зсув символу input в ELF файлі
    intptr_t symbol_offset = find_input_offset(exe_path);
    if (!symbol_offset) {
        std::cerr << ERR_FIND_SYMBOL;
        return 0;
    }
    
    // Знаходимо базову адресу сегмента даних
    intptr_t data_base = find_data_segment(pid, exe_path);
    if (!data_base) {
        std::cerr << ERR_FIND_DATA_SEGMENT;
        return 0;
    }
    
    // Розраховуємо адресу змінної input
    intptr_t input_address = data_base + INPUT_OFFSET;
    
    return input_address;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << MSG_USAGE;
        return 1;
    }

    pid_t pid = std::stoi(argv[1]);

    if (!check_process_exists(pid)) {
        std::cerr << ERR_NO_PID_START << pid << ERR_NO_PID_END;
        return 1;
    }

    std::string exe_link = "/proc/" + std::to_string(pid) + "/exe";
    char exe_path[PATH_MAX_SIZE];
    ssize_t len = readlink(exe_link.c_str(), exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror(ERR_READLINK);
        return 1;
    }
    exe_path[len] = '\0';
    
    intptr_t address = find_address_from_output(pid);
    if (address == 0) {
        std::cerr << ERR_FIND_ADDRESS;
        return 1;
    }

    std::string mem_path = "/proc/" + std::to_string(pid) + "/mem";
    int mem_fd = open(mem_path.c_str(), O_RDONLY);
    if (mem_fd == -1) {
        perror(ERR_OPEN_MEM);
        return 1;
    }

    int prev = 0, curr = 0;

    std::cout << START_MESSAGE;
    while (true) {
        if (lseek(mem_fd, address, SEEK_SET) == -1) {
            perror(ERR_LSEEK);
            break;
        }

        ssize_t n = read(mem_fd, &curr, sizeof(curr));
        if (n != sizeof(curr)) {
            if (n == 0) {
                std::cerr << END_MESSAGE;
            } else {
                perror(ERR_READ);
            }
            break;
        }

        if (curr != prev) {
            std::cout << MSG_CHANGE << curr << "\n";
            prev = curr;
        }

        usleep(SLEEP_INTERVAL);
    }

    close(mem_fd);
    return 0;
}
