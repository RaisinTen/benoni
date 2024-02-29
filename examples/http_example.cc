#include <http.h>

#include <future>
#include <iostream>
#include <variant>

using req::Method;
using req::request;
using req::RequestOptions;
using req::RequestOptionsBuilder;
using req::Response;

auto request_promisified(
    const std::string &url,
    RequestOptions options = RequestOptionsBuilder().build())
    -> std::future<Response> {
  std::shared_ptr<std::promise<Response>> promise =
      std::make_shared<std::promise<Response>>();

  std::future<Response> future = promise->get_future();

  request(url, options, [promise](std::variant<std::string, Response> result) {
    // This callback is run in a background thread.
    if (std::holds_alternative<std::string>(result)) {
      promise->set_exception(std::make_exception_ptr(
          std::runtime_error(std::get<std::string>(result))));
      return;
    }
    promise->set_value(std::get<Response>(result));
  });

  return future;
}

int main() {
  std::cout << "before request" << std::endl;
  std::future<Response> response_future =
      request_promisified("https://postman-echo.com/get");
  try {
    Response response = response_future.get();
  } catch (const std::exception &exception) {
    std::cerr << "error: [" << exception.what() << "]" << std::endl;
    return 1;
  }
  std::cout << "after request" << std::endl;

  return 0;
}
