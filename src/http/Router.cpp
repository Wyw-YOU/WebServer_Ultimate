#include "http/Router.hpp"

void Router::Get(const std::string& path, Handler handler)
{
    getHandlers_[path] = std::move(handler);
}

void Router::Post(const std::string& path, Handler handler)
{
    postHandlers_[path] = std::move(handler);
}

bool Router::Route(const HttpRequest& req, HttpResponse& resp)
{
    
    if(req.MethodType() == HttpMethod::GET)
    {
        auto it = getHandlers_.find(req.Path());

        if(it == getHandlers_.end())
            return false;

        it->second(req, resp);
        return true;
    }

    if(req.MethodType() == HttpMethod::POST)
    {
        auto it = postHandlers_.find(req.Path());

        if(it == postHandlers_.end())
            return false;

        it->second(req, resp);
        return true;
    }

    return false;
}