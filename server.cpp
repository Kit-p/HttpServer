#include <iostream>
#include <thread>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Global Constants
const int SERVER_PORT = 80; // use port 80 for HTTP
const int LISTENNQ = 5;
const int MAXLINE = 8192;

enum class HttpMethods
{
    UNDEFINED,
    GET,
    POST
};

class HttpRequest;

bool startsWith(std::string, std::string);
bool endsWith(std::string, std::string);
HttpRequest *parse_request(int conn_fd);
void request_handler(int conn_fd);

int main()
{
    int server_fd, conn_fd;
    if (server_fd = socket(AF_INET, SOCK_STREAM, 0) < 0)
    {
        std::cerr << "Socket creation failed!" << std::endl;
        return 0;
    }

    sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(sockaddr_in);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(sockaddr)) < 0)
    {
        std::cerr << "Bind failed!" << std::endl;
        return 0;
    }

    if (listen(server_fd, LISTENNQ) < 0)
    {
        std::cerr << "Listen failed!" << std::endl;
        return 0;
    }

    char ip_str[INET_ADDRSTRLEN] = {0};

    while (true)
    {
        conn_fd = accept(server_fd, (sockaddr *)&client_addr, &len);
        if (conn_fd < 0)
        {
            std::cerr << "Accept failed!" << std::endl;
            return 0;
        }

        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

        std::cout << "Connection from " << ip_str << ":" << ntohs(client_addr.sin_port) << std::endl;

        std::thread t(request_handler, conn_fd);
    }

    return 0;
}

void request_handler(int conn_fd)
{
    HttpRequest *request = parse_request(conn_fd);
}

HttpRequest *parse_request(int conn_fd)
{
    int buffer_size = 0;
    char buf[MAXLINE] = {0};
    std::string msg = "";
    HttpRequest *request = nullptr;

    while (true)
    {
        buffer_size = recv(conn_fd, buf, MAXLINE - 1, 0);

        if (buffer_size <= 0)
        {
            std::cerr << "Recv failed!" << std::endl;
            continue;
        }

        if (buf[buffer_size] != '\0')
        {
            buf[buffer_size] = '\0';
        }
        msg += buf;

        request = HttpRequest::parse(msg);
        if (!request->isBad())
        {
            break;
        }
        delete request;
        request = nullptr;
    }

    return request;
}

bool startsWith(std::string base, std::string compare)
{
    if (compare.length() <= 0 || base.length() < compare.length())
    {
        return false;
    }
    return base.compare(0, compare.length(), compare) == 0;
}

bool endsWith(std::string base, std::string compare)
{
    if (compare.length() <= 0 || base.length() < compare.length())
    {
        return false;
    }
    return base.compare(base.length() - compare.length(), std::string::npos, compare) == 0;
}

class HttpRequest
{
public:
    HttpRequest() : method(HttpMethods::UNDEFINED), url(""), version("") {}

    HttpMethods method;
    std::string url;
    std::string version;

    bool isBad()
    {
        if (this->method == HttpMethods::UNDEFINED)
            return true;

        if (!startsWith(this->url, "/"))
            return true;

        if (!startsWith(this->version, "HTTP/"))
            return true;

        return false;
    }

    static HttpRequest *parse(std::string msg)
    {
        HttpRequest *request = new HttpRequest();

        // parse
        const std::string sp = " ";
        const std::string crlf = "\r\n";

        int start_pos = 0;
        int end_pos = msg.find(sp);
        if (start_pos >= msg.length() || end_pos == std::string::npos)
        {
            return request;
        }

        request->method = toMethod(msg.substr(start_pos, end_pos - start_pos));

        start_pos = end_pos + 1;
        end_pos = msg.find(sp);
        if (start_pos >= msg.length() || end_pos == std::string::npos)
        {
            return request;
        }

        request->url = msg.substr(start_pos, end_pos - start_pos);

        start_pos = end_pos + 1;
        end_pos = msg.find(crlf);
        if (start_pos >= msg.length() || end_pos == std::string::npos)
        {
            return request;
        }

        request->version = msg.substr(start_pos, end_pos - start_pos);

        return request;
    }

    static HttpMethods toMethod(std::string method)
    {
        if (method == "GET" || method == "get")
            return HttpMethods::GET;

        if (method == "POST" || method == "post")
            return HttpMethods::POST;

        return HttpMethods::UNDEFINED;
    }
};
