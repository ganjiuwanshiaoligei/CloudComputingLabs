# -*- coding=utf-8 -*-
import socket
import threading
import queue
# -*- coding:utf-8 -*-
import os
import xml.dom.minidom
import sys


# 返回码
class ErrorCode(object):
    OK = "HTTP/1.1 200 OK\r\n"
    NOT_FOUND = "HTTP/1.1 404 Not Found\r\n"
    NOT_IMP = "HTTP/1.1 501 Not Implement\r\n"

# 将字典转成字符串
def dict2str(d):
    s = ''
    for i in d:
        s = s + i+': '+d[i]+'\r\n'
    return s


class HttpRequest(object):
    rootdir = os.path.split( os.path.realpath( sys.argv[0] ) )[0]
    


    def __init__(self):
        self.method = None
        self.url = None
        self.protocol = None
        self.head = dict()
        self.Cookie = None
        self.request_data = dict()
        self.response_line = ''
        self.response_head = dict()
        self.response_body = ''
        self.session = None
        self.lent = 0

    def passRequestLine(self, request_line):
        header_list = request_line.split(' ')
        self.method = header_list[0].upper()
        self.url = header_list[1]
        if self.url == '/':
            self.url = '/index.html'
        self.protocol = header_list[2]

    def passRequestHead(self, request_head):
        head_options = request_head.split('\r\n')
        for option in head_options:
            key, val = option.split(': ', 1)
            self.head[key] = val
            # print key, val
        if 'Cookie' in self.head:
            self.Cookie = self.head['Cookie']

    def passRequest(self, request):
        request = request.decode('utf-8')
       # self.lent = str(len(request))
        if len(request.split('\r\n', 1)) != 2:
            return
        request_line, body = request.split('\r\n', 1)
        request_head = body.split('\r\n\r\n', 1)[0]     # 头部信息
        self.passRequestLine(request_line)
        self.passRequestHead(request_head)


        # 所有post视为动态请求
        # get如果带参数也视为动态请求
        # 不带参数的get视为静态请求
        if self.method == 'POST':
            self.request_data = {}
            request_body = body.split('\r\n\r\n', 1)[1]
            parameters = request_body.split('&')   # 每一行是一个字段
            for i in parameters:
                if i=='':
                    continue
                key, val = i.split('=', 1)
                self.request_data[key] = val
            self.dynamicRequest(HttpRequest.rootdir+self.url)
        elif self.method == 'GET':
            if self.url.find('?') != -1:        # 含有参数的get
                self.request_data = {}
                req = self.url.split('?', 1)[1]
                s_url = self.url.split('?', 1)[0]
                parameters = req.split('&')
                for i in parameters:
                    key, val = i.split('=', 1)
                    self.request_data[key] = val
                self.dynamicRequest( HttpRequest.rootdir+s_url)
            else:
                self.staticRequest( HttpRequest.rootdir+self.url)
        else:
            self.response_line = ErrorCode.NOT_IMP
          #  self.response_head['Content-length'] = self.lent
            self.response_head['Server'] = 'My Web Server'
            self.response_head['Content-Type'] = 'text/html'
            self.response_body = '<html><title>501 Not Implemented</title><body bgcolor=ffffff>' + '\n' + '  Not Implemented'+'\n'  + "<p>Does not implement this method:" + self.method + '\n' + '<hr><em>HTTP Web server</em>' + '\n' + '</body></html>'

    # 只提供制定类型的静态文件
    def staticRequest(self, path):
        # print path
        if not os.path.isfile(path):
            self.response_line = ErrorCode.NOT_FOUND
         #   self.response_head['Content-length'] = self.lent
            self.response_head['Server'] = 'My Web Server'
            self.response_head['Content-Type'] = 'text/html'
            self.response_body = '<html><title>404 Not Found</title><body bgcolor=ffffff>' + '\n' + '  Not Found' +'\n' + "<p>Couldn't find this file:" + path + '\n' + '<hr><em>HTTP Web server</em>' + '\n' + '</body></html>'+ '\n'

        else:
            extension_name = os.path.splitext(path)[1]  # 扩展名
            extension_set = {'.css', '.html', '.js'}
            if extension_name == '.png':
                f = open(path, 'rb')
                self.response_line = ErrorCode.OK
             #   self.response_head['Content-length'] = self.lent
                self.response_head['Server'] = 'My Web Server'
                self.response_head['Content-Type'] = 'text/png'
                self.response_body = f.read()
            elif extension_name in extension_set:
                f = open(path, 'r')
                self.response_line = ErrorCode.OK
             #   self.response_head['Content-length'] = self.lent
                self.response_head['Server'] = 'My Web Server'
                self.response_head['Content-Type'] = 'text/html'
                self.response_body = f.read()
            elif extension_name == '.py':
                self.dynamicRequest(path)
            # 其他文件不返回
            else:
                self.response_line = ErrorCode.NOT_FOUND
              #  self.response_head['Content-length'] = self.lent
                self.response_head['Server'] = 'My Web Server'
                self.response_head['Content-Type'] = 'text/html'
                self.response_body = '<html><title>404 Not Found</title><body bgcolor=ffffff>' + '\n' + '  Not Found' + '\n' + "<p>Couldn't find this file:" + path + '\n' + '<hr><em>HTTP Web server</em>' + '\n' + '</body></html>' + '\n'

    def dynamicRequest(self, path):
        # 如果找不到或者后缀名不是py则输出404
        if not os.path.isfile(path) or os.path.splitext(path)[1] != '.py':

            self.response_line = ErrorCode.NOT_FOUND
          #  self.response_head['Content-length'] = self.lent
            self.response_head['Server'] = 'My Web Server'
            self.response_head['Content-Type'] = 'text/html'
            self.response_body = '<html><title>404 Not Found</title><body bgcolor=ffffff>' + '\n' + '  Not Found' + '\n' + "<p>Couldn't find this file:" + path + '\n' + '<hr><em>HTTP Web server</em>' + '\n' + '</body></html>'

        else:
            # 获取文件名，并且将/替换成.
            file_path = path.split('.', 1)[0].replace('/', '.')
            self.response_line = ErrorCode.OK
            m = __import__(file_path)
            m.main.SESSION = self.processSession()
            if self.method == 'POST':
                m.main.POST = self.request_data
                m.main.GET = None
            else:
                m.main.POST = None
                m.main.GET = self.request_data
            self.response_body = m.main.app()
            #self.response_head['Content-length'] = self.lent
            self.response_head['Server'] = 'My Web Server'
            self.response_head['Content-Type'] = 'text/html'


    def getResponse(self):
        t=str(len(self.response_line+dict2str(self.response_head)+'\r\n'+self.response_body))
        self.response_head['Content-length']=t
        return self.response_line+dict2str(self.response_head)+'\r\n'+self.response_body




# 每个任务线程
class WorkThread(threading.Thread):
    def __init__(self, work_queue):
        super().__init__()
        self.work_queue = work_queue
        self.daemon = True

    def run(self):
        while True:
            func, args = self.work_queue.get()
            func(*args)
            self.work_queue.task_done()


# 线程池
class ThreadPoolManger():
    def __init__(self, thread_number):
        self.thread_number = thread_number
        self.work_queue = queue.Queue()
        for i in range(self.thread_number):     # 生成一些线程来执行任务
            thread = WorkThread(self.work_queue)
            thread.start()

    def add_work(self, func, *args):
        self.work_queue.put((func, args))


def tcp_link(sock, addr):
    #print('Accept new connection from %s:%s...' % addr)
    request = sock.recv(1024)
 #   print(request,'\n')
    http_req = HttpRequest()
    http_req.passRequest(request)
    sock.send(http_req.getResponse().encode('utf-8'))
   # print(http_req.getResponse().encode('utf-8'))
    sock.close()


def start_server(a,b,c):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((a, int(b)))
    s.listen(10)
    thread_pool = ThreadPoolManger(int(c))
    while True:
        sock, addr = s.accept()
        thread_pool.add_work(tcp_link, *(sock, addr))


if __name__ == '__main__':
    start_server(sys.argv[1],sys.argv[2],sys.argv[3])
    pass

