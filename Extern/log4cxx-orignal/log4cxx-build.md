# log4cxx-build

## 참고)
- [[Linux] log4cxx 컴파일](https://kiaak.tistory.com/m/entry/Linux-log4cxx-%EC%BB%B4%ED%8C%8C%EC%9D%BC)
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
$ ./configure --with-expat=builtin --with-apr=/usr/local/apr
$ make
$ sudo make install
 
# 3. log4cxx 다운로드 및 빌드
$ cd .. 
$ wget https://archive.apache.org/dist/logging/log4cxx/0.10.0/apache-log4cxx-0.10.0.tar.gz
$ tar -xvzf apache-log4cxx-0.10.0.tar.gz
$ tar -zxvf patch.tar.gz -C ./apache-log4cxx-0.10.0
$ cd apache-log4cxx-0.10.0
$ ./configure --with-apr=/usr/local/apr --with-apr-util=/usr/local/apr --with-charset=utf-8
$ make
$ sudo make install

# 4. 확인
$ cd /usr/local/lib
$ cd /usr/local/include/log4cxx
$ cd /usr/local/apr
```


```bash
# 사용
# apr-1.5.2
----------------------------------------------------------------------
Libraries have been installed in:
   /usr/local/apr/lib

If you ever happen to want to link against installed libraries
in a given directory, LIBDIR, you must either use libtool, and
specify the full pathname of the library, or use the `-LLIBDIR'
flag during linking and do at least one of the following:
   - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
     during execution
   - add LIBDIR to the `LD_RUN_PATH' environment variable
     during linking
   - use the `-Wl,-rpath -Wl,LIBDIR' linker flag
   - have your system administrator add LIBDIR to `/etc/ld.so.conf'

See any operating system documentation about shared libraries for
more information, such as the ld(1) and ld.so(8) manual pages.
----------------------------------------------------------------------

# apr-util-1.5.4
----------------------------------------------------------------------
Libraries have been installed in:
   /usr/local/apr/lib

If you ever happen to want to link against installed libraries
in a given directory, LIBDIR, you must either use libtool, and
specify the full pathname of the library, or use the `-LLIBDIR'
flag during linking and do at least one of the following:
   - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
     during execution
   - add LIBDIR to the `LD_RUN_PATH' environment variable
     during linking
   - use the `-Wl,-rpath -Wl,LIBDIR' linker flag
   - have your system administrator add LIBDIR to `/etc/ld.so.conf'

See any operating system documentation about shared libraries for
more information, such as the ld(1) and ld.so(8) manual pages.
----------------------------------------------------------------------

# apache-log4cxx-0.10.0
----------------------------------------------------------------------
Libraries have been installed in:
  /usr/local/lib
​
If you ever happen to want to link against installed libraries
in a given directory, LIBDIR, you must either use libtool, and
specify the full pathname of the library, or use the `-LLIBDIR'
flag during linking and do at least one of the following:
  - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
    during execution
  - add LIBDIR to the `LD_RUN_PATH' environment variable
    during linking
  - use the `-Wl,--rpath -Wl,LIBDIR' linker flag
  - have your system administrator add LIBDIR to `/etc/ld.so.conf'
​
See any operating system documentation about shared libraries for
more information, such as the ld(1) and ld.so(8) manual pages.
----------------------------------------------------------------------​
```

