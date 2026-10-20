#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace pirogov {

struct AppConfig {
    std::string server_url;
    std::string model;
    std::string system_prompt;
    int context_size;
    int max_tokens;
    double temperature;
    int timeout_seconds;
};

struct Message {
    std::string role;
    std::string content;
};

struct Usage {
    int prompt_tokens;
    int completion_tokens;
    int total_tokens;
};

struct ChatResult {
    std::string text;
    Usage usage;
};

AppConfig parse_config(const nlohmann::json& data);
AppConfig load_config(const std::string& path);

nlohmann::json build_request(
    const AppConfig& config,
    const std::vector<Message>& messages
);

int parse_input_tokens(const nlohmann::json& response);
ChatResult parse_chat_response(const nlohmann::json& response);
bool remove_oldest_pair(std::vector<Message>& messages);

int request_input_tokens(
    const AppConfig& config,
    const std::vector<Message>& messages
);

ChatResult request_chat(
    const AppConfig& config,
    const std::vector<Message>& messages
);

} // namespace pirogov
