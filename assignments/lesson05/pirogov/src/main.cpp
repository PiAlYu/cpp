#include "chat.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: local_chat <config.json>\n";
        return 1;
    }

    try {
        pirogov::AppConfig config = pirogov::load_config(argv[1]);
        std::vector<pirogov::Message> history;

        if (!config.system_prompt.empty()) {
            history.push_back({"system", config.system_prompt});
        }

        std::cout << "Локальный чат запущен. Команды: /reset и /exit.\n";

        while (true) {
            std::cout << "Вы: ";
            std::string text;
            if (!std::getline(std::cin, text)) {
                break;
            }

            if (text == "/exit") {
                break;
            }
            if (text == "/reset") {
                history.clear();
                if (!config.system_prompt.empty()) {
                    history.push_back({"system", config.system_prompt});
                }
                std::cout << "История очищена.\n";
                continue;
            }
            if (text.find_first_not_of(" \t\r") == std::string::npos) {
                std::cout << "Введите непустую строку.\n";
                continue;
            }

            history.push_back({"user", text});

            try {
                int removed_pairs = 0;
                bool message_is_too_long = false;

                while (true) {
                    int input_tokens = pirogov::request_input_tokens(config, history);
                    if (input_tokens + config.max_tokens <= config.context_size) {
                        break;
                    }
                    if (!pirogov::remove_oldest_pair(history)) {
                        message_is_too_long = true;
                        break;
                    }
                    removed_pairs++;
                }

                if (message_is_too_long) {
                    history.pop_back();
                    std::cout << "Сообщение слишком длинное. Сократите его.\n";
                    continue;
                }

                if (removed_pairs > 0) {
                    std::cout << "Удалено старых пар: " << removed_pairs << '\n';
                }

                pirogov::ChatResult result = pirogov::request_chat(config, history);
                history.push_back({"assistant", result.text});

                std::cout << "Модель: " << result.text << '\n';
                std::cout << "Токены: вход " << result.usage.prompt_tokens
                          << ", ответ " << result.usage.completion_tokens
                          << ", всего " << result.usage.total_tokens
                          << " / " << config.context_size << "\n";
            } catch (...) {
                if (!history.empty() && history.back().role == "user") {
                    history.pop_back();
                }
                throw;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
