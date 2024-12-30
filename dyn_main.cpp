#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <functional>
#include <sstream>
#include <algorithm> // для std::shuffle
#include <fstream>   // для работы с файлами
#include <cstring>   // для std::strcmp
#include <cstdlib>   // для std::atoi

// --------------------------------------------------------------
// ОПРЕДЕЛЕНИЕ ПАРАМЕТРОВ И ТИПОВ
// --------------------------------------------------------------

// Флаг режима отладки
static bool debugMode = false;

// Размер сетки (N x N). Будет определен на основе длины input_data.
static size_t N = 20; // Значение по умолчанию, может быть изменено

// Максимальное количество живых клеток. Не должно превышать N.
static size_t maxLive = 20; // Значение по умолчанию, может быть изменено

// Тип для координат клетки (строка, столбец)
using Cell = std::pair<size_t, size_t>;

// Пользовательская хеш-функция для Cell, используемая в unordered_set
struct CellHash {
    std::size_t operator()(const Cell& cell) const {
        // Комбинируем хеши строки и столбца с использованием простого множителя
        return std::hash<size_t>()(cell.first) * 31 + std::hash<size_t>()(cell.second);
    }
};

// Компаратор равенства для Cell
struct CellEq {
    bool operator()(const Cell& a, const Cell& b) const {
        return (a.first == b.first) && (a.second == b.second);
    }
};

// Разреженное представление сетки: множество живых клеток
using SparseGrid = std::unordered_set<Cell, CellHash, CellEq>;

// --------------------------------------------------------------
// ФУНКЦИЯ ЛОГИРОВАНИЯ
// --------------------------------------------------------------

// Логирует сообщения, если режим отладки включен
void logMessage(const std::string& msg) {
    if (debugMode) {
        std::cout << "[DEBUG] " << msg << "\n";
    }
}

// --------------------------------------------------------------
// ГЕНЕРАЦИЯ НАЧАЛЬНОГО СОСТОЯНИЯ
// --------------------------------------------------------------

// Генерирует начальную конфигурацию с не более чем maxLive живыми клетками
SparseGrid generateInitialConfiguration(size_t N, size_t maxLive, std::mt19937& gen) {
    SparseGrid grid;
    if (maxLive == 0) {
        return grid;
    }

    std::uniform_int_distribution<size_t> dist(0, N - 1);

    while (grid.size() < maxLive) {
        size_t r = dist(gen);
        size_t c = dist(gen);
        grid.emplace(r, c);
    }

    return grid;
}

// --------------------------------------------------------------
// ПОДСЧЕТ СОСЕДЕЙ
// --------------------------------------------------------------

// Подсчитывает количество живых соседей для данной клетки
int countNeighbors(const SparseGrid& grid, size_t r, size_t c, size_t N) {
    int cnt = 0;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            ssize_t rr = static_cast<ssize_t>(r) + dr;
            ssize_t cc = static_cast<ssize_t>(c) + dc;
            if (rr < 0 || rr >= static_cast<ssize_t>(N) || cc < 0 || cc >= static_cast<ssize_t>(N))
                continue;
            if (grid.find({static_cast<size_t>(rr), static_cast<size_t>(cc)}) != grid.end()) {
                cnt++;
            }
        }
    }
    return cnt;
}

// --------------------------------------------------------------
// ОПРЕДЕЛЕНИЕ СЛЕДУЮЩЕГО СОСТОЯНИЯ КЛЕТКИ
// --------------------------------------------------------------

// Определяет, будет ли клетка жива в следующем поколении
bool nextStateOfCell(const SparseGrid& grid, size_t r, size_t c, size_t N) {
    bool isAlive = (grid.find({r, c}) != grid.end());
    int neighbors = countNeighbors(grid, r, c, N);

    if (isAlive) {
        // Выживает, если у нее 2 или 3 живых соседа
        return (neighbors == 2 || neighbors == 3);
    } else {
        // Рождается, если у нее ровно 3 живых соседа
        return (neighbors == 3);
    }
}

// --------------------------------------------------------------
// ВЫЧИСЛЕНИЕ СЛЕДУЮЩЕГО ПОКОЛЕНИЯ
// --------------------------------------------------------------

// Вычисляет следующее поколение на основе текущей сетки
SparseGrid nextGeneration(const SparseGrid& current, size_t N) {
    SparseGrid next;
    std::unordered_map<Cell, int, CellHash, CellEq> neighborCount;
    neighborCount.reserve(current.size() * 9); // Грубая оценка

    // 1) Определяем всех кандидатов (живые клетки и их соседи)
    for (const auto& cell : current) {
        size_t r = cell.first;
        size_t c = cell.second;

        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                ssize_t rr = static_cast<ssize_t>(r) + dr;
                ssize_t cc = static_cast<ssize_t>(c) + dc;
                if (rr < 0 || rr >= static_cast<ssize_t>(N) || cc < 0 || cc >= static_cast<ssize_t>(N))
                    continue;
                Cell candidate = {static_cast<size_t>(rr), static_cast<size_t>(cc)};
                neighborCount[candidate] = 0; // Инициализируем счетчик
            }
        }
    }

    // 2) Подсчитываем количество живых соседей для каждого кандидата
    for (const auto& cell : current) {
        size_t r = cell.first;
        size_t c = cell.second;

        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                ssize_t rr = static_cast<ssize_t>(r) + dr;
                ssize_t cc = static_cast<ssize_t>(c) + dc;
                if (rr < 0 || rr >= static_cast<ssize_t>(N) || cc < 0 || cc >= static_cast<ssize_t>(N))
                    continue;
                Cell neighbor = {static_cast<size_t>(rr), static_cast<size_t>(cc)};
                auto it = neighborCount.find(neighbor);
                if (it != neighborCount.end()) {
                    it->second += 1;
                }
            }
        }
    }

    // 3) Определяем, какие клетки будут живыми в следующем поколении
    for (const auto& kv : neighborCount) {
        const Cell& cell = kv.first;
        int cnt = kv.second;
        bool aliveNow = (current.find(cell) != current.end());
        bool aliveNext;

        if (aliveNow) {
            aliveNext = (cnt == 2 || cnt == 3);
        } else {
            aliveNext = (cnt == 3);
        }

        if (aliveNext) {
            next.emplace(cell);
        }
    }

    return next;
}

// --------------------------------------------------------------
// ХЕШИРОВАНИЕ ВСЕГО СОСТОЯНИЯ СЕТКИ
// --------------------------------------------------------------

// Вычисляет хеш для текущего состояния сетки
std::size_t hashGrid(const SparseGrid& grid) {
    // Используем простое комбинирование хешей клеток с использованием простого простого множителя
    const std::size_t prime = 1000003;
    std::size_t h = 0;
    for (const auto& cell : grid) {
        std::size_t cellHash = CellHash{}(cell);
        h = h * prime + cellHash;
    }
    return h;
}

// --------------------------------------------------------------
// ВЫЧИСЛЕНИЕ ПРОСТОГО КОНТРОЛЬНОГО СУММЫ (НЕОБЯЗАТЕЛЬНО)
// --------------------------------------------------------------

// Вычисляет простую контрольную сумму для состояния сетки (необязательно, для верификации)
std::size_t computeChecksum(const SparseGrid& grid) {
    std::size_t checksum = 0;
    for (const auto& cell : grid) {
        checksum += cell.first * 31 + cell.second;
    }
    return checksum;
}

// --------------------------------------------------------------
// ОБРАБОТКА ПАРАМЕТРОВ КОМАНДНОЙ СТРОКИ
// --------------------------------------------------------------

// Обрабатывает аргументы командной строки для установки параметров
void parseArguments(int argc, char* argv[], std::string& inputDataFile, bool& debugModeFlag) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            inputDataFile = argv[++i];
        } else if (std::strcmp(argv[i], "--debug") == 0) {
            debugModeFlag = true;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Использование: " << argv[0] << " [--input <файл>] [--debug]\n";
            exit(0);
        } else {
            std::cerr << "Неизвестный аргумент: " << argv[i] << "\n";
            std::cout << "Использование: " << argv[0] << " [--input <файл>] [--debug]\n";
            exit(1);
        }
    }
}

// --------------------------------------------------------------
// ОСНОВНАЯ ФУНКЦИЯ
// --------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Инициализируем генератор случайных чисел с текущим временем для случайности
    std::mt19937 gen(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

    // Обрабатываем аргументы командной строки
    std::string inputDataFile = "";
    parseArguments(argc, argv, inputDataFile, debugMode);

    // Читаем input_data
    std::string input_data;
    if (!inputDataFile.empty()) {
        std::ifstream infile(inputDataFile);
        if (!infile) {
            std::cerr << "Ошибка: Невозможно открыть входной файл: " << inputDataFile << "\n";
            return 1;
        }
        std::ostringstream ss;
        ss << infile.rdbuf();
        input_data = ss.str();
        infile.close();
    } else {
        // Если файл не указан, запрашиваем ввод у пользователя или используем строку по умолчанию
        std::cout << "Введите строку input_data (или оставьте пустым для использования значения по умолчанию): ";
        std::getline(std::cin, input_data);
        if (input_data.empty()) {
            input_data = "DefaultInputData";
        }
    }

    // Устанавливаем размер сетки N и максимальное количество живых клеток maxLive на основе длины input_data
    N = input_data.size();
    maxLive = input_data.size();

    logMessage("Размер сетки установлен на " + std::to_string(N) + "x" + std::to_string(N));
    logMessage("Максимальное количество живых клеток установлено на " + std::to_string(maxLive));

    // Генерируем начальную конфигурацию
    auto current = generateInitialConfiguration(N, maxLive, gen);
    logMessage("Начальная конфигурация сгенерирована. Количество живых клеток = " + std::to_string(current.size()));

    // Подготовка к обнаружению цикла
    // Map: хеш -> номер итерации
    std::unordered_map<std::size_t, size_t> visited;
    visited.reserve(10000); // Резервируем место для 10,000 записей

    // Параметры
    size_t maxIterations = 2000; // Лимит итераций для поиска цикла
    bool cycleFound = false;
    size_t cycleStart = 0;
    size_t cycleLen = 0;

    // Мемоизация: сохраняем состояния на каждые 10 итераций
    std::unordered_map<size_t, SparseGrid> memoStates;
    memoStates.reserve(maxIterations / 10 + 1);

    // Цикл симуляции
    for (size_t iter = 0; iter < maxIterations; iter++) {
        // Логирование каждые 10 итераций
        if (iter % 10 == 0) {
            std::ostringstream oss;
            oss << "Итерация = " << iter << ", Живых клеток = " << current.size();
            logMessage(oss.str());
        }

        // Вычисляем хеш текущего состояния сетки
        std::size_t currentHash = hashGrid(current);

        // Проверяем, не встречался ли уже такой хеш
        auto it = visited.find(currentHash);
        if (it != visited.end()) {
            // Цикл обнаружен
            cycleFound = true;
            cycleStart = it->second;
            cycleLen = iter - cycleStart;
            logMessage("Цикл обнаружен! Начало цикла на итерации " + std::to_string(cycleStart) +
                       ", Длина цикла = " + std::to_string(cycleLen) +
                       ", Текущая итерация = " + std::to_string(iter));
            break;
        } else {
            // Сохраняем текущий хеш с номером итерации
            visited[currentHash] = iter;
        }

        // Мемоизация: сохраняем состояние каждые 10 итераций
        if (iter % 10 == 0) {
            memoStates[iter] = current;
            logMessage("Состояние на итерации " + std::to_string(iter) + " сохранено для мемоизации.");
        }

        // Вычисляем следующее поколение
        auto nxt = nextGeneration(current, N);
        current = std::move(nxt);

        // Проверяем и ограничиваем количество живых клеток до maxLive
        if (current.size() > maxLive) {
            size_t toRemove = current.size() - maxLive;
            logMessage("Количество живых клеток превышает maxLive. Удаляем " + std::to_string(toRemove) + " клеток.");

            // Сохраняем живые клетки в вектор для случайного удаления
            std::vector<Cell> cells(current.begin(), current.end());

            // Перемешиваем вектор для случайного порядка удаления
            std::shuffle(cells.begin(), cells.end(), gen);

            // Удаляем первые 'toRemove' клеток из перемешанного списка
            for (size_t i = 0; i < toRemove && i < cells.size(); ++i) {
                current.erase(cells[i]);
                logMessage("Удалена клетка (" + std::to_string(cells[i].first) + ", " +
                           std::to_string(cells[i].second) + ")");
            }
        }

        // Дополнительно: можно добавить паузу для наблюдения (опционально)
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Отчет после завершения симуляции
    if (cycleFound) {
        std::cout << "Цикл обнаружен!\n";
        std::cout << "Цикл начинается с итерации " << cycleStart << " и имеет длину " << cycleLen << ".\n";

        // Пример: доступ к состоянию на определенной итерации с использованием информации о цикле
        size_t queryIter;
        std::cout << "Введите номер итерации для получения состояния: ";
        std::cin >> queryIter;

        if (queryIter < cycleStart) {
            // Если запрашиваемая итерация была мемоизирована
            auto it_memo = memoStates.find(queryIter);
            if (it_memo != memoStates.end()) {
                std::cout << "Состояние на итерации " << queryIter << " получено из мемоизации.\n";
                // Здесь можно добавить код для отображения или обработки состояния
            } else {
                std::cout << "Состояние на итерации " << queryIter << " не было мемоизировано.\n";
            }
        } else {
            // Вычисляем эквивалентную итерацию внутри цикла
            size_t equivalentIter = cycleStart + ((queryIter - cycleStart) % cycleLen);
            auto it_memo = memoStates.find(equivalentIter);
            if (it_memo != memoStates.end()) {
                std::cout << "Состояние на итерации " << queryIter << " соответствует итерации " <<
                             equivalentIter << " внутри цикла.\n";
                // Здесь можно добавить код для отображения или обработки состояния
            } else {
                std::cout << "Эквивалентное состояние для итерации " << queryIter << " не было мемоизировано.\n";
            }
        }
    } else {
        std::cout << "Цикл не обнаружен за " << maxIterations << " итераций.\n";
        std::cout << "Состояния каждые 10 итераций были мемоизированы для быстрого доступа.\n";
    }

    // Отчет о финальном состоянии
    std::cout << "Финальное количество живых клеток: " << current.size() << "\n";
    if (debugMode) {
        std::cout << "Финальные живые клетки:\n";
        for (const auto& cell : current) {
            std::cout << "(" << cell.first << ", " << cell.second << ") ";
        }
        std::cout << "\n";
    }

    return 0;
}
