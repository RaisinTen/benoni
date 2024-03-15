#include <http.h>

#include <libsoup/soup.h>

#include <array>   // std::array
#include <cassert> // assert
#include <map>     // std::map
#include <sstream> // std::stringstream
#include <string>  // std::string
#include <variant> // std::variant

namespace req {
namespace {

struct AsyncHttpContext {
  SoupMessage *message;
  std::array<uint8_t, 2048> buffer;
  std::stringstream response;
  std::function<void(std::variant<std::string, Response>)> callback;

  ~AsyncHttpContext() { g_object_unref(message); }
};

auto stream_close_callback(GObject *source_object, GAsyncResult *res,
                           gpointer data) -> void {
  GInputStream *stream = G_INPUT_STREAM(source_object);
  auto async_http_context = static_cast<AsyncHttpContext *>(data);

  GError *error = nullptr;
  gboolean stream_closed = g_input_stream_close_finish(stream, res, &error);
  if (stream_closed == FALSE) {
    assert(error);
    g_object_unref(stream);
    auto callback = std::move(async_http_context->callback);
    delete async_http_context;
    callback(error->message);
    return;
  }

  g_object_unref(stream);

  std::map<std::string, std::string> headers;
  soup_message_headers_foreach(
      async_http_context->message->response_headers,
      [](const char *name, const char *value, gpointer user_data) {
        auto &headers =
            *static_cast<std::map<std::string, std::string> *>(user_data);
        headers[name] = value;
      },
      &headers);

  Response response{
      .body = async_http_context->response.str(),
      .status = static_cast<uint16_t>(async_http_context->message->status_code),
      .headers = std::move(headers)};
  auto callback = std::move(async_http_context->callback);
  delete async_http_context;
  callback(std::move(response));
}

auto stream_read_callback(GObject *source_object, GAsyncResult *res,
                          gpointer data) -> void {
  GInputStream *stream = G_INPUT_STREAM(source_object);
  auto async_http_context = static_cast<AsyncHttpContext *>(data);

  GError *error = nullptr;
  gssize bytes_read = g_input_stream_read_finish(stream, res, &error);
  if (bytes_read == -1) {
    assert(error);
    g_object_unref(stream);
    auto callback = std::move(async_http_context->callback);
    delete async_http_context;
    callback(error->message);
    return;
  }

  if (bytes_read == 0) {
    // end
    g_input_stream_close_async(stream, G_PRIORITY_DEFAULT, nullptr,
                               stream_close_callback, async_http_context);
    return;
  }

  assert(bytes_read > 0);

  for (gssize i = 0; i < bytes_read; ++i) {
    async_http_context->response << async_http_context->buffer[i];
  }

  g_input_stream_read_async(stream, async_http_context->buffer.data(),
                            async_http_context->buffer.max_size(),
                            G_PRIORITY_DEFAULT, nullptr, stream_read_callback,
                            async_http_context);
}

auto session_send_callback(GObject *object, GAsyncResult *result,
                           gpointer data) -> void {
  auto async_http_context = static_cast<AsyncHttpContext *>(data);

  GError *error = nullptr;
  GInputStream *stream =
      soup_session_send_finish(SOUP_SESSION(object), result, &error);
  if (!stream) {
    assert(error);
    auto callback = std::move(async_http_context->callback);
    delete async_http_context;
    callback(error->message);
    return;
  }

  g_input_stream_read_async(stream, async_http_context->buffer.data(),
                            async_http_context->buffer.max_size(),
                            G_PRIORITY_DEFAULT, nullptr, stream_read_callback,
                            async_http_context);
}

} // namespace

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  const char *method = nullptr;
  switch (options.method()) {
#define V(HTTP_METHOD)                                                         \
  case Method::HTTP_METHOD:                                                    \
    method = #HTTP_METHOD;                                                     \
    break;

    REQ_HTTP_METHODS(V)
#undef V
  }
  SoupMessage *message = soup_message_new(method, url.c_str());
  if (message == nullptr) {
    callback("The uri could not be parsed");
    return;
  }

  SoupSession *session = soup_session_new_with_options(SOUP_SESSION_USER_AGENT,
                                                       "Req/1.0", nullptr);
  auto async_http_context = new AsyncHttpContext{};
  async_http_context->message = message;
  async_http_context->callback = std::move(callback);
  soup_session_send_async(session, async_http_context->message,
                          G_PRIORITY_DEFAULT, session_send_callback,
                          async_http_context);
}

} // namespace req
