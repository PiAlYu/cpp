#include "chat.hpp"

#include <httplib.h>

#include <stdexcept>
#include <string>

namespace pirogov {

namespace {

std::string http_post(
    const AppConfig& config,
    const std::string& path,
    const std::string& body
) {
    httplib::Client client(config.server_url);
    client.set_connection_timeout(5, 0);
    client.set_read_timeout(config.timeout_seconds, 0);
    client.set_write_timeout(5, 0);

    httplib::Result response = client.Post(path, body, "application/json");
    if (!response) {
        throw std::runtime_error(
            "сервер недоступен или истёк тайм-аут: " +
            httplib::to_string(response.error())
        );
    }
    if (response->status != 200) {
        throw std::runtime_error(
            "сервер вернул HTTP " + std::to_string(response->status) +
            ": " + response->body
        );
    }
    return response->body;
}

nlohmann::json parse_server_json(const std::string& text) {
    try {
        return nlohmann::json::parse(text);
    } catch (const nlohmann::json::parse_error&) {
        throw std::runtime_error("сервер вернул некорректный JSON");
    }
}

} // namespace

int request_input_tokens(
    const AppConfig& config,
    const std::vector<Message>& messages
) {
    std::string body = http_post(
        config,
        "/v1/chat/completions/input_tokens",
        build_request(config, messages).dump()
    );
    return parse_input_tokens(parse_server_json(body));
}

ChatResult request_chat(
    const AppConfig& config,
    const std::vector<Message>& messages
) {
    std::string body = http_post(
        config,
        "/v1/chat/completions",
        build_request(config, messages).dump()
    );
    return parse_chat_response(parse_server_json(body));
}

} // namespace pirogov
