#include "chat.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

void check(bool condition, const std::string& name, int& errors) {
    if (!condition) {
        std::cerr << "Тест не пройден: " << name << '\n';
        errors++;
    }
}

int main() {
    int errors = 0;

    nlohmann::json config_json = {
        {"server_url", "http://127.0.0.1:8080"},
        {"model", "local-model"},
        {"system_prompt", "Отвечай кратко"},
        {"context_size", 1024},
        {"max_tokens", 64},
        {"temperature", 0.3},
        {"timeout_seconds", 180}
    };

    pirogov::AppConfig config = pirogov::parse_config(config_json);
    check(config.context_size == 1024, "чтение конфигурации", errors);

    std::vector<pirogov::Message> messages = {
        {"system", "Отвечай кратко"},
        {"user", "Привет"}
    };
    nlohmann::json request = pirogov::build_request(config, messages);
    check(request["model"] == "local-model", "модель в запросе", errors);
    check(request["messages"].size() == 2, "сообщения в запросе", errors);
    check(request["stream"] == false, "потоковый режим выключен", errors);

    nlohmann::json token_response = {{"input_tokens", 25}};
    check(pirogov::parse_input_tokens(token_response) == 25,
          "разбор числа токенов", errors);

    nlohmann::json chat_response = {
        {"choices", nlohmann::json::array({
            {{"message", {{"content", "Здравствуйте!"}}}}
        })},
        {"usage", {
            {"prompt_tokens", 25},
            {"completion_tokens", 4},
            {"total_tokens", 29}
        }}
    };
    pirogov::ChatResult result = pirogov::parse_chat_response(chat_response);
    check(result.text == "Здравствуйте!", "разбор текста ответа", errors);
    check(result.usage.total_tokens == 29, "разбор статистики", errors);

    std::vector<pirogov::Message> long_history = {
        {"system", "Система"},
        {"user", "Первый вопрос"},
        {"assistant", "Первый ответ"},
        {"user", "Новый вопрос"}
    };
    check(pirogov::remove_oldest_pair(long_history), "удаление старой пары", errors);
    check(long_history.size() == 2, "размер сокращённой истории", errors);
    check(long_history.front().role == "system", "системное сообщение сохранено", errors);
    check(long_history.back().content == "Новый вопрос", "новый вопрос сохранён", errors);

    bool bad_response_found = false;
    try {
        pirogov::parse_chat_response(nlohmann::json::object());
    } catch (const std::exception&) {
        bad_response_found = true;
    }
    check(bad_response_found, "ошибка структуры ответа", errors);

    if (errors == 0) {
        std::cout << "Все тесты пройдены\n";
    }
    return errors;
}
