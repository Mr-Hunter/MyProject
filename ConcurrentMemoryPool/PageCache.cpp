#include "PageCache.h"

PageCache PageCache::_inst;

Span* PageCache::_NewSpan(size_t npage)
{
	//std::unique_lock<std::mutex> lock(_mutex);//�������ݹ�����Լ���ʱ���Ѿ���ס���Լ�������������ؿ�һ���ӽӿ�

	if (!_pagelist[npage].Empty())
	{
		return _pagelist[npage].PopFront();
	}

	for (size_t i = npage + 1; i < NPAGES; ++i)
	{
		SpanList* pagelist = &_pagelist[i];
		if (!pagelist->Empty())
		{
			Span* span = pagelist->PopFront();
			Span* split = new Span;
			split->_pageid = span->_pageid + span->_npage - npage;	//β��
			split->_npage = npage;
			span->_npage -= npage;

			_pagelist[span->_npage].PushFront(span);
			
			//����ӳ��,���ڻ���
			for (size_t i = 0; i < split->_npage; ++i)
			{
				_id_span_map[split->_pageid + i] = split;
			}

			return split;
		}
	}

	//�ڴ治��
	void* ptr = SystemAlloc(NPAGES-1);

	Span* largespan = new Span;
	largespan->_pageid = (PageID)ptr >> PAGE_SHIFT;
	largespan->_npage = NPAGES - 1;

	_pagelist[NPAGES - 1].PushFront(largespan);

	//����ӳ��,���ڻ���
	for (size_t i = 0; i < largespan->_npage; ++i)
	{
		_id_span_map[largespan->_pageid+i] = largespan;
	}

	Span* span = _NewSpan(npage);
	span->_objsize = npage << PAGE_SHIFT;
	return span;
}

Span* PageCache::NewSpan(size_t npage)
{
	std::unique_lock<std::mutex> lock(_mutex);//ԭ�������������ݹ�����Լ���ʱ���Ѿ���ס���Լ������ڵ��ԸĽ�
	
	if (npage >= NPAGES)
	{
		void* ptr = SystemAlloc(npage);

		//����ֱ�ӷ��أ��ҵ�ʱ���Ҳ����������ڴ��ʱ��֪�����
		Span* span = new Span;
		span->_pageid = (PageID)ptr >> PAGE_SHIFT;
		span->_npage = npage;

		span->_objsize = npage << PAGE_SHIFT;  //�����ܵ��ֽ���

		_id_span_map[span->_pageid] = span;
		return span;
	}
	
	return _NewSpan(npage);
}


//��ȡ�Ӷ���span��ӳ��
Span* PageCache::MapObjectToSpan(void* obj)
{
	PageID pageid = (PageID)obj >> PAGE_SHIFT;
	auto it = _id_span_map.find(pageid);
	assert(it != _id_span_map.end());  //�ϸ�˵�����ҵ������������

	return it->second;
}

// �ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
void PageCache::ReleaseSpanToPageCahce(Span* span)
{
	std::unique_lock<std::mutex> lock(_mutex);

	if (span->_npage >= NPAGES)
	{
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		_id_span_map.erase(span->_pageid);   //��ȥӳ��
		SystemFree(ptr);
		delete span;
		return;
	}

	auto previt = _id_span_map.find(span->_pageid - 1);
	while (previt != _id_span_map.end())
	{
		Span* prevspan = previt->second;
		//������
		if (prevspan->_usecount != 0)
			break;

		//����ϳ���NPAGES���򲻺ϲ�������û�취����,������һ��Сȱ��
		if (prevspan->_npage + span->_npage >= NPAGES)
			break;

		_pagelist[prevspan->_npage].Erase(prevspan);
		prevspan->_npage += span->_npage;
		delete span;
		span = prevspan;

		previt = _id_span_map.find(span->_pageid - 1);
	}

	auto nextit = _id_span_map.find(span->_pageid + span->_npage);
	while (nextit != _id_span_map.end())
	{
		Span* nextspan = nextit->second;
		if (nextspan->_usecount != 0)
			break;

		//����ϳ���NPAGES���򲻺ϲ�������û�취����,������һ��Сȱ��
		if (nextspan->_npage + span->_npage >= NPAGES)
			break;

		_pagelist[nextspan->_npage].Erase(nextspan);
		span->_npage += nextspan->_npage;
		delete nextspan;

		nextit = _id_span_map.find(span->_pageid + span->_npage);
	}

	//����ӳ��
	for (size_t i = 0; i < span->_npage; ++i)
	{
		_id_span_map[span->_pageid + i] = span;
	}

	_pagelist[span->_npage].PushFront(span);
}