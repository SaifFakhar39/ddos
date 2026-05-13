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