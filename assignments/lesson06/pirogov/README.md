# Урок 6 — Pirogov

Это простой coding agent на Python. Он отправляет задачу локальной LLM,
получает tool calls, выполняет разрешённые действия и возвращает результат
модели. Агент умеет читать и записывать файлы, собирать CMake-проект,
запускать тесты и программу.

## Структура

- `agent.py` — агент и его инструменты;
- `.env.example` — пример настроек подключения к LLM;
- `requirements.txt` — зависимости Python;
- `demo/` — небольшой C++ проект, который агент исправил;
- `REPORT.md` — ход работы, ответы на вопросы и проверка lesson 5.

## Ограничения безопасности

Агент работает только внутри каталога, переданного в `--project`. Он не может
читать `.git`, `.venv` и `.env`. Команды shell с операторами `;`, `&&`, `||`,
`|`, `>`, `<` запрещены. Разрешены только `cmake`, `ctest` и запуск программы
из каталога `build`. У каждой команды есть тайм-аут 60 секунд.

## Подготовка

Нужны Python 3.10 или новее, CMake, C++ компилятор и локальный сервер с
OpenAI-совместимым API.

Команды для PowerShell:

```powershell
cd assignments\lesson06\pirogov
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
Copy-Item .env.example .env
```

После этого надо открыть `.env` и указать адрес, ключ и имя своей модели:

```text
OPENAI_API_KEY=local
OPENAI_BASE_URL=http://127.0.0.1:8080/v1
OPENAI_MODEL=qwen38_27b
```

Файл `.env` не добавляется в Git.

## Запуск на демонстрационном проекте

Сначала надо запустить локальный сервер модели. Затем в другом окне
PowerShell выполнить:

```powershell
.\.venv\Scripts\python.exe .\agent.py --project .\demo
```

Текст задания:

```text
Build this C++ project. If the build fails, find the reason, fix it and run the program.
```

Агент нашёл пропущенную точку с запятой, исправил `demo/main.cpp`, повторил
сборку и запустил программу. Результат программы:

```text
42
```

Проект можно проверить и без агента:

```powershell
cmake -S .\demo -B .\demo\build
cmake --build .\demo\build --config Debug
.\demo\build\Debug\demo_app.exe
```

## Запуск на другом проекте

В `--project` можно передать любой CMake-проект:

```powershell
.\.venv\Scripts\python.exe .\agent.py --project C:\path\to\project
```

Пример задания для проверки:

```text
Study the project, build it and run tests. Do not change files unless the build or tests fail.
```

Агент печатает каждый tool call и его результат, поэтому действия можно
проверять во время работы. Агент не заменяет проверку человеком: предложения
модели могут быть неточными.

