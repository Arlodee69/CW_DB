# CW_DB — Курсовой проект по системному программированию

## Описание проекта

Данный проект представляет собой реализацию упрощённой системы управления базами данных (СУБД) на языке C++14.

СУБД поддерживает:

* хранение данных в файловой системе;
* SQL-подобный язык запросов;
* работу с типами `int` и `string`;
* DDL: создание и удаление баз данных и таблиц;
* DML: вставку, обновление, удаление и выборку данных;
* индексацию данных на основе B-Tree/B+Tree/B*Tree/B*+Tree;
* оптимизацию запросов через индексы;
* Условия выборки: ==, !=, <, >, <=, >=, BETWEEN, LIKE
* Алиасы столбцов в SELECT ... AS
* валидацию запросов и ограничений целостности;
* вывод результатов выборки в формате JSON.

---

# Структура проекта

```
CW_DB/
│
├── CMakeLists.txt              # Конфигурация сборки проекта
├── README.md                   # Описание проекта
├── .gitignore
│
├── build/                      # Директория сборки (генерируется CMake)
│
├── data/                       # Файлы баз данных
│   └── .gitkeep
│
├── docs/                       # Документация и пояснительная записка
│
├── scripts/                    # SQL-скрипты для тестирования
│   ├── create_db.sql
│   ├── insert_test.sql
│   └── select_test.sql
│
├── tests/                      # Модульные тесты
│   ├── parser_tests.cpp
│   ├── btree_tests.cpp
│   └── storage_tests.cpp
│
├── include/                    # Заголовочные файлы
│   │
│   ├── core/                   # Основные сущности СУБД
│   │   ├── DatabaseEngine.hpp
│   │   ├── Database.hpp
│   │   ├── Table.hpp
│   │   ├── Column.hpp
│   │   ├── Row.hpp
│   │   └── Types.hpp
│   │
│   ├── parser/                 # Лексер и парсер SQL
│   │   ├── Lexer.hpp
│   │   ├── Parser.hpp
│   │   ├── Token.hpp
│   │   ├── AST.hpp
│   │   └── Validator.hpp
│   │
│   ├── query/                  # Выполнение запросов
│   │   ├── QueryExecutor.hpp
│   │   ├── ConditionEvaluator.hpp
│   │   └── ResultSet.hpp
│   │
│   ├── storage/                # Работа с файловой системой
│   │   ├── FileManager.hpp
│   │   ├── Serializer.hpp
│   │   ├── MetadataManager.hpp
│   │   └── PageManager.hpp
│   │
│   ├── index/                  # Индексные структуры
│   │   ├── BTreeNode.hpp
│   │   ├── BPlusTree.hpp
│   │   └── IndexManager.hpp
│   │
│   ├── utils/                  # Вспомогательные компоненты
│   │   ├── JsonFormatter.hpp
│   │   ├── Logger.hpp
│   │   ├── StringUtils.hpp
│   │   └── FileUtils.hpp
│   │
│   └── exceptions/             # Пользовательские исключения
│       ├── SqlException.hpp
│       ├── ParserException.hpp
│       ├── StorageException.hpp
│       └── ConstraintException.hpp
│
├── src/                        # Реализация исходного кода
│   │
│   ├── main.cpp
│   │
│   ├── core/
│   │   ├── DatabaseEngine.cpp
│   │   ├── Database.cpp
│   │   ├── Table.cpp
│   │   ├── Column.cpp
│   │   └── Row.cpp
│   │
│   ├── parser/
│   │   ├── Lexer.cpp
│   │   ├── Parser.cpp
│   │   ├── AST.cpp
│   │   └── Validator.cpp
│   │
│   ├── query/
│   │   ├── QueryExecutor.cpp
│   │   ├── ConditionEvaluator.cpp
│   │   └── ResultSet.cpp
│   │
│   ├── storage/
│   │   ├── FileManager.cpp
│   │   ├── Serializer.cpp
│   │   ├── MetadataManager.cpp
│   │   └── PageManager.cpp
│   │
│   ├── index/
│   │   ├── BTreeNode.cpp
│   │   ├── BPlusTree.cpp
│   │   └── IndexManager.cpp
│   │
│   ├── utils/
│   │   ├── JsonFormatter.cpp
│   │   ├── Logger.cpp
│   │   ├── StringUtils.cpp
│   │   └── FileUtils.cpp
│   │
│   └── exceptions/
│       └── SqlException.cpp
│
└── third_party/                # Внешние библиотеки
    ├── json/                   # nlohmann/json
    └── googletest/             # GoogleTest
```

---

# Описание основных компонентов

## core/

Основные сущности СУБД.

### DatabaseEngine

Главный управляющий компонент системы. Отвечает за:

* загрузку баз данных;
* переключение между БД (`USE`);
* обработку запросов;
* взаимодействие между подсистемами.

### Database

Представляет одну базу данных и хранит:

* список таблиц;
* метаданные базы.

### Table

Описывает таблицу:

* схему таблицы;
* строки данных;
* индексы.

### Row

Строка таблицы.

### Column

Описание столбца:

* имя;
* тип данных;
* ограничения (`NOT_NULL`, `INDEXED`).

---

## parser/

Подсистема обработки SQL-запросов.

### Lexer

Разбивает входной SQL-текст на токены.

### Parser

Строит AST (Abstract Syntax Tree).

### Validator

Проверяет:

* корректность запросов;
* существование таблиц;
* соответствие типов;
* ограничения целостности.

---

## query/

Подсистема выполнения запросов.

### QueryExecutor

Выполняет SQL-команды:

* CREATE;
* INSERT;
* SELECT;
* UPDATE;
* DELETE.

### ConditionEvaluator

Обрабатывает условия `WHERE`.

---

## storage/

Подсистема файлового хранения данных.

### FileManager

Работа с файлами таблиц и баз данных.

### Serializer

Сериализация и десериализация данных.

### MetadataManager

Хранение схем таблиц и служебной информации.

### PageManager

Управление страницами хранения данных.

---

## index/

Подсистема индексов.

### BTree / BPlusTree

Реализация индексной структуры данных.

Поддерживаются операции:

* вставки;
* удаления;
* поиска;
* диапазонного поиска.

### IndexManager

Управляет индексами таблиц.

---

## utils/

Вспомогательные компоненты:

* логирование;
* работа со строками;
* форматирование JSON;
* файловые утилиты.

---

## exceptions/

Пользовательские исключения и обработка ошибок.

---

# Формат хранения данных

```text
data/
└── database_name/
    ├── metadata.bin
    │
    └── table_name/
        ├── schema.bin
        ├── rows.bin
        │
        └── indexes/
            └── id.idx
```

---

# Режимы запуска

## Интерактивный режим

```bash
./cw_db
```

Команды вводятся пользователем через терминал.

---

## Пакетный режим

```bash
./cw_db script.sql
```

Команды считываются из SQL-файла.

---

# Поддерживаемые команды

## Работа с базами данных

```sql
CREATE DATABASE db_name;
DROP DATABASE db_name;
USE db_name;
```

---

## Работа с таблицами

```sql
CREATE TABLE users (
    id int INDEXED,
    name string NOT_NULL
);

DROP TABLE users;
```

---

## Работа с данными

```sql
INSERT INTO users (id, name) VALUE (1, "Alex");

-- Пропущенные nullable-колонки инициализируются NULL
INSERT INTO users (id, name) VALUE (2, "Bob");

-- Несколько строк за один запрос
INSERT INTO users (id, name) VALUE (3, "Carol"), (4, "Dave");

SELECT * FROM users;

-- Конкретные колонки с алиасами
SELECT id, name AS username FROM users;

-- С условием
SELECT * FROM users WHERE id == 1;

-- Диапазон (включает левую границу, исключает правую: [2, 5))
SELECT * FROM users WHERE id BETWEEN 2 AND 5;

-- Регулярное выражение
SELECT * FROM users WHERE name LIKE "A.*";

-- Обращение через database_name.table_name (без USE)
SELECT * FROM mydb.users WHERE id > 0;

UPDATE users SET name = "Bob" WHERE id == 1;

DELETE FROM users WHERE id == 1;
```

---

# Участники
Участник1 - Зона ответственности—Ядро, файловое хранение, B-Tree индексы, сериализация
Участник2 - Парсер, лексер, Query Engine, условия WHERE, JSON-вывод

# План разработки

## Этап 1

* настройка структуры проекта;
* CMake;
* интерактивный режим;
* lexer.

## Этап 2

* parser;
* AST;
* базовые DDL-команды.

## Этап 3

* файловое хранение;
* сериализация данных.

## Этап 4

* INSERT;
* SELECT;
* JSON output.

## Этап 5

* WHERE;
* условные выражения.

## Этап 6

* индексирование;
* B+Tree.

## Этап 7

* UPDATE;
* DELETE.

## Этап 8

* тестирование;
* оптимизация.

---

# Используемые технологии

* C++14
* STL
* CMake
* nlohmann/json
* GoogleTest

---

# Требования

* C++14 или выше
* CMake 3.10+
* GCC / Clang / MSVC

---

# Сборка проекта

```bash
mkdir build
cd build

cmake ..
cmake --build .
```

---

# Запуск

```bash
./cw_db
```

или

```bash
./cw_db script.sql
```
