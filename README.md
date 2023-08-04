# MyWebserver
threadpool+log+getrequest
## 编译指令
  g++ config.cpp http_conn.cpp log.cpp web.cpp main.cpp -o server -pthread  
## 执行
  ./server
## webbench压测
### 1.1 安装依赖 exuberant-ctags  
	sudo apt-get install exuberant-ctags
### 1.2 下载源码并安装  
	wget http://blog.s135.com/soft/linux/webbench/webbench-1.5.tar.gz  
	tar zxvf webbench-1.5.tar.gz  
	cd webbench-1.5  
	make && sudo make install
### 1.3 原理
>  **webbench首先fork出多个子进程，每个子进程都循环做web访问测试。子进程把访问的结果通过pipe告诉父进程，父进程做最终的统计结果。**
### 1.4 测试结果
>  **指令：webbench -c 10000 -t 1 http://ip:port/**
1.10000,1s    

	tp://172.27.170.70:9006/index.html
	Webbench - Simple Web Benchmark 1.5
	Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

	Benchmarking: GET http://172.27.170.70:9006/index.html
	10000 clients, running 1 sec.

	Speed=1690739 pages/min, 4480143 bytes/sec.
	Requests: 28179 susceed, 0 failed.  
2.10000,30s


	/172.27.170.70:9006/index.html
	Webbench - Simple Web Benchmark 1.5
	Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

	Benchmarking: GET http://172.27.170.70:9006/index.html
	10000 clients, running 30 sec.

	Speed=1386354 pages/min, 3673838 bytes/sec.
	Requests: 693177 susceed, 0 failed.
