#include <http.h>

#if defined(linux)
#include <glib.h>
#endif

#include <iostream>
#include <variant>

using req::Method;
using req::request;
using req::RequestOptions;
using req::RequestOptionsBuilder;
using req::Response;

int main() {
  GMainLoop *loop = g_main_loop_new(nullptr, FALSE);

  std::cout << "before request" << std::endl;

  request("https://postman-echo.com/get", RequestOptionsBuilder{}.build(),
          [](std::variant<std::string, Response> result) {
            std::cout << "inside callback" << std::endl;

            // This callback is run in a background thread.
            if (std::holds_alternative<std::string>(result)) {
              std::cerr << "error: [" << std::get<std::string>(result) << "]"
                        << std::endl;
              exit(EXIT_FAILURE);
            }

            Response response{std::get<Response>(result)};

            std::cout << "response body: [" << response.body << "]"
                      << std::endl;
            std::cout << "response status: [" << response.status << "]"
                      << std::endl;
            exit(EXIT_SUCCESS);
          });

  std::cout << "after request" << std::endl;

#if defined(linux)
  g_main_loop_run(loop);
  g_main_loop_unref(loop);
#else
  while (true)
    ;
#endif

  return 0;
}
