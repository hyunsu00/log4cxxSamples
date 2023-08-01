# log4cxxSamples

## ldconfig
```bash
# 공유 라이브러리 캐시를 다시 설정
# 동적으로 링크된 실행 파일은 공유 라이브러리에 완벽하게 의존적이므로 새로운 버전의 라이브러리를 설치하고 
# 이것을 사용하려면 디렉토리를 설정하고 ldconfig로 공유라이브러리 캐시를 다시 설정해야 한다.
# /etc/ld.so.conf.d/ 에 .conf 파일을 추가, 수정하거나
# LD_LIBRARY_PATH 를 변경하게 된다면 ldconfig를 통해 라이브러리를 다시 설정해야 한다.

# /etc/id.so.cache 캐쉬파일 업데이트
$ sudo ldconfig
# 현재 캐시에 저장된 디렉토리와 라이브러리 목록 출력
$ sudo ldconfig -p
```

## ldd
```bash
# 라이브러리 의존성 확인
# /etc/id.so.cache 캐쉬파일 업데이트
$ ldd liblog4cxx.so
```

## readelf
```bash
# path 가 설정되었는지는 readelf -d 로 확인
$ readelf -d liblog4cxx.so
```

## LD_LIBRARY_PATH
- shared library를 사용한 프로그램은 run-time linker가 이 프로그램에 필요한 library들을 LD_LIBRARY_PATH에서 찾아서 링크시킨 후 실행된다.

## LD_RUN_PATH (링커-R option)
- 실행파일 안에 LD_LIBRARY_PATH 외에 shared library를 찾을 directory를 기록해 놓는 것이다.

## GCC 옵션
### C / C++ 기타옵션
```bash
# -fPIC
# 코드의 배치를 지정하는 옵션으로 공용라이브러리를 만들 때 주로 사용되며, 생성되는 바이너리의 크기는 증가하지만 
# 코드의 동작이 보다 빠르게 동작하도록 해준다.
```

### 링커옵션
```bash
# -l
# 링크되는 라이브러리의 이름을 지정 한다.
# ex)-llog4cxx

# -L
# 링크되는 라이브러리가 존재하는 경로를 지정 한다.(공유,정적라이브러리 search)
# ex) -L/usr/local/lib

# -rpath
# 링크되는 라이브러리가 존재하는 경로를 지정 한다.(공유라이브러리만 search)
# ex) -rpath/usr/local/lib

# -R
# 링크되는 실행파일 안에 shared library를 찾을 경로를 기록
# ex) -R/usr/local/lib

# -Wl
# 링크옵션임을 알린다.
# ex) -Wl,-rpath

# -shared
# 공유 라이브러리와 정적 라이브러리가 같이 있다면, 공유 라이브러리를 우선하여 링크한다. 
# (아무 옵션을 주지 않아도 공유 라이브러리를 우선으로 링크한다.)

# -static 
# 정적 라이브러리와 공유 라이브러리가 같이 있다면, 정적 라이브러리를 우선하여 링크한다.
```
