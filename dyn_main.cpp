#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>    // Для std::setw
#include <chrono>
#include <thread>
#include <cstdlib>    // Для std::atoi
#include <algorithm>  // Для std::find
#include <cstdint>    // Для uint32_t

// Глобальные переменные для размеров поля
uint32_t ROWS = 20;
uint32_t COLS = 20;

// Тип сетки
using Grid = std::vector<std::vector<int>>;

// Флаги для управления выводом
static bool debugMode = true;

// Правила игры: наборы соседей для выживания и рождения
std::vector<int> survivalRules;
std::vector<int> birthRules;

// Функция генерации правил на основе длины и суммы байт файла
void generateRules(std::size_t fileLength, std::size_t byteSum) {
    // Инициализация генератора случайных чисел с заданным сидом
    unsigned int seed = static_cast<unsigned int>(fileLength + byteSum);
    srand(seed);

    // Генерация количества правил выживания (от 1 до 3)
    int numSurvival = rand() % 3 + 1;
    survivalRules.clear();
    while (survivalRules.size() < static_cast<size_t>(numSurvival)) {
        int rule = rand() % 9; // 0-8
        if (std::find(survivalRules.begin(), survivalRules.end(), rule) == survivalRules.end()) {
            survivalRules.push_back(rule);
        }
    }

    // Генерация количества правил рождения (от 1 до 3)
    int numBirth = rand() % 3 + 1;
    birthRules.clear();
    while (birthRules.size() < static_cast<size_t>(numBirth)) {
        int rule = rand() % 9; // 0-8
        if (std::find(birthRules.begin(), birthRules.end(), rule) == birthRules.end()) {
            birthRules.push_back(rule);
        }
    }

    // Вывод сгенерированных правил
    std::cout << "Сгенерированные правила игры:\n";
    std::cout << "Выживание (S): ";
    for (const auto& rule : survivalRules) {
        std::cout << rule << " ";
    }
    std::cout << "\nРождение (B): ";
    for (const auto& rule : birthRules) {
        std::cout << rule << " ";
    }
    std::cout << "\n\n";
}

// Функция хеширования сетки
std::size_t hashGrid(const Grid& grid) {
    std::size_t h = 0;
    for (const auto &row : grid) {
        for (const auto cell : row) {
            h ^= std::hash<int>()(cell) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
    }
    return h;
}

// Подсчёт соседей для "Игры Жизнь"
int countNeighbors(const Grid &g, int r, int c, bool verbose = false) {
    int count = 0;
    if (verbose) {
        std::cout << "  [Debug] Подсчёт соседей для клетки (" << r << "," << c << "): ";
    }
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int rr = r + dr;
            int cc = c + dc;
            if (rr >= 0 && rr < ROWS && cc >= 0 && cc < COLS) {
                count += g[rr][cc];
                if (verbose) {
                    std::cout << "[" << rr << "," << cc << "]=" << g[rr][cc] << " ";
                }
            }
        }
    }
    if (verbose) {
        std::cout << "=> Всего живых соседей: " << count << "\n";
    }
    return count;
}

// Правило "Игры Жизнь" с использованием пользовательских правил
int transitionRule(int cell, int neighbors) {
    if (cell == 1) {
        // Живая клетка выживает при наличии neighbors в survivalRules
        return (std::find(survivalRules.begin(), survivalRules.end(), neighbors) != survivalRules.end()) ? 1 : 0;
    } else {
        // Мёртвая клетка оживает при наличии neighbors в birthRules
        return (std::find(birthRules.begin(), birthRules.end(), neighbors) != birthRules.end()) ? 1 : 0;
    }
}

// Выполняем один шаг (итерацию) автомата
Grid nextGeneration(const Grid &g, bool verbose = false) {
    Grid newG = g;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int n = countNeighbors(g, r, c, verbose);
            newG[r][c] = transitionRule(g[r][c], n);
        }
    }
    return newG;
}
// Функция для вывода сетки в консоль с цветами
void printGrid(const Grid &grid, int iteration) {
    // ANSI код для очистки экрана и перемещения курсора в верхний левый угол
    std::cout << "\033[2J\033[H";
    std::cout << "Итерация: " << iteration << "\n";
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (grid[r][c]) {
                // Зелёный цвет для живых клеток
                std::cout << "\033[32m██\033[0m";
            } else {
                // Серый цвет для мёртвых клеток
                std::cout << "\033[90m  \033[0m";
            }
        }
        std::cout << "\n";
    }
    // Разделительная линия
    std::cout << std::string(COLS * 2, '-') << "\n";
}

// Функция для чтения файла и инициализации сетки в бинарном режиме
bool readInputFile(const std::string& filename, Grid& grid, std::size_t& fileLength, std::size_t& byteSum) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Ошибка: Не удалось открыть файл " << filename << "\n";
        return false;
    }

    // Чтение первых 8 байт для ROWS и COLS
    uint32_t rows = 0, cols = 0;
    infile.read(reinterpret_cast<char*>(&rows), sizeof(uint32_t));
    infile.read(reinterpret_cast<char*>(&cols), sizeof(uint32_t));

    // Обновление глобальных переменных
    ROWS = rows;
    COLS = cols;

    if (ROWS <= 0 || COLS <= 0) {
        std::cerr << "Ошибка: Размеры поля должны быть положительными.\n";
        return false;
    }

    // Инициализируем сетку
    grid.assign(ROWS, std::vector<int>(COLS, 0));

    // Чтение последующих ROWS * COLS байтов для инициализации клеток
    std::vector<char> buffer(ROWS * COLS, 0);
    infile.read(buffer.data(), buffer.size());

    // Вычисление длины файла и суммы байт
    infile.seekg(0, std::ios::end);
    fileLength = infile.tellg();
    infile.seekg(0, std::ios::beg); // Сброс позиции на начало файла

    byteSum = 0;
    for (const auto& byte : buffer) {
        byteSum += static_cast<unsigned char>(byte);
    }

    // Инициализация сетки
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            grid[r][c] = static_cast<unsigned char>(buffer[r * COLS + c]) ? 1 : 0;
        }
    }

    infile.close();

    return true;
}

// Функция для парсинга аргументов командной строки
bool parseArguments(int argc, char* argv[], std::string& inputFile) {
    for(int i =1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "--file") {
            if(i +1 < argc) {
                inputFile = argv[i+1];
                i++; // Пропускаем следующий аргумент, так как он уже прочитан
            } else {
                std::cerr << "Ошибка: Опция --file требует аргумент (имя файла).\n";
                return false;
            }
        }
        else if(arg == "--debug") {
            debugMode = true;
        }
        else {
            std::cerr << "Предупреждение: Неизвестный аргумент \"" << arg << "\"\n";
        }
    }

    if(inputFile.empty()) {
        std::cerr << "Ошибка: Необходимо указать входной файл через опцию --file.\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    // Парсинг аргументов командной строки
    std::string inputFile;
    if(!parseArguments(argc, argv, inputFile)) {
        std::cerr << "Использование: " << argv[0] << " --file <input_file> [--debug]\n";
        return 1;
    }

    Grid grid;
    std::size_t fileLength = 0;
    std::size_t byteSum = 0;

    // Чтение и инициализация сетки из файла
    if (!readInputFile(inputFile, grid, fileLength, byteSum)) {
        return 1; // Ошибка при чтении файла
    }

    // Генерация правил на основе длины и суммы байт файла
    generateRules(fileLength, byteSum);
    // Вывод начального состояния, если включён режим отладки
    if (debugMode) {
        std::cout << "Начальное состояние:\n";
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                std::cout << (grid[r][c] ? "██" : "  ");
            }
            std::cout << "\n";
        }
        std::cout << std::string(COLS * 2, '-') << "\n";
    }

    // Словарь «хеш -> номер итерации» для обнаружения циклов
    std::unordered_map<std::size_t, int> visited;
    // Запоминаем начальное состояние
    std::size_t h0 = hashGrid(grid);
    visited[h0] = 0;

    // Ограничение — до скольки итераций мы ищем цикл
    const int MAX_ITER = 2000;

    bool cycleFound = false;
    int cycleStart  = -1;
    int cycleLen    = -1;

    Grid current = grid;

    // Цикл итераций
    for (int iter = 1; iter <= MAX_ITER; iter++) {
        // Печать текущего состояния
        printGrid(current, iter - 1);

        // Считаем следующее поколение
        Grid newGrid = nextGeneration(current, debugMode);
        current = newGrid;

        // Хешируем
        std::size_t h = hashGrid(current);

        if (debugMode) {
            std::cout << "  [Debug] Хеш текущего состояния: " << h << "\n";
        }

        // Проверяем на повтор
        if (visited.find(h) != visited.end()) {
            // Цикл найден!
            cycleFound = true;
            cycleStart = visited[h];
            cycleLen   = iter - cycleStart;
            std::cout << "Найден цикл!\n"
                      << "Начало цикла на итерации " << cycleStart
                      << ", длина цикла: " << cycleLen << "\n";
            break;
        } else {
            visited[h] = iter;
        }

        // Опционально: подсчёт и вывод количества живых клеток
        if (debugMode) {
            int liveCells = 0;
            for (const auto &row : current) {
                for (auto cell : row) liveCells += cell;
            }
            std::cout << "  [Debug] Количество живых клеток: " << liveCells << "\n";
        }

        // Задержка для удобства наблюдения
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Печатаем последнее состояние, если цикл не найден
    if (!cycleFound) {
        printGrid(current, MAX_ITER);
    }

    // Решаем, "стоит ли строить" (вести автомат дальше)
    // Предположим, нам нужны циклы длиной ровно 10
    const int REQUIRED_CYCLE_LEN = 10;

    if (cycleFound) {
        if (cycleLen == REQUIRED_CYCLE_LEN) {
            std::cout << "Цикл подходит! Делаем дальнейшие построения.\n";
            // Пример: анимируем цикл несколько раз
            for (int i = 0; i < cycleLen; i++) {
                printGrid(current, cycleStart + i);
                current = nextGeneration(current, debugMode);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        } else {
            std::cout << "Цикл не подходит (нужен " << REQUIRED_CYCLE_LEN
                      << ", найден " << cycleLen << "). Останавливаемся.\n";
            return 0;
        }
    } else {
        std::cout << "Цикл не найден (за " << MAX_ITER << " итераций). "
                  << "Останавливаемся.\n";
        return 0;
    }

    // Дополнительный код, который продолжает работу с автоматом
    std::cout << "Автомат с нужной длиной цикла найден, продолжаем...\n";
    // ... ваш дальнейший код ...

    return 0;
}