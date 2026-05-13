<h1 align="center">🔥 HTTP Flooder V2</h1>
<p align="center">
  <b>C++ High-Performance HTTP/HTTPS Stress Testing Tool</b><br>
  <i>Optimized for authorized penetration testing and security assessments</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg?style=for-the-badge&logo=c%2B%2B">
  <img src="https://img.shields.io/badge/OpenSSL-3.x-red.svg?style=for-the-badge&logo=openssl">
  <img src="https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge">
  <img src="https://img.shields.io/badge/Threads-Multi--Threaded-orange.svg?style=for-the-badge">
</p>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Architecture](#-architecture)
- [Requirements](#-requirements)
- [Installation](#-installation)
- [Usage](#-usage)
- [Parameters](#-parameters)
- [Examples](#-examples)
- [Performance](#-performance)
- [Comparison: V1 vs V2](#-comparison-v1-vs-v2)
- [Build Options](#-build-options)
- [Troubleshooting](#-troubleshooting)
- [Important Notice](#-important-notice)
- [License](#-license)

---

## 📌 Overview

**HTTP Flooder V2** is a high-performance, multi-threaded stress testing tool written in **C++17**. It is designed to simulate heavy HTTP/HTTPS traffic against web servers for **authorized** penetration testing and security assessments.

The V2 release introduces significant performance optimizations over V1, including persistent Keep-Alive connections, DNS caching, TLS session reuse, pre-generated header randomization, and real HTTP response parsing for accurate statistics.

---

## 🚀 Features

| Feature | Details |
|---------|---------|
| **Protocols** | HTTP / HTTPS (with OpenSSL) |
| **Methods** | GET (with cache-busting params) / POST |
| **Concurrency** | Multi-threaded (user-defined thread count) |
| **Keep-Alive** | Up to **50,000 requests per connection** |
| **DNS Caching** | Resolved once at startup, reused across all threads |
| **TLS Session Reuse** | Single SSL context + session caching for minimal handshake overhead |
| **Header Randomization** | Pre-generated cache of **500 unique User-Agents, Referers, Accept headers** |
| **Accurate Statistics** | Parses HTTP response codes — only **2xx** counts as success |
| **Real-time Monitoring** | Requests/sec, success rate, failures, bandwidth, active connections |
| **Custom Headers** | Load custom HTTP headers from file |
| **Cross-Platform** | Linux, macOS, Windows (via WinSock2) |

---

## 🏗 Architecture