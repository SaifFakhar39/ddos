# Advanced HTTP Flooder V3 - C++

<div align="center">

![C++](https://img.shields.io/badge/C%2B%2B-17%2B-blue?style=flat-square&logo=c%2B%2B)
![OpenSSL](https://img.shields.io/badge/OpenSSL-3.x-green?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-red?style=flat-square)

**أداة HTTP Flooding متقدمة مكتوبة بلغة C++ مع دعم البروكسي و Keep-Alive**

</div>

## 📖 نظرة عامة

**Advanced HTTP Flooder V3** هي أداة أداء عالي مكتوبة بلغة C++ تركز على الاستقرار، السرعة، والكفاءة. تدعم HTTPS، بروكسي HTTP، إعادة استخدام الاتصالات (Keep-Alive)، وقراءة الردود الحقيقية.

> ⚠️ **هذا المشروع لأغراض تعليمية وبحثية فقط.**

---

## ✨ المميزات

- **أداء عالي جداً** بفضل C++17
- دعم **HTTP/HTTPS** (TLS)
- **Keep-Alive** حقيقي (إرسال آلاف الطلبات على اتصال واحد)
- دعم **بروكسي HTTP** (مع CONNECT للـ HTTPS)
- قراءة وتحليل **HTTP Response Codes** (2xx, 4xx, 5xx)
- إحصائيات دقيقة في الوقت الفعلي (RPS, Success Rate, MB/s)
- Pre-caching للـ Headers لتقليل الحمل على CPU
- واجهة ملونة احترافية
- إدارة ذاكرة واستقرار عالي
- متوافق مع **Windows و Linux**

---

## 🛠️ المتطلبات

### Linux:
```bash
sudo apt update
sudo apt install g++ libssl-dev openssl
Windows:

Visual Studio 2022 أو MinGW-w64
OpenSSL (موصى به v3.x)


📥 التحميل والتجميع
Linux:
Bashgit clone https://github.com/yourusername/http-flooder-v3.git
cd http-flooder-v3
g++ -O3 -std=c++17 -o flooder main.cpp -lssl -lcrypto -lpthread
Windows (MinGW):
Bashg++ -O3 -std=c++17 -o flooder.exe main.cpp -lssl -lcrypto -lws2_32

🚀 طريقة الاستخدام
Bash./flooder <URL> <threads> <mode> <seconds> <proxy_file>
أمثلة:
Bash# هجوم GET بـ 300 خيط لمدة 60 ثانية
./flooder https://example.com 300 get 60 proxy.txt

# هجوم POST
./flooder https://target.com/api 500 post 120 proxy.txt

# بدون بروكسي (للاختبار)
./flooder http://localhost 200 get 30 nil

📁 ملفات البروكسي
أنشئ ملف proxy.txt بالصيغة التالية:
text123.45.67.89:8080
203.0.113.45:3128
198.51.100.23:80

⚠️ تحذير هام

هذا البرنامج يُستخدم لأغراض تعليمية وبحث أمني فقط.
استخدامه على أي موقع أو خادم بدون إذن صريح غير قانوني وقد يعرضك للمساءلة القانونية.
المطور غير مسؤول عن أي سوء استخدام.


📊 الأداء

























الوضعالخيوطRPS المتوقعHTTP + Proxy3008,000 - 25,000HTTPS + Proxy2004,000 - 12,000HTTP (بدون بروكسي)50030,000+
الأداء يعتمد بشكل كبير على جودة البروكسيات واتصالك.

🛠️ المخطط المستقبلي

 دعم SOCKS5
 Randomization أكثر تقدماً (User-Agent, Headers)
 Rate Limiting Bypass techniques
 WebSocket Flooding
 Docker Support


📄 الترخيص
هذا المشروع مرخص تحت MIT License.

⚠️ تنبيه:
هذا المستودع مخصص للدراسة وفهم آليات هجمات Layer 7. استخدمه بمسؤولية.