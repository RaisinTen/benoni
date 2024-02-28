#ifndef REQ_HTTP_H_
#define REQ_HTTP_H_

#include <functional> // std::function
#include <map>        // std::map
#include <optional>   // std::optional
#include <string>     // std::string
#include <variant>    // std::variant

namespace req {

#define REQ_HTTP_METHODS(V)                                                    \
  V(GET)                                                                       \
  V(HEAD)                                                                      \
  V(POST) V(PUT) V(DELETE) V(CONNECT) V(OPTIONS) V(TRACE) V(PATCH)

enum class Method {
#define V(HTTP_METHOD) HTTP_METHOD,

  REQ_HTTP_METHODS(V)
#undef V
};

class RequestOptionsBuilder;

class RequestOptions {
public:
  Method method() const { return method_; }
  const std::string &body() const { return body_; }
  const std::map<std::string, std::string> &headers() const { return headers_; }
  const std::optional<int> &timeout() const { return timeout_; }

private:
  RequestOptions(Method method, std::string body,
                 std::map<std::string, std::string> headers,
                 std::optional<int> timeout)
      : method_{method}, body_{std::move(body)}, headers_{std::move(headers)},
        timeout_{std::move(timeout)} {}

  friend RequestOptionsBuilder;

  Method method_;
  std::string body_;
  std::map<std::string, std::string> headers_;
  std::optional<int> timeout_;
};

class RequestOptionsBuilder {
public:
  RequestOptionsBuilder &set_method(Method method) {
    method_ = method;
    return *this;
  }

  RequestOptionsBuilder &set_body(std::string body) {
    body_ = std::move(body);
    return *this;
  }

  RequestOptionsBuilder &
  set_headers(std::map<std::string, std::string> headers) {
    headers_ = std::move(headers);
    return *this;
  }

  RequestOptionsBuilder &set_timeout(int timeout) {
    timeout_ = timeout;
    return *this;
  }

  RequestOptions build() {
    return RequestOptions(method_, std::move(body_), std::move(headers_),
                          std::move(timeout_));
  }

private:
  Method method_ = Method::GET;
  std::string body_;
  std::map<std::string, std::string> headers_;
  std::optional<int> timeout_;
};

struct Response {
  std::string body;
  uint16_t status;
  std::map<std::string, std::string> headers;
};

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void;

} // namespace req

#endif
