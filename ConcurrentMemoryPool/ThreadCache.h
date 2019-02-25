#pragma once

#include "Common.h"
//thread cache��������һ����ϣӳ��Ķ�������������
class ThreadCache
{
public:
	// ������ͷ��ڴ����
	/*
	���ڴ�����size<=64kʱ��thread cache�������ڴ棬����size�����������е�λ�ã��������������
	���ڴ����ʱ��ֱ�Ӵ�FistList[i]��Popһ�¶���ʱ�临�Ӷ���O(1)����û����������
	��FreeList[i]��û�ж���ʱ����������central cache�л�ȡһ�������Ķ��󣬲��뵽������������һ
	������
	*/
	void* Allocate(size_t byte);

	// �����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t byte);

	/*
	���ͷ��ڴ�С��64kʱ���ڴ��ͷŻ�thread cache������size�����������е�λ�ã�������Push��
	FreeList[i].
	������ĳ��ȹ����������һ�����ڴ����central cache
	*/
	void Deallocate(void* ptr, size_t byte);

	// �ͷŶ���ʱ���������ʱ�������ڴ�ص����Ļ���
	void ListTooLong(FreeList* list, size_t byte);
private:
	FreeList _freelist[NLISTS];	//NLISTS����ô���루������򣩾�����
	//�߳���λ�ȡCache��
	//1��ȫ�־�̬�����̹߳���������Threadcache��������������������Ȼ��ÿ����һ���߳�id��
	//		Ҳ���ǿ�ʼ����Ϊ�գ���һ���̣߳�����һ��cache�������ӵ���������¼tid(ÿ���߳�tid����ͨ��һ���ӿڻ�ȡ�����磺thread_self()��)
	//�����߳������Ȳ��ң�����ͬtidʹ�ã�û�д���������.....�����ã��߳�һ������²�̫��
	//���Ǵ˴���ÿ���̻߳�ȡcache����Ҫ��������Ϊ�������ڣ�����ͬʱ�е��ڶ����ݣ��е���д����
	//�������ԣ�����������̫��
	/*
	int _tid;
	ThreadCache* _next;
	*/
	//2.���̵߳�ʱ�򣬼ǵ�tls��thread local storage�����̱߳��ش洢��ÿ���̶߳���һ��ȫ�ֱ����������⺯����������������Ǹ��̡߳�
	//�������ĵط����ڣ������ݣ���ͬƽ̨������Ҫ��ִ���
	//��̬�붯̬���𣺾�̬Ч�ʸ��ߣ������
	
};
static _declspec(thread) ThreadCache* tls_threadcache = nullptr;//_declspec(thread) ��̬tlsȫ�ֱ�����ÿ���̶߳��ܿ����������Ĳ���ͬһ��