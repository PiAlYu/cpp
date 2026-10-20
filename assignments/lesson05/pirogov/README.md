# Урок 5 — Pirogov

Это C++17-клиент для локального `llama-server`. Сценарий — простой чат с памятью. Клиент хранит историю сообщений, считает токены перед каждым ответом, удаляет старые пары при заполнении контекста и показывает статистику `usage`.

Программа не скачивает и не запускает модель самостоятельно. Перед запуском клиента сервер уже должен работать локально.

## Проверенное окружение

- ОС: Windows 11 Home Single Language, версия 10.0.26100;
- процессор: Intel Core i9-12900H, 14 ядер и 20 логических процессоров;
- оперативная память: 15,7 ГБ;
- видеокарта: NVIDIA GeForce RTX 3070 Ti Laptop GPU;
- режим задания: CPU, `-ngl 0`, два потока, GPU не использовался;
- `llama-server`: `0.7.0-dev`, build 1, commit `1b29621`;
- модель: [Qwen2.5-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/tree/main);
- файл: `qwen2.5-0.5b-instruct-q4_k_m.gguf`;
- квантование: Q4_K_M, размер около 469 МиБ.

## Возможности клиента

- адрес сервера и имя модели читаются из JSON-конфигурации;
- история хранится в `std::vector<Message>`;
- перед генерацией вызывается `/v1/chat/completions/input_tokens`;
- проверяется условие `input_tokens + max_tokens <= context_size`;
- при нехватке места удаляется самая старая пара `user`–`assistant`;
- системное сообщение и текущая реплика сохраняются;
- ответ читается из `choices[0].message.content`;
- статистика читается из `usage` без значений по умолчанию;
- `/reset` очищает историю, а `/exit` завершает программу;
- обрабатываются ошибка соединения, тайм-аут, HTTP-ошибка и неправильный JSON.

## Структура

- `include/chat.hpp` — структуры и объявления функций;
- `src/config.cpp` — чтение и проверка конфигурации;
- `src/chat.cpp` — JSON-запросы, ответы и сокращение истории;
- `src/http_client.cpp` — HTTP POST;
- `src/main.cpp` — диалог с пользователем;
- `tests/chat_test.cpp` — проверки логики без сервера;
- `examples/` — фактические запросы, ответы и результаты;
- `project_ideas.md` — идеи итогового проекта.

CMake создаёт библиотеку `chat_core`, программу `local_chat` и тест `chat_test`.

## Зависимости

Нужны Git, CMake 3.20 или новее и компилятор C++17. CMake загружает закреплённые версии:

- `nlohmann_json` 3.11.3;
- `cpp-httplib` 0.18.1.

Интернет нужен только при первой настройке сборки. Для обращения к локальному HTTP-серверу дополнительные библиотеки устанавливать не нужно.

## Подготовка llama-server

Сервер и модель нужно хранить вне репозитория курса. Пример команд для Windows в Developer PowerShell:

```powershell
git clone --depth 1 https://github.com/ggml-org/llama.cpp.git
cd llama.cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLLAMA_OPENSSL=OFF -DGGML_CUDA=OFF
cmake --build build --config Release --target llama-server -j 2
.\build\bin\Release\llama-server.exe --version
```

Создание каталога и скачивание только нужного GGUF-файла:

```powershell
New-Item -ItemType Directory -Force models
curl.exe -L --fail --output models\qwen2.5-0.5b-instruct-q4_k_m.gguf "https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf?download=true"
```

Запуск сервера на CPU:

```powershell
.\build\bin\Release\llama-server.exe -m models\qwen2.5-0.5b-instruct-q4_k_m.gguf --alias local-model -c 1024 -ngl 0 -t 2 -np 1 --host 127.0.0.1 --port 8080
```

Проверка в другом терминале:

```powershell
curl.exe -i http://127.0.0.1:8080/health
```

Ожидается HTTP 200 и `{"status":"ok"}`. Веб-интерфейс доступен по адресу `http://127.0.0.1:8080`.

## Сборка клиента

Из каталога `assignments/lesson05/pirogov`:

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Запуск клиента

```powershell
.\build\Debug\local_chat.exe .\config.example.json
```

Пример:

```text
Вы: Меня зовут Алексей. Запомни моё имя.
Модель: Ваше имя - Алексей.
Токены: вход 53, ответ 9, всего 62 / 1024
```

Полный проверенный диалог находится в [examples/results.md](examples/results.md). Его можно повторить входным файлом:

```powershell
cmd /c "build\Debug\local_chat.exe config.example.json < examples\session_input.txt"
```

Прямой HTTP-пример:

```powershell
curl.exe --fail-with-body --max-time 180 http://127.0.0.1:8080/v1/chat/completions -H "Content-Type: application/json" --data-binary "@examples/http_request.json"
```

## Конфигурация

В `config.example.json` указаны:

- `server_url` — адрес сервера;
- `model` — значение `--alias` сервера;
- `system_prompt` — системное сообщение;
- `context_size` — размер контекста сервера;
- `max_tokens` — максимальная длина ответа;
- `temperature` — случайность генерации;
- `timeout_seconds` — конечный тайм-аут ответа.

`context_size` должен совпадать с параметром `-c` сервера. В этой работе используются 1024 токена и `max_tokens = 64`.

## Проверка ошибки соединения

Остановите сервер сочетанием `Ctrl+C` и снова запустите клиент. После ввода сообщения программа должна вывести понятную ошибку и завершиться с кодом 1:

```text
Ошибка: сервер недоступен или истёк тайм-аут: Could not establish connection
```

## Ограничения

Модель 0.5B работает быстро, но иногда путает смысл, язык или грамматику. Пример с неправильной формулировкой сохранён в `examples/results.md`. Клиент проверяет формат ответа и размер контекста, но не может гарантировать правильность текста модели.
