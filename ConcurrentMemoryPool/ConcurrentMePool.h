#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//
static void* ConcurrentAlloc(size_t size)
{
	if (size > MAXBYTES)
	{
		//�����������������ҳ���룬�����˷����أ����ҽ���pagecache������ҳ����
		size_t roundsize = ClassSize::_Roundup(size,1<<PAGE_SHIFT);
		//ҳ��
		size_t npage = roundsize >> PAGE_SHIFT;

		Span* span = PageCache::GetInstance()->NewSpan(npage);
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		// ͨ��tls�������̶߳��е�ȫ�ֱ��������ݱ�����ȡ�߳��Լ���threadcache
		if (tls_threadcache == nullptr)
		{
			tls_threadcache = new ThreadCache;
			//cout << "�´������߳�"<<std::this_thread::get_id() << "->" << tls_threadcache << endl;
		}
		return tls_threadcache->Allocate(size);
	}
}

static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objsize;

	if (size > MAXBYTES)
	{
		PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
	}
	else
	{
		tls_threadcache->Deallocate(ptr, size);
	}
}