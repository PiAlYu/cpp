#include "chat.hpp"

#include <stdexcept>

namespace pirogov {

namespace {

int read_token_count(const nlohmann::json& data, const std::string& name) {
    if (!data.contains(name) || !data.at(name).is_number_integer()) {
        throw std::runtime_error("в ответе нет целого поля " + name);
    }

    int value = data.at(name).get<int>();
    if (value < 0) {
        throw std::runtime_error("поле " + name + " не может быть отрицательным");
    }
    return value;
}

} // namespace

nlohmann::json build_request(
    const AppConfig& config,
    const std::vector<Message>& messages
) {
    nlohmann::json request;
    request["model"] = config.model;
    request["temperature"] = config.temperature;
    request["max_tokens"] = config.max_tokens;
    request["stream"] = false;
    request["messages"] = nlohmann::json::array();

    for (const Message& message : messages) {
        nlohmann::json item;
        item["role"] = message.role;
        item["content"] = message.content;
        request["messages"].push_back(item);
    }

    return request;
}

int parse_input_tokens(const nlohmann::json& response) {
    if (!response.is_object()) {
        throw std::runtime_error("ответ подсчёта токенов должен быть объектом");
    }
    return read_token_count(response, "input_tokens");
}

ChatResult parse_chat_response(const nlohmann::json& response) {
    const char* text_error =
        "в ответе нет строки choices[0].message.content";

    if (!response.is_object() || !response.contains("choices")) {
        throw std::runtime_error(text_error);
    }

    const nlohmann::json& choices = response.at("choices");
    if (!choices.is_array() || choices.empty() || !choices.at(0).is_object()) {
        throw std::runtime_error(text_error);
    }

    const nlohmann::json& choice = choices.at(0);
    if (!choice.contains("message") || !choice.at("message").is_object()) {
        throw std::runtime_error(text_error);
    }

    const nlohmann::json& message = choice.at("message");
    if (!message.contains("content") || !message.at("content").is_string()) {
        throw std::runtime_error(text_error);
    }

    if (!response.contains("usage") || !response.at("usage").is_object()) {
        throw std::runtime_error("в ответе нет объекта usage");
    }

    const nlohmann::json& usage_data = response.at("usage");

    ChatResult result;
    result.text = message.at("content").get<std::string>();
    result.usage.prompt_tokens = read_token_count(usage_data, "prompt_tokens");
    result.usage.completion_tokens = read_token_count(usage_data, "completion_tokens");
    result.usage.total_tokens = read_token_count(usage_data, "total_tokens");
    return result;
}

bool remove_oldest_pair(std::vector<Message>& messages) {
    for (std::size_t index = 0; index + 1 < messages.size(); index++) {
        if (messages[index].role == "user" &&
            messages[index + 1].role == "assistant") {
            messages.erase(messages.begin() + index, messages.begin() + index + 2);
            return true;
        }
    }
    return false;
}

} // namespace pirogov
