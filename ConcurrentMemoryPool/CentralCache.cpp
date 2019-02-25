#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_inst;

//// ��׮	(α���ݲ���)
//// �����Ļ����ȡһ�������Ķ����thread cache
//size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t bytes)
//{	 //��֤threadcache�����Ļ�ȡ�ڴ�
//	// ͨ��α���ݲ��Թ��ܣ���Ԫ�����н�����׮
//	start = malloc(n*bytes);
//	end = (char*)start + (n - 1)*bytes;
//	//���ӳ�����
//	void* cur = start;
//	while (cur != end)
//	{
//		void* next = (char*)cur + bytes;
//		NEXT_OBJ(cur) = next;
//		cur = next;
//	}
//	NEXT_OBJ(end) = nullptr;
//	return n;
//}

//��ȡ��Ϊ�յ�span��������һ��
Span* CentralCache::GetOneSpan(SpanList* spanlist, size_t bytes)
{
	Span* span = spanlist->begin();
	while (span != spanlist->end())
	{
		if (span->_objlist != nullptr)
			return span;

		span = span->_next;
	}

	//ͨ��bytes�����npage
	size_t npage = ClassSize::BytesMovePage(bytes);
	//ȫ��span�� ��pagecache����һ���µĺ��ʴ�С��span
	Span* newspan = PageCache::GetInstance()->NewSpan(npage);

	// ��span���ڴ��и��һ����bytes��С�Ķ��������
	char* start = (char*)(newspan->_pageid << PAGE_SHIFT);
	char* end = start + (newspan->_npage << PAGE_SHIFT);
	char* cur = start;
	char* next = cur + bytes;
	while (next < end)
	{
		NEXT_OBJ(cur) = next;
		cur = next;
		next = cur + bytes;
	}
	NEXT_OBJ(cur) = nullptr;
	newspan->_objlist = start;
	newspan->_objsize = bytes;
	newspan->_usecount = 0;

	// ��newspan���뵽spanlist
	spanlist->PushFront(newspan);
	return newspan;
}

// �����Ļ����ȡһ�������Ķ���� central->thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t bytes)
{
	size_t index = ClassSize::Index(bytes);
	SpanList* spanlist = &_spanlist[index];

	//��Ͱ����

	//spanlist->_mutex.lock() 
	//�����ڴ治�㣬���������룬�������virtualallov���벻�ɹ������쳣
	//����Ҫ�����쳣�����Ҳ����ͷ������Ͳ����������⣬�������������󽵵�
	//��������RALL
	std::unique_lock<std::mutex> lock(spanlist->_mutex);

	Span* span = GetOneSpan(spanlist, bytes);  

	void* cur = span->_objlist;
	void* prev = cur;
	size_t fetchnum = 0;
	while (cur != nullptr && fetchnum < num)
	{
		prev = cur;
		cur = NEXT_OBJ(cur);
		++fetchnum;
	}

	start = span->_objlist;
	end = prev;
	NEXT_OBJ(end) = nullptr;

	span->_objlist = cur;
	span->_usecount += fetchnum;

	//����һ��spanΪ�գ��Ƶ�β�ϣ������´α�����span
	if (span->_objlist == nullptr)
	{
		spanlist->Erase(span);
		spanlist->PushBack(span);
	}

	return fetchnum;
}

//thread->central
void CentralCache::ReleaseListToSpans(void* start, size_t byte)
{
	//�ͷŶ�Ͱ����
	size_t index = ClassSize::Index(byte);
	SpanList* spanlist = &_spanlist[index];
	std::unique_lock<std::mutex> lock(spanlist->_mutex);

	//�ҵ����ʵ�spanͷɾ
	while (start)
	{
		void* next = NEXT_OBJ(start);
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);

		//���ͷŵĶ���ص��յ�span����span�Ƶ�ͷ�ϣ����ٱ�����span
		if (span->_objlist == nullptr)
		{
			spanlist->Erase(span);
			spanlist->PushFront(span);
		}

		NEXT_OBJ(start) = span->_objlist;
		span->_objlist = start;

		//usecount == 0����ʾ���У����ͷŵĹ黹�ˣ����յ�pagecache���кϲ�
		if (--span->_usecount == 0)
		{
			_spanlist[index].Erase(span);
			span->_objlist = nullptr;
			span->_objsize = 0;
			span->_next = span->_prev = nullptr;

			PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
		}

		start = next;
	}

}