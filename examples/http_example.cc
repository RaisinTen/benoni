#include <http.h>

#include <iostream>
#include <variant>

int main() {
  using req::request;
  using req::RequestOptions;
  using req::RequestOptionsBuilder;
  using req::Response;

  RequestOptionsBuilder request_options_builder;
  RequestOptions request_options = request_options_builder.build();

  std::cout << "before request" << std::endl;
  request("https://postman-echo.com/get", request_options,
          [](std::variant<std::string, Response> result) {
            // This callback is run in a background thread.
            if (std::holds_alternative<std::string>(result)) {
              std::cerr << "error: [" << std::get<std::string>(result) << "]"
                        << std::endl;
              return 1;
            }
            std::cout << "success" << std::endl;
          });
  std::cout << "after request" << std::endl;

  return 0;
}
