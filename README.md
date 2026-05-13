# README.md — جاهز للنسخ

````md
<div align="center">

# ⚡ Advanced HTTP Flooder V2
### High Performance Multi-Threaded HTTP/HTTPS Stress Testing Engine

<img src="https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus" />
<img src="https://img.shields.io/badge/OpenSSL-Supported-green?style=for-the-badge&logo=openssl" />
<img src="https://img.shields.io/badge/Linux-Compatible-orange?style=for-the-badge&logo=linux" />
<img src="https://img.shields.io/badge/Windows-Compatible-blue?style=for-the-badge&logo=windows" />
<img src="https://img.shields.io/badge/Multi--Threaded-Optimized-red?style=for-the-badge" />

---

### 🚀 Optimized HTTP/HTTPS Connection Engine Written in Modern C++

مصمم لاختبارات الضغط الشبكي وتحليل الأداء والبنية التحتية عالية التحمل.

</div>

---

# 📌 Overview

**Advanced HTTP Flooder V2** هو محرك اتصالات HTTP/HTTPS عالي الأداء مكتوب بلغة **C++** مع دعم كامل لـ:

- Multi-threading
- Persistent Keep-Alive Connections
- SSL/TLS عبر OpenSSL
- Connection Reuse
- Real HTTP Response Validation
- Live Statistics Monitoring
- Optimized Socket Performance

تم تصميم المشروع لتحقيق أعلى كفاءة ممكنة في إدارة الاتصالات وتقليل الـ overhead أثناء عمليات الاختبار الشبكي.

---

# ✨ Features

## ⚡ High Performance Networking
- Persistent HTTP Keep-Alive
- آلاف الطلبات على نفس الاتصال
- TCP_NODELAY optimization
- DNS caching
- TLS session reuse
- Low latency socket operations

---

## 🔒 HTTPS / SSL Support
- دعم HTTPS الكامل عبر OpenSSL
- TLS Session Cache
- SSL Handshake Optimization
- Auto Retry Mode

---

## 📊 Advanced Statistics
يعرض النظام إحصائيات مباشرة مثل:

- عدد الطلبات المرسلة
- عدد الاستجابات الناجحة (2xx)
- الأخطاء (4xx / 5xx)
- الاتصالات النشطة
- معدل الطلبات في الثانية
- كمية البيانات المنقولة

---

## 🧠 Smart Request Generation
- Random User-Agent generation
- Dynamic Referers
- Randomized GET parameters
- Multiple Accept headers
- Header caching لتحسين الأداء

---

## 🖥 Cross Platform
يدعم:

| Platform | Status |
|---|---|
| Linux | ✅ |
| Windows | ✅ |
| macOS | ⚠️ Experimental |

---

# 🏗 Architecture

```text
                ┌────────────────────┐
                │   Main Controller   │
                └─────────┬──────────┘
                          │
         ┌────────────────┼────────────────┐
         │                │                │
         ▼                ▼                ▼
 ┌────────────┐   ┌────────────┐   ┌────────────┐
 │ Worker #1  │   │ Worker #2  │   │ Worker #N  │
 └─────┬──────┘   └─────┬──────┘   └─────┬──────┘
       │                │                │
       ▼                ▼                ▼
 ┌────────────────────────────────────────────┐
 │ Persistent Keep-Alive Connections Pooling │
 └────────────────────────────────────────────┘
```

---

# 📦 Requirements

## Linux

```bash
sudo apt update
sudo apt install g++ libssl-dev build-essential
```

---

## Windows

### باستخدام MinGW:
- GCC 10+
- OpenSSL
- Winsock2

---

# 🔨 Compilation

## Linux

```bash
g++ -O3 -std=c++17 flooder.cpp -o flooder -lssl -lcrypto -lpthread
```

---

## Windows (MinGW)

```bash
g++ -O3 -std=c++17 flooder.cpp -o flooder.exe -lws2_32 -lssl -lcrypto
```

---

# 🚀 Usage

```bash
./flooder <url> <threads> <get/post> <seconds> <header_file/nil>
```

---

# 📌 Example

```bash
./flooder https://example.com 200 get 60 nil
```

---

# ⚙ Parameters

| Parameter | Description |
|---|---|
| `<url>` | Target URL |
| `<threads>` | Number of worker threads |
| `<get/post>` | Request method |
| `<seconds>` | Test duration |
| `<header_file/nil>` | Custom headers file |

---

# 📈 Live Statistics Example

```text
[12s] Sent: 542100 (45175/s)
OK: 531882 (44320/s)
Failed: 10218
Active: 200
MB: 742
```

---

# 🧩 Core Optimizations

## 🔥 Persistent Connections
بدلاً من إنشاء اتصال جديد لكل طلب:
- يتم إعادة استخدام نفس الاتصال لعشرات الآلاف من الطلبات.

---

## ⚡ TLS Session Reuse
تقليل تكلفة:
- TLS Handshake
- SSL Negotiation

---

## 🧠 Header Pre-Caching
يتم توليد:
- User-Agent
- Accept
- Referer

مسبقاً لتقليل الضغط على المعالج.

---

## 📡 Real Response Validation
يقوم النظام بقراءة:
- HTTP Status Code
- Headers

للتأكد من نجاح الاتصال فعلياً.

---

# 📂 Project Structure

```text
project/
│
├── flooder.cpp
├── README.md
└── headers.txt
```

---

# 🔐 Security Notice

> هذا المشروع مخصص حصرياً لاختبارات الضغط والأبحاث الأمنية والاختبارات المخبرية المصرح بها فقط.

أي استخدام غير قانوني أو ضد أنظمة بدون تصريح يعتبر مسؤولية المستخدم بالكامل.

---

# 🛠 Recommended Build Flags

```bash
-O3
-march=native
-flto
-pthread
```

---

# 📚 Technologies Used

| Technology | Purpose |
|---|---|
| C++17 | Core Engine |
| OpenSSL | HTTPS/TLS |
| POSIX Threads | Multi-threading |
| Winsock2 | Windows Networking |

---

# 🧪 Performance Notes

يعتمد الأداء على:
- سرعة الشبكة
- عدد الأنوية
- جودة السيرفر المستهدف
- Limits الخاصة بالنظام
- File Descriptor Limits

---

# ⚙ Suggested System Tuning (Linux)

```bash
ulimit -n 1048576
sysctl -w net.ipv4.ip_local_port_range="1024 65535"
sysctl -w net.ipv4.tcp_fin_timeout=15
```

---

# 🖼 Preview

```text
███████╗██╗      ██████╗  ██████╗ ██████╗ ███████╗██████╗
██╔════╝██║     ██╔═══██╗██╔═══██╗██╔══██╗██╔════╝██╔══██╗
█████╗  ██║     ██║   ██║██║   ██║██████╔╝█████╗  ██████╔╝
██╔══╝  ██║     ██║   ██║██║   ██║██╔═══╝ ██╔══╝  ██╔══██╗
██║     ███████╗╚██████╔╝╚██████╔╝██║     ███████╗██║  ██║
╚═╝     ╚══════╝ ╚═════╝  ╚═════╝ ╚═╝     ╚══════╝╚═╝  ╚═╝
```

---

# 📄 License

```text
MIT License
```

---

# 🤝 Contribution

أي تحسينات أو Pull Requests مرحب بها.

---

# ⭐ Credits

Developed with ❤️ using Modern C++ & OpenSSL.

---

<div align="center">

### ⚡ Advanced HTTP Flooder V2
#### Optimized Networking Engine

</div>
````
