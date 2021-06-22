// BuildConfig.h
#pragma once

#define ENABLE_MEMORY_LEAK_CHECK	1

// memory leak 체크
// 디버그에서는 메모리닉 체크를 하기 위해 속도가 느려진다.
// 필요할때만 사용을 한다.
#if (ENABLE_MEMORY_LEAK_CHECK == 1)
#	if defined(_WIN32) && defined(_DEBUG) 
#		include <vld.h>
#	endif
#endif

