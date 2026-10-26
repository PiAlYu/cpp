# OpenCode + Qwen на суперкомпьютере МГУ

OpenCode — AI-агент для программирования, аналог Codex. Он умеет читать и изменять файлы проекта, запускать программы, компилировать код и исправлять ошибки.

Модель `qwen38_27b` уже работает на сервере МГУ, поэтому **устанавливать её на свой компьютер не нужно**.

### 1. Установка OpenCode

На macOS или Linux открыть терминал и выполнить:

```bash
curl -fsSL https://opencode.ai/v2/install | bash
```

После установки открыть новое окно терминала и проверить:

```bash
opencode --version
```

Для Windows можно использовать WSL с Ubuntu.

### 2. Настройка модели

Создать каталог и открыть файл конфигурации:

```bash
mkdir -p ~/.config/opencode
nano ~/.config/opencode/opencode.jsonc
```

Вставить:

```json
{
  "$schema": "https://opencode.ai/config.json",
  "model": "msu/qwen38_27b",
  "providers": {
    "msu": {
      "name": "MSU Supercomputer",
      "package": "@opencode/ai/providers/openai-compatible",
      "settings": {
        "baseURL": "https://contest.cs.msu.ru/llm/v1",
        "apiKey": "ВАШ_API_КЛЮЧ"
      },
      "models": {
        "qwen38_27b": {
          "name": "Qwen 27B",
          "modelID": "qwen38_27b",
          "capabilities": {
            "tools": true,
            "input": ["text"],
            "output": ["text"]
          }
        }
      }
    }
  },
  "permissions": [
    {"action": "shell", "resource": "*", "effect": "ask"},
    {"action": "edit", "resource": "*", "effect": "ask"}
  ]
}
```

Заменить `ВАШ_API_КЛЮЧ` на выданный ключ.

Сохранить файл: `Ctrl+O`, `Enter`, `Ctrl+X`.

Ограничить доступ к файлу:

```bash
chmod 600 ~/.config/opencode/opencode.jsonc
```

### 3. Запуск

Перейти в каталог своего проекта:

```bash
cd ~/myproject
```

Запустить агента:

```bash
opencode --standalone
```

### 4. Проверка работы

Написать агенту:

> Изучи структуру моего проекта на C++. Объясни, какие файлы за что отвечают. Ничего пока не изменяй.

Затем можно попробовать:

> Создай программу на C++, которая сортирует массив целых чисел. Скомпилируй её, запусти и проверь результат.

### Важно

- API-ключ нельзя публиковать или добавлять в Git.
- Перед разрешением команд и изменения файлов проверяйте действия агента.
- Агент может ошибаться, даже если программа успешно компилируется.
- Для доступа к серверу МГУ необходимо сетевое подключение, разрешающее обращения к `contest.cs.msu.ru`.

**Документация:** https://opencode.ai/v2/docs