#ifndef SUNDA_HTTP_PARSER_H
#define SUNDA_HTTP_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <regex>

namespace WebServer {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params; // For URL params like :name
};

class HTTPParser {
public:
    static HttpRequest parse(const std::string& raw) {
        HttpRequest req;
        std::istringstream stream(raw);
        std::string line;

        // Request Line: GET /path HTTP/1.1
        if (std::getline(stream, line)) {
            std::istringstream line_stream(line);
            line_stream >> req.method >> req.path;
        }

        // Headers
        while (std::getline(stream, line) && line != "\r" && line != "") {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 2); // skip ": "
                if (!value.empty() && value.back() == '\r') value.pop_back();
                req.headers[key] = value;
            }
        }

        // Body
        if (req.headers.count("Content-Length")) {
            size_t length = std::stoul(req.headers["Content-Length"]);
            std::vector<char> body_buf(length);
            stream.read(body_buf.data(), length);
            req.body = std::string(body_buf.data(), length);
        } else {
             // Fallback for smaller/chunked? For now keep it simple.
             std::stringstream body_stream;
             body_stream << stream.rdbuf(); 
             req.body = body_stream.str();
        }

        return req;
    }

    // Helper to match route with params
    static bool match_route(const std::string& pattern, const std::string& actual_path, std::map<std::string, std::string>& params) {
        // Simple router: /hello/:name
        // Convert to regex: /hello/([^/]+)
        
        std::string regex_pattern = pattern;
        std::vector<std::string> param_names;
        
        std::regex param_regex(":([a-zA-Z0-9]+)");
        auto words_begin = std::sregex_iterator(pattern.begin(), pattern.end(), param_regex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            param_names.push_back(match[1].str());
        }

        regex_pattern = std::regex_replace(pattern, param_regex, "([^/]+)");
        regex_pattern = "^" + regex_pattern + "$";

        std::regex full_regex(regex_pattern);
        std::smatch match_results;
        if (std::regex_match(actual_path, match_results, full_regex)) {
            for (size_t i = 0; i < param_names.size(); i++) {
                params[param_names[i]] = match_results[i + 1].str();
            }
            return true;
        }

        return false;
    }
};

} // namespace WebServer

#endif
