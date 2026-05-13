🚀 Advanced HTTP Flooder - Optimized V2 🛡️

أداة احترافية مكتوبة بلغة ++C لاختبار تحمل المواقع والشبكات (Stress Testing) بأداء عالٍ وتقنيات متقدمة.

هذا الإصدار (V2) هو تطوير جذري يهدف إلى الوصول لأقصى عدد من الطلبات في الثانية (RPS) مع استهلاك أقل قدر ممكن من موارد المعالج والذاكرة، من خلال تقليل الـ Overhead الناتج عن إنشاء الاتصالات المتكررة.

💎 المميزات الرئيسية (Key Features)

⚡ أداء خارق (Ultra High RPS): استخدام تقنية Keep-Alive الحقيقية لإرسال آلاف الطلبات عبر اتصال TCP واحد، مما يقلل من عبء المصافحة (Handshake).

🔐 تحسين التشفير (TLS Session Reuse): دعم كامل لبروتوكول HTTPS مع إعادة استخدام جلسات SSL/TLS لتقليل التأخير الناتج عن التشفير.

📊 تحليل الاستجابة (Smart Response Tracking): الأداة لا ترسل فقط، بل تقرأ الـ HTTP Response للتأكد من نجاح الطلبات (2xx) أو فشلها (4xx/5xx).

🎲 رأسيات عشوائية (Dynamic Headers): توليد تلقائي للـ User-Agents و Referers بشكل مسبق (Caching) لتجنب الحسابات الثقيلة أثناء الهجوم.

🌍 توافق كامل (Cross-Platform): تعمل الأداة بسلاسة على أنظمة Linux و Windows (دعم Winsock و POSIX).

📈 إحصائيات حية (Live Dashboard): واجهة سطر أوامر ملونة تعرض (عدد الطلبات، النجاح، الفشل، حجم البيانات المرسلة MB، والـ RPS الحالي).

🛠️ المتطلبات التقنية (Prerequisites)

تحتاج إلى المكتبات التالية لتشغيل الأداة:

OpenSSL Library: للتعامل مع اتصالات HTTPS المشفرة.

g++ / MSVC: مترجم يدعم معايير C++11 أو أحدث.

Threads: دعم خيوط المعالجة المتعددة.

🚀 التثبيت والتشغيل (Installation & Build)

🐧 على نظام Linux (Ubuntu/Debian)

# تثبيت المكتبات اللازمة
sudo apt-get update
sudo apt-get install build-essential libssl-dev

# تجميع الكود
g++ -O3 main.cpp -o flooder -lssl -lcrypto -lpthread

# التشغيل
./flooder <url> <threads> <mode> <duration> <header_file>


🪟 على نظام Windows

قم بتثبيت OpenSSL لـ Windows.

استخدم MinGW أو Visual Studio.

اربط المكتبات التالية: -lws2_32 -lssl -lcrypto.

📖 دليل الاستخدام (Usage Guide)

./flooder <URL> <THREADS> <GET/POST> <TIME> <HEADERS>


المعامل

الوصف

مثال

URL

الرابط المستهدف (HTTP/HTTPS)

https://target.com/

THREADS

عدد الخيوط (التوازي)

500

MODE

نوع الطلب (get أو post)

get

TIME

مدة الاختبار بالثواني

60

HEADERS

مسار ملف headers مخصص أو nil

nil

مثال تطبيقي:

./flooder https://example.com/ 200 get 60 nil


🏗️ البنية الهندسية (Architecture)

يعتمد الكود على توزيع المهام عبر floodWorker حيث يتم تخصيص ConnectionPool لكل خيط لضمان عدم حدوث تصادم في الذاكرة (Thread-Safe). يتم استخدام std::atomic لضمان دقة الإحصائيات في بيئة متعددة الخيوط دون التضحية بالسرعة.

⚠️ إخلاء مسؤولية (Legal Disclaimer)

هذه الأداة مخصصة للأغراض التعليمية واختبار الاختراق الأخلاقي فقط.
استخدام هذه الأداة ضد أهداف دون إذن صريح مسبق يعتبر عملاً غير قانوني. المطور غير مسؤول عن أي سوء استخدام أو أضرار ناتجة عن هذه الأداة. استخدمها بمسؤولية لدعم أمنك السيبراني وتطوير مهاراتك البرمجية.

👨‍💻 المساهمة (Contribution)

إذا كان لديك اقتراحات لتحسين الكود أو إضافة مميزات جديدة (مثل دعم HTTP/2)، فلا تتردد في فتح Issue أو إرسال Pull Request.

