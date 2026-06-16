#pragma once

#include <functional>
#include <unordered_map>

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "Log.hpp"

class Router
{
public:
    using Handler = std::function<void(const HttpRequest&, HttpResponse&)>;

    void Get(const std::string& path, Handler handler);
    void Post(const std::string& path, Handler handler);
    bool Route(const HttpRequest& request, HttpResponse& response);

private:
    std::unordered_map<std::string, Handler> getHandlers_;
    std::unordered_map<std::string, Handler> postHandlers_;
};