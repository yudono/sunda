#ifndef SUNDA_WEBSERVER_LIB_H
#define SUNDA_WEBSERVER_LIB_H

#include "../../core/lang/interpreter.h"
#include "tcp_server.h"
#include "http_parser.h"
#include "../json/json_lib.h"
#include <map>
#include <functional>

namespace WebServer {

struct Route {
    std::string method;
    std::string path;
    Value handler; // Sunda closure
};

class ServerInstance {
public:
    std::vector<Route> routes;
    TCPServer server;

    void add_route(std::string method, std::string path, Value handler) {
        routes.push_back({method, path, handler});
    }

    void listen(int port, Interpreter& interpreter) {
        if (!server.start(port)) {
            std::cerr << "Failed to start server on port " << port << std::endl;
            return;
        }

        while (true) {
            int client_fd = server.accept_client();
            if (client_fd < 0) continue;

            std::string raw_req = server.read_request(client_fd);
            if (raw_req.empty()) {
                server.close_client(client_fd);
                continue;
            }

            HttpRequest req = HTTPParser::parse(raw_req);
            
            bool found = false;
            for (auto& route : routes) {
                if (route.method == req.method) {
                    std::map<std::string, std::string> params;
                    if (HTTPParser::match_route(route.path, req.path, params)) {
                        // Create Context object for Sunda
                        std::map<std::string, Value> ctx_map;
                        
                        // Context.req
                        std::map<std::string, Value> req_map;
                        req_map["path"] = Value(req.path, 0, false);
                        req_map["method"] = Value(req.method, 0, false);
                        req_map["body"] = Value(req.body, 0, false);
                        
                        // Context.req.param(name)
                        req_map["param"] = Value([params](std::vector<Value> args) -> Value {
                            if (args.empty()) return Value("undefined", 0, false);
                            std::string p = args[0].strVal;
                            if (params.count(p)) return Value(params.at(p), 0, false);
                            return Value("undefined", 0, false);
                        });

                        // Context.req.header(name)
                        req_map["header"] = Value([req](std::vector<Value> args) -> Value {
                            if (args.empty()) return Value("undefined", 0, false);
                            std::string h = args[0].strVal;
                            if (req.headers.count(h)) return Value(req.headers.at(h), 0, false);
                            return Value("undefined", 0, false);
                        });

                        // Context.req.json()
                        req_map["json"] = Value([req](std::vector<Value> args) -> Value {
                            JSONLib::JsonParser parser(req.body);
                            return parser.parse();
                        });

                        ctx_map["req"] = Value(req_map);

                        // Context.text(str)
                        ctx_map["text"] = Value([](std::vector<Value> args) -> Value {
                            std::string body = args.empty() ? "" : args[0].toString();
                            std::string res = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
                            return Value(res, 0, false);
                        });

                        // Context.json(obj)
                        ctx_map["json"] = Value([](std::vector<Value> args) -> Value {
                            std::string body = args.empty() ? "{}" : args[0].toJson();
                            std::string res = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
                            return Value(res, 0, false);
                        });

                        // Context.html(str)
                        ctx_map["html"] = Value([](std::vector<Value> args) -> Value {
                            std::string body = args.empty() ? "" : args[0].toString();
                            std::string res = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
                            return Value(res, 0, false);
                        });

                        // Context.header(contentType, body)
                        ctx_map["response"] = Value([](std::vector<Value> args) -> Value {
                            std::string type = args.size() > 0 ? args[0].strVal : "text/plain";
                            std::string body = args.size() > 1 ? args[1].toString() : "";
                            std::string res = "HTTP/1.1 200 OK\r\nContent-Type: " + type + "\r\nContent-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
                            return Value(res, 0, false);
                        });

                        // Call handler
                        std::vector<Value> args = { Value(ctx_map) };
                        Value result = interpreter.callClosure(route.handler, args);
                        
                        server.send_response(client_fd, result.strVal);
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                std::string body = "404 Not Found";
                std::string res = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
                server.send_response(client_fd, res);
            }

            server.close_client(client_fd);
        }
    }
};

// Singleton manager for multiple server instances if needed, but usually one is fine.
static std::vector<std::shared_ptr<ServerInstance>> g_servers;

void register_webserver(Interpreter& interpreter) {
    // Special hack for listen since it needs interpreter
    static Interpreter* s_interpreter = &interpreter; 
    
    interpreter.registerNative("WebServer_create", [](std::vector<Value> args) -> Value {
        auto instance = std::make_shared<ServerInstance>();
        g_servers.push_back(instance);
        
        std::map<std::string, Value> server_obj;
        
        server_obj["get"] = Value([instance](std::vector<Value> args) -> Value {
            if (args.size() < 2) return Value("", 0, false);
            instance->add_route("GET", args[0].strVal, args[1]);
            return Value("", 1, true); 
        });

        server_obj["post"] = Value([instance](std::vector<Value> args) -> Value {
            if (args.size() < 2) return Value("", 0, false);
            instance->add_route("POST", args[0].strVal, args[1]);
            return Value("", 1, true);
        });

        server_obj["put"] = Value([instance](std::vector<Value> args) -> Value {
            if (args.size() < 2) return Value("", 0, false);
            instance->add_route("PUT", args[0].strVal, args[1]);
            return Value("", 1, true);
        });

        server_obj["delete"] = Value([instance](std::vector<Value> args) -> Value {
            if (args.size() < 2) return Value("", 0, false);
            instance->add_route("DELETE", args[0].strVal, args[1]);
            return Value("", 1, true);
        });

        server_obj["patch"] = Value([instance](std::vector<Value> args) -> Value {
            if (args.size() < 2) return Value("", 0, false);
            instance->add_route("PATCH", args[0].strVal, args[1]);
            return Value("", 1, true);
        });

        server_obj["listen"] = Value([instance](std::vector<Value> args) -> Value {
            int port = 3000;
            if (!args.empty() && args[0].isMap) {
                auto it = args[0].mapVal->find("port");
                if (it != args[0].mapVal->end() && it->second.isInt) {
                    port = it->second.intVal;
                }
            }
            instance->listen(port, *s_interpreter);
            return Value("", 0, false);
        });

        return Value(server_obj);
    });
}

} // namespace WebServer

#endif
