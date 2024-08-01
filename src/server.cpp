#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <sstream>
#include <map>
#include <thread>



std::vector<std::string> split_string(const std::string &str, const std::string &delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, *delimiter.begin())) {
        result.push_back(item);
    }

    return result;
}

std::string get_path(std::string request) {
  std::vector<std::string> toks = split_string(request, "\r\n");
  std::vector<std::string> path_toks = split_string(toks[0], " ");
  return path_toks[1];
}

std::map<std::string, std::string> get_headers(std::string request) {
  std::map<std::string, std::string> headers;
  std::vector<std::string> toks = split_string(request, "\r\n");
  for (int i = 1; i < toks.size(); i++) {
    std::cout << toks[i] << "\n";
    size_t pos = toks[i].find(":");
    if (pos == std::string::npos) {
      break;
    }
    std::vector<std::string> header_toks = split_string(toks[i], ": ");
    std::cout << header_toks[0] << " " << header_toks[1] << "\n";
    headers[header_toks[0]] = header_toks[1];
  }
  return headers;
}

struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

HTTPRequest parse_request(std::string request) {
    HTTPRequest req;
    std::stringstream ss(request);
    std::string line;
    std::getline(ss, line);
    std::istringstream line_ss(line);
    line_ss >> req.method >> req.path >> req.version;
    while (std::getline(ss, line) && !line.empty()) {
        size_t pos = line.find(":");
        if (pos != std::string::npos) {
            std::string header_name = line.substr(0, pos);
            std::string header_value = line.substr(pos + 2);
            header_value.erase(header_value.end() - 1);
            req.headers[header_name] = header_value;
        }
    }
    std::stringstream body_ss;
    std::getline(ss, line);
    body_ss << line;
    while (std::getline(ss, line)) {
        body_ss << line << "\n";
        body_ss << "\n" << line;
    }
    req.body = body_ss.str();
    return req;
}

int handle_client(int client_fd, struct sockaddr_in client_addr) {
  char buff[2048];
  int client_addr_len = sizeof(client_addr);
  // use ssize_t if return value can be negative (i.e. error value)
  // reading from file descriptor here, another way is just int ret = read(client_fd, buffer, sizeof(buffer));
  // use recv for TCP, recvfrom for UDP (UDP is connectionless)
  ssize_t t = recv(client_fd, buff, sizeof(buff) - 1, 0);
  if (t < 0){
    std::cerr << "listen failed\n";
    return 1;
  }

  std::string r(buff);
  std::cout << client_fd << "\n";
  std::cout << r << "\n";
  HTTPRequest request = parse_request(r);

  std::string response;
  if (request.method == "GET") {
    if (request.path == "/") {
      response = "HTTP/1.1 200 OK\r\n\r\n";
    } else if (request.path == "/user-agent") {
      std::string body = request.headers["User-Agent"];
      response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
    } else if (request.path.substr(0, 6) == "/echo/") {
      std::string subStr = request.path.substr(6);
      response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(subStr.length()) + "\r\n\r\n" + subStr;
    } else if (request.path.substr(0, 7) == "/files/") {
        // std::string directory = ".";
        // std::string filename = request.path.substr(7);
        // std::ifstream file(directory + "/" + filename);
        // if (!file) {
        //     HTTPResponse response = { "HTTP/1.1 404 Not Found", "text/plain", {}, "Not Found" };
        //     write_response(client_fd, response);
        //     return;
        // }
        // std::stringstream body;
        // body << file.rdbuf();
        // HTTPResponse response = { "HTTP/1.1 200 OK", "application/octet-stream", { {"Content-Length", std::to_string(body.str().length())} }, body.str() };
        // write_response(client_fd, response);
    } else {
      response = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
  } else {
      // HTTPResponse response = { "HTTP/1.1 405 Method Not Allowed", "text/plain", {}, "Method Not Allowed" };
      // write_response(client_fd, response);
  }

  send(client_fd , response.c_str() , response.length(), 0);

  return 0;
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  // int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  // if (client_fd < 0) {
  //   std::cerr << "accept failed\n";
  //   return 1;
  // }
  int client_fd;
  while (true) {
    client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n" << client_fd << "\n";
    std::thread th(handle_client, client_fd, client_addr);
    th.detach();
  }
  
  close(server_fd);

  return 0;
}
