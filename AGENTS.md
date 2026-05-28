# Project Rules

## Запрещено
- **Exception handling**: не использовать `try`/`catch`/`throw`. Код должен быть noexcept-friendly.
- **ECS**: не использовать Entity-Component-System архитектуру.
- **Динамический полиморфизм через виртуальные функции** — запрещён в hot-path. Только через CRTP, std::variant, или явные таблицы функций.
- **std::shared_ptr в hot-path**: только std::unique_ptr или raw owning pointers.

## Требования к коду
- **C++20/26**, без исключений.
- **Явный noexcept** на всех функциях, где возможно.
- **constexpr/consteval** где возможно.
- **RAII** для всех ресурсов.
- **OpenGL 4.5+ core profile**, только DSA (Direct State Access).
- **Ручное управление памятью** через кастомные аллокаторы где нужна производительность.
- **Flat структуры данных** вместо иерархических (SoA, ECS-запрещён, но ручная SoA норм).

## Стиль
- Имена файлов: `snake_case`.
- Классы: `PascalCase`.
- Глобальные Функции/методы: `PascalCase`.
- Локальные встроенные функции/приватные методы: `camelCase`.
- Макросы: `UPPER_SNAKE_CASE`.
- Только табуляция в отступах кода
- Без `using namespace std;`.
