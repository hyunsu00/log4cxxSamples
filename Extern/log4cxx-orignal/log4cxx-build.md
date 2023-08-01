# log4cxx-build

```bash
# 1. apr(Apache Portable Runtime) 다운로드 및 빌드. 
$ cd /tmp
$ wget https://archive.apache.org/dist/apr/apr-1.5.2.tar.gz
$ tar -xvzf apr-1.5.2.tar.gz
$ cd apr-1.5.2
$ ./configure
$ make
$ sudo make install

# 2. apr-util 다운로드 및 빌드
$ cd .. 
$ wget https://archive.apache.org/dist/apr/apr-util-1.5.4.tar.gz
$ tar -xvzf apr-util-1.5.4.tar.gz
$ cd apr-util-1.5.4
$ ./configure --with-apr=/usr/local/apr
$ make
$ sudo make install
 
# 3. log4cxx 다운로드 및 빌드
$ cd .. 
$ wget https://archive.apache.org/dist/logging/log4cxx/0.10.0/apache-log4cxx-0.10.0.tar.gz
$ tar -xvzf apache-log4cxx-0.10.0.tar.gz
$ tar -zxvf patch.tar.gz -C ./apache-log4cxx-0.10.0
$ cd apache-log4cxx-0.10.0
$ ./configure --enable-cppunit --with-apr=/usr/local/apr --with-apr-util=/usr/local/apr --with-charset=utf-8
$ make
$ sudo make install

# 4. 확인
$ cd /usr/local/lib
$ cd /usr/local/include/log4cxx
$ cd /usr/local/apr
```
