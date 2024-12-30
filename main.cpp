#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <iomanip>    // Для std::setw
#include <chrono>
#include <thread>

// Размеры поля
static const int ROWS = 20;
static const int COLS = 20;

// Тип сетки
using Grid = std::vector<std::vector<int>>;

// Флаг для включения/отключения отладочного вывода
static bool debugMode = false;

// Простейшая функция хеширования сетки
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

// Правило "Игры Жизнь"
int transitionRule(int cell, int neighbors) {
    if (cell == 1) {
        // Живая клетка выживает при 2 или 3 соседях
        return (neighbors == 2 || neighbors == 3) ? 1 : 0;
    } else {
        // Мёртвая клетка оживает при ровно 3 соседях
        return (neighbors == 3) ? 1 : 0;
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

int main() {
    // ---------------------------
    // 1) ИНИЦИАЛИЗАЦИЯ АВТОМАТА
    // ---------------------------
    Grid grid(ROWS, std::vector<int>(COLS, 0));
    // Пример начальной конфигурации (глайдер, если хватает места)
    grid[1][0] = 1;
    grid[2][1] = 1;
    grid[0][2] = 1;
    grid[1][2] = 1;
    grid[2][2] = 1;

    if (debugMode) {
        std::cout << "Начальное состояние:\n";
        // Печать без очистки экрана
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                std::cout << (grid[r][c] ? "██" : "  ");
            }
            std::cout << "\n";
        }
        std::cout << std::string(COLS * 2, '-') << "\n";
    }

    // ---------------------------------------------------------
    // 2) СНАЧАЛА ПРОВЕРЯЕМ, ЕСТЬ ЛИ ЦИКЛ И ПОДХОДИТ ЛИ ОН НАМ
    // ---------------------------------------------------------
    // Словарь «хеш -> номер итерации», чтобы отследить повтор
    std::unordered_map<std::size_t, int> visited;
    // Запомним начальное состояние
    std::size_t h0 = hashGrid(grid);
    visited[h0] = 0;

    // Зададим ограничение — до скольки итераций мы ищем цикл
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
            // Цикл!
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

    // -----------------------------------------------------
    // 3) РЕШАЕМ, «СТОИТ ЛИ СТРОИТЬ» (ДАЛЬШЕ ВЕСТИ АВТОМАТ)
    // -----------------------------------------------------
    // Предположим, нам нужны циклы длиной ровно 10 (как пример).
    const int REQUIRED_CYCLE_LEN = 10;

    if (cycleFound) {
        if (cycleLen == REQUIRED_CYCLE_LEN) {
            std::cout << "Цикл подходит! Делаем дальнейшие построения.\n";
            // Здесь можно продолжить работу с автоматом, зная, что есть цикл нужной длины.
            // Например, анимировать цикл несколько раз:
            for (int i = 0; i < cycleLen; i++) {
                printGrid(current, cycleStart + i);
                current = nextGeneration(current, debugMode);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        } else {
            std::cout << "Цикл не подходит (нужен " << REQUIRED_CYCLE_LEN
                      << ", найден " << cycleLen << "). Останавливаемся.\n";
            // Можно выйти из программы или сделать что-то ещё.
            return 0;
        }
    } else {
        std::cout << "Цикл не найден (за " << MAX_ITER << " итераций). "
                  << "Останавливаемся.\n";
        // Или продолжать дальше «как есть».
        return 0;
    }

    // -----------------------------------------------------
    // 4) ЗДЕСЬ УЖЕ КОД, КОТОРЫЙ ПРОДОЛЖАЕТ РАБОТУ С АВТОМАТОМ
    // -----------------------------------------------------
    // Раз дошли сюда, значит цикл был нужной длины.
    // Можете, например, ещё раз его прогнать, показать,
    // сохранить, вывести в файл и т.п.

    std::cout << "Автомат с нужной длиной цикла найден, продолжаем...\n";
    // ... ваш дальнейший код ...

    return 0;
}