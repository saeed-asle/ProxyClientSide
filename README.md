# Simple C Proxy
Authored by Saeed Asle
# Description
This project is a simple proxy server implemented in C.
HTTP Proxy Client Side a basic HTTP client that used to retrieve web resources given by a URL, cache them locally, then either print the data or show it in a browser.
It allows users to fetch and store web pages from remote servers locally.
The proxy server supports HTTP requests and can be used to cache web pages for faster access.

# Features
* Fetches web pages from remote servers using HTTP requests.
* Caches fetched web pages locally for future access.
* Supports basic error handling for network and file operations.
# How to Use
* Compile the code using a C compiler. For example, you can use GCC:

    gcc -o cproxy cproxy.c
    
Run the proxy server, specifying the URL of the web page you want to fetch
# Output
The proxy server outputs the HTTP response received from the remote server, including the content of the web page.
It also provides basic logging to track the flow of communication.
