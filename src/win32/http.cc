#include <http.h>

#include <map>     // std::map
#include <string>  // std::string
#include <thread>  // std::thread
#include <variant> // std::variant

namespace req {

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  std::thread([callback]() {
    std::variant<std::string, Response> result;
    result = Response{"Hello, world!", 200, {}};
    callback(std::move(result));
  }).detach();
}

} // namespace req
