#include <http.h>

#include <map>     // std::map
#include <string>  // std::string
#include <variant> // std::variant

namespace req {

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {}

} // namespace req
