#include <benoni/http.h>

#if defined(linux)
#include <glib.h>
#endif

#include <iostream>
#include <variant>

using benoni::Method;
using benoni::request;
using benoni::RequestOptions;
using benoni::RequestOptionsBuilder;
using benoni::Response;

int main() {
#if defined(linux)
  GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
#endif

  std::string url{"https://postman-echo.com/get"};
  std::cout << "sending request to: \"" << url << "\"" << std::endl;

  request(std::move(url), RequestOptionsBuilder{}.build(),
          [](std::variant<std::string, Response> result) {
            // This callback is run in a background thread on Windows and Apple
            // and on the main thread on Gtk Linux.
            if (std::holds_alternative<std::string>(result)) {
              std::cerr << "error: [" << std::get<std::string>(result) << "]"
                        << std::endl;
              exit(EXIT_FAILURE);
            }

            Response response{std::get<Response>(result)};

            std::cout << "response status: " << response.status << std::endl;
            std::cout << "response headers: [" << std::endl;
            for (const auto &[key, value] : response.headers) {
              std::cout << "  \"" << key << "\": \"" << value << "\","
                        << std::endl;
            }
            std::cout << "]" << std::endl;
            std::cout << "response body: \"" << response.body << "\""
                      << std::endl;
            exit(EXIT_SUCCESS);
          });

#if defined(linux)
  g_main_loop_run(loop);
  g_main_loop_unref(loop);
#else
  while (true)
    ;
#endif

  return 0;
}
