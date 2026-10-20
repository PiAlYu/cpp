#include "chat.hpp"

#include <fstream>
#include <stdexcept>

namespace pirogov {

namespace {

std::string read_string(
    const nlohmann::json& data,
    const std::string& name,
    bool can_be_empty = false
) {
    if (!data.contains(name)) {
        throw std::runtime_error("нет поля " + name);
    }
    if (!data.at(name).is_string()) {
        throw std::runtime_error("поле " + name + " должно быть строкой");
    }

    std::string value = data.at(name).get<std::string>();
    if (!can_be_empty && value.empty()) {
        throw std::runtime_error("поле " + name + " не должно быть пустым");
    }
    return value;
}

int read_integer(const nlohmann::json& data, const std::string& name) {
    if (!data.contains(name)) {
        throw std::runtime_error("нет поля " + name);
    }
    if (!data.at(name).is_number_integer()) {
        throw std::runtime_error("поле " + name + " должно быть целым числом");
    }
    return data.at(name).get<int>();
}

} // namespace

AppConfig parse_config(const nlohmann::json& data) {
    if (!data.is_object()) {
        throw std::runtime_error("корень конфигурации должен быть объектом");
    }

    AppConfig config;
    config.server_url = read_string(data, "server_url");
    config.model = read_string(data, "model");
    config.system_prompt = read_string(data, "system_prompt", true);
    config.context_size = read_integer(data, "context_size");
    config.max_tokens = read_integer(data, "max_tokens");
    config.timeout_seconds = read_integer(data, "timeout_seconds");

    if (!data.contains("temperature") || !data.at("temperature").is_number()) {
        throw std::runtime_error("поле temperature должно быть числом");
    }
    config.temperature = data.at("temperature").get<double>();

    if (config.context_size < 128) {
        throw std::runtime_error("context_size должен быть не меньше 128");
    }
    if (config.max_tokens < 1 || config.max_tokens >= config.context_size) {
        throw std::runtime_error("max_tokens должен быть от 1 до context_size - 1");
    }
    if (config.temperature < 0.0 || config.temperature > 2.0) {
        throw std::runtime_error("temperature должна быть от 0 до 2");
    }
    if (config.timeout_seconds < 1) {
        throw std::runtime_error("timeout_seconds должен быть положительным");
    }

    return config;
}

AppConfig load_config(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("не удалось открыть конфигурацию: " + path);
    }

    nlohmann::json data;
    try {
        input >> data;
    } catch (const nlohmann::json::parse_error& error) {
        throw std::runtime_error(
            "ошибка синтаксиса JSON в конфигурации: " + std::string(error.what())
        );
    }

    return parse_config(data);
}

} // namespace pirogov
