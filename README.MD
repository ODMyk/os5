# System Call API. Hooks & Event Handlers.

Цей проєкт демонструє використання низькорівневих можливостей ядра Linux для доступу до пам’яті іншого процесу та перехоплення клавіатурного вводу через `/proc/[pid]/mem` і `/dev/input/event*`.

## 🔧 Реалізовано

### 📦 1. Спостереження за пам’яттю іншого процесу

- `writer.cpp`: процес-джерело, який записує значення у змінну.
- `reader.cpp`: процес-спостерігач, який читає цю змінну напряму через `/proc/[pid]/mem`.

### 🎹 2. Перехоплення клавіатурного вводу

- `keyboard_hook.cpp`: читає натискання клавіш із `/dev/input/eventX` та виводить коди натиснутих клавіш у реальному часі.

## 📁 Структура проєкту

```bash
os5/
├── reader.cpp           # Процес-спостерігач
├── writer.cpp           # Процес-джерело
├── keyboard_hook.cpp    # Перехоплення клавіатурного вводу
├── Makefile             # Файл для компіляції
├── dist/                # Каталог для зібраних виконуваних файлів (зʼявляється після компіляції)
└── README.md            # Документація
```

## Встановлення залежностей

### Ubuntu / Debian / Linux Mint

```bash
sudo apt update
sudo apt install -y build-essential g++ cmake make libudev-dev
```

### Arch / Manjaro

```bash
sudo pacman -Syu --noconfirm
sudo pacman -S --noconfirm base-devel gcc cmake make libudev0-shim
```

### Fedora

```bash
sudo dnf install -y @development-tools gcc-c++ cmake make systemd-devel
```

## ⚙️ Компіляція

У репозиторії використовується `Makefile`, що дозволяє зручно компілювати будь-яку з версій:

```bash
make                    # Компілює одразу всі 3 файли
make reader             # Компілює src/reader.cpp -> dist/reader
make writer             # Компілює src/writer.cpp -> dist/writer
make keyboard_hook      # Компілює src/keyboard_hook.cpp -> dist/keyboard_hook
```

## ▶️ Запуск

### 1. Відстеження змін у пам'яті

#### Крок 1. У першому терміналі:

```bash
./writer
```

#### Крок 2. У другому терміналі:

```bash
sudo ./reader <PID_першого_процесу>
```

Де PID — це ідентифікатор процесу writer, для зручності він одразу виводиться у консоль

### 2. Перехоплення клавіатури

```bash
sudo ./keyboard_hook
```

Далі довільне натискання у довільному процесі (вікні) буде фіксуватися. Може виникнути баг, якщо підʼєднано декілька клавіатур - програма слідкує за першою, яку знайшла

## 📤 Результат

Файл `keyboard.log` з'явиться у корені проєкту після запуску keyboard_hook.

## 🪪 License

Цей проєкт ліцензовано за умовами MIT License.
