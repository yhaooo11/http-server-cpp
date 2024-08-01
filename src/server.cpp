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
    // size_t pos = toks[i].find(":");
    // if (pos == std::string::npos) {
    //   break;
    // }
    // std::vector<std::string> header_toks = split_string(toks[i], ": ");
    // headers[header_toks[0]] = header_toks[1];
  }
  return headers;
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
  
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  if (client_fd < 0) {
    std::cerr << "accept failed\n";
    return 1;
  }

  std::cout << "Client connected\n";

  char buff[2048];
  // use ssize_t if return value can be negative (i.e. error value)
  // reading from file descriptor here, another way is just int ret = read(client_fd, buffer, sizeof(buffer));
  ssize_t t = recvfrom(client_fd, buff, sizeof(buff) - 1, 0, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  if (t < 0){
    std::cerr << "listen failed\n";
    return 1;
  }

  std::string request(buff);
  // std::cout << "Received message from client: " << msg_str << "\n";
  std::string path = get_path(request);
  std::map<std::string, std::string> headers = get_headers(request);
  std::cout << headers["User-Agent"] << "\n";
  // std::cout << "Path: " << path << "\n";
  // std::string response = request.starts_with("GET / HTTP/1.1\r\n") ? "HTTP/1.1 200 OK\r\n\r\n" : "HTTP/1.1 404 Not Found\r\n\r\n" ;

  std::vector<std::string> split_paths = split_string(path, "/");
  std::string response;
  if (path == "/") {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  } else if (split_paths[1] == "echo") {
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(split_paths[2].length()) + "\r\n\r\n" + split_paths[2];
  } else if (split_paths[1] == "user-agent") {
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(headers["User-Agent"].length()) + "\r\n\r\n" + headers["User-Agent"];
  } else {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  // std::string message = "HTTP/1.1 200 OK\r\n\r\n";
  send(client_fd , response.c_str() , response.length(), 0);
  
  close(server_fd);

  return 0;
}
