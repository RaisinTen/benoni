#include <http.h> // request

#include <stdio.h>  // popen, fgets, pclose
#include <stdlib.h> // atoi

#include <exception> // std::make_exception_ptr
#include <future>    // std::promise, std::future
#include <map>       // std::map
#include <stdexcept> // std::runtime_error
#include <string>    // std::string, std::to_string
#include <variant>   // std::variant

#include <gtest/gtest.h> // TEST, ASSERT_NE, ASSERT_EQ

using req::Method;
using req::request;
using req::RequestOptions;
using req::RequestOptionsBuilder;
using req::Response;

namespace {

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

} // namespace

TEST(http, http_basic) {
  // TODO(RaisinTen): Use spawn().
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response = request_promisified(url).get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_get) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  require('assert').strictEqual(req.method, 'GET');
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(
          url, RequestOptionsBuilder().set_method(Method::GET).build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

// TODO(RaisinTen): Add tests for the 'HEAD' HTTP method.

TEST(http, http_post) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  require('assert').strictEqual(req.method, 'POST');
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(
          url, RequestOptionsBuilder().set_method(Method::POST).build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_put) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  require('assert').strictEqual(req.method, 'PUT');
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(
          url, RequestOptionsBuilder().set_method(Method::PUT).build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_delete) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  require('assert').strictEqual(req.method, 'DELETE');
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(
          url, RequestOptionsBuilder().set_method(Method::DELETE).build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

// TODO(RaisinTen): Add tests for the 'CONNECT' HTTP method.

TEST(http, http_options) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  require('assert').strictEqual(req.method, 'OPTIONS');
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(
          url, RequestOptionsBuilder().set_method(Method::OPTIONS).build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_trace) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  require('assert').strictEqual(req.method, 'TRACE');
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(
          url, RequestOptionsBuilder().set_method(Method::TRACE).build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_patch) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  require('assert').strictEqual(req.method, 'PATCH');
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(
          url, RequestOptionsBuilder().set_method(Method::PATCH).build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_status) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(404, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: 'Hello World!',
  }));
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response = request_promisified(url).get();

  ASSERT_EQ(response.body, R"({"data":"Hello World!"})");

  ASSERT_EQ(response.status, 404);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_google) {
  const Response response = request_promisified("https://google.com").get();
  ASSERT_TRUE(response.body.starts_with("<!"));
  ASSERT_EQ(response.status, 200);
}

TEST(http, http_invalid_encoding) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
  res.end(Buffer.from([ 128 ]));
  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  std::future<Response> response = request_promisified(url);
  ASSERT_THROW(response.get(), std::runtime_error);

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_request_body) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  require('assert').strictEqual(req.method, 'POST');
  let body = [];
  req
    .on('data', (chunk) => {
      body.push(chunk);
    }).on('end', () => {
      body = Buffer.concat(body).toString();
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({
        data: body
      }));

      this.close();
      process.exit();
    });
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(url, RequestOptionsBuilder()
                                   .set_method(Method::POST)
                                   .set_body("Hello, world!")
                                   .build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"Hello, world!"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_request_headers) {
  FILE *pp = popen(R"(node -e \
"console.log(require('node:http').createServer(function(req, res) {
  require('assert').strictEqual(req.method, 'POST');
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({
    data: req.headers.lol + ' ' + req.headers.hello
  }));

  this.close();
  process.exit();
}).listen().address().port);"
)",
                   "r");
  ASSERT_NE(pp, nullptr);
  char buf[64];
  char *output = fgets(buf, sizeof(buf), pp);
  ASSERT_NE(output, nullptr);
  int port = atoi(output);
  const std::string url = "http://localhost:" + std::to_string(port);

  const Response response =
      request_promisified(
          url, RequestOptionsBuilder()
                   .set_method(Method::POST)
                   .set_headers({{"hello", "world"}, {"lol", "hey"}})
                   .build())
          .get();

  ASSERT_EQ(response.body, R"({"data":"hey world"})");

  ASSERT_EQ(response.status, 200);

  ASSERT_EQ(response.headers.size(), 5);
  ASSERT_EQ(response.headers.at("Connection"), "keep-alive");
  ASSERT_EQ(response.headers.at("Content-Type"), "application/json");
  ASSERT_FALSE(response.headers.at("Date").empty());
  ASSERT_EQ(response.headers.at("Keep-Alive"), "timeout=5");
  ASSERT_EQ(response.headers.at("Transfer-Encoding"), "Identity");

  ASSERT_EQ(pclose(pp), 0);
}

TEST(http, http_invalid_url) {
  std::future<Response> response =
      request_promisified("inv://a.lib-url.com?a=b#LOL");
  ASSERT_THROW(response.get(), std::runtime_error);
}

TEST(http, http_timeout) {
  std::future<Response> response =
      request_promisified("https://nghttp2.org/httpbin/delay/5",
                          RequestOptionsBuilder().set_timeout(2).build());
  ASSERT_THROW(response.get(), std::runtime_error);
}
