# 收集学生信息的服务器
很多时候我们需要大量收集学生的信息，本服务器能实现该需求，学生在自己的浏览器上连接本服务器，注册登录后填写自己的信息，服务器会把学生信息记录在mysql数据库中
* 通过线程池高并发的处理http请求
* 基于阻塞队列实现的异步日志系统
* RAII机制管理的数据库连接池
* 基于有限状态机方法的GET/POST请求解析
## 编译指令
> make server
## 执行
> ./server
## 数据库连接
	sudo apt install mysql-server    
	sudo mysql    
	ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'your_password';    
第一次用创建用户表  

	create table user(    
	username char(50) NULL,   
	passwd char(50) NULL   
	);   

	create table student(    
	id char(50) NULL,    
	name char(50) NULL,    
	gender char(50) NULL,    
	age char(50) NULL,    
	school char(50) NULL,   
	major char(50) NULL,   
	cellphone char(50) NULL,   
	email char(50) NULL   
	);   

	INSERT INTO user(username, passwd) VALUES('name', 'passwd');   

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
