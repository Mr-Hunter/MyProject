#pragma once

#include "Common.h"

// 1.central cache��������һ����ϣӳ���span��������������
// 2.ÿ��ӳ���С��empty span����һ�������У�nonempty span����һ��������
// 3.Ϊ�˱�֤ȫ��ֻ��Ψһ��central cache������౻��Ƴ��˵���ģʽ
class CentralCache
{
public:
	static CentralCache* GetInstance(){
		return &_inst;
	}
	
	// �����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte);

	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t byte_size);

	// ��page cache��ȡһ��span
	Span* GetOneSpan(SpanList* spanlist, size_t bytes);
private:
	// ���Ļ�����������
	SpanList _spanlist[NLISTS];
private:
	//��ֹ���˴���������˽��
	CentralCache() = default;//����Ĭ�Ϲ��죬�������ɶ������Ĭ��
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;

	static CentralCache _inst; //.h�ļ��᱾����ļ����ã�������.cpp��ʼ�����������������⡣

};