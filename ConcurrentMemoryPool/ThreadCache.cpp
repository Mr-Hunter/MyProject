#include "ThreadCache.h"
#include "CentralCache.h"


//��һ�ַ�����ÿ�ζ��Ǵ�CentralCache��ȡ�̶������Ķ���10 ������̶�����������һ���������Զ����գ��������64kҲ�ǹ̶�������������ô�ͺܹ�����Ŷ��
//void* ThreadCache::FetchFromCentralCache(size_t index, size_t byte)
//{
//	FreeList* freelist = &_freelist[index];
//	size_t num = 10;
//
//	void* start, *end;
//	//����num�����Ĳ�һ���У���Ҫ��ȡʵ������ĸ������������ٱ�֤��һ��
//	size_t fetchnum = CentralCache::GetInstance()->FetchRangeObj(start, end, num, byte);
//	if (fetchnum == 1)
//		return start;
//	//����Ļ�������һ�����أ�����Ĺ�������������
//	freelist->PushRange(NEXT_OBJ(start), end, fetchnum - 1);
//	return start;
//}
//��������ÿ�λ�ȡʱ����ȡ�����������������Ĺ���
//�����ˮ���ܲ�һ����Ӧ����һ�����������Ĺ��̣�����Ӧ����һ���۷壬
//�����и����⣬����thread������2M�ͻ��գ����뼸��16�ֽڶ�����ô�⼸����Զ���ܻص�CentralCache,
//CentralCache��Ӧspan��Զ�ز���PageCache��Ҳ���ǲ��ܺϳɸ����ҳ��
//��������һ�����⣺����һ�����������Ľ�㣬��Ĺ��������Ա��´�ʹ�ã�����������˻�����ɿռ��˷����⣬
//����������ԭ����ǣ�������������ó��߳��������ſ�ʼ���������øô�С����Ͳ����룬�ڴ��þ�������࣬
//Ҳ�ͽ�����������ֽڴ�С����Ƶ�������ͷţ���Ҳ�����Զ����кϲ��ڿ��٣��ܺõļ���������Ƭ����
//��ǰ��ÿ���������Ĵ�С��Ŀ���ֽھ������������ӣ�

void* ThreadCache::FetchFromCentralCache(size_t index, size_t byte)
{
	FreeList* freelist = &_freelist[index];
	size_t num_to_move = ClassSize::BytesMoveNum(byte);

	num_to_move = min(num_to_move,freelist->MaxSize());	//С�������������࣬�ƶ��࣬8�ֽڣ�Ҳ���ƶ�512������������٣��ƶ�С��1024Ҳ���ƶ�64��
	void* start  = nullptr, *end = nullptr;
	size_t fetchnum = CentralCache::GetInstance()->FetchRangeObj(start, end, num_to_move, byte);
	if (fetchnum >1)
		freelist->PushRange(NEXT_OBJ(start), end, fetchnum - 1);

	//��������ԽƵ������ҪԽ����ô���������������
	//���Ҵﵽ512������������ֹͣ��
	if (num_to_move <= freelist->MaxSize())
	{
		freelist->SetMaxSize(num_to_move+1);
	}

	return start;
}

void* ThreadCache::Allocate(size_t byte)
{
	assert(byte <= MAXBYTES);

	//�����Զ���Ķ������
	// ���ݶ������size����ȡ��
	byte = ClassSize::Roundup(byte);
	//��ȡ��������Ӧ���±�
	size_t index = ClassSize::Index(byte);
	FreeList* freelist = &_freelist[index];
	if (!freelist->Empty())
	{
		//����ͷɾһ���ڵ㲢����
		return freelist->Pop();
	}
	else
	{
		//����Ϊ�գ���Centralache�л�ȡ����
		//ȡһ��������ÿ�ζ������������ģ����Զ�ȡһ�㣬��Ĺ��������ϣ��Ա��´�ʹ��
		return FetchFromCentralCache(index, byte);
	}
}

void ThreadCache::ListTooLong(FreeList* freelist, size_t byte)
{
	//�ֱ������ֱ������������,tͷָ�뽻������cache
	void* start = freelist->Clear();
	CentralCache::GetInstance()->ReleaseListToSpans(start,byte);
	

}
void ThreadCache::Deallocate(void* ptr, size_t byte)
{
	assert(byte <= MAXBYTES);
	size_t index = ClassSize::Index(byte);
	FreeList* freelist = &_freelist[index];
	freelist->Push(ptr);

	//���������������������һ�����������Ļ����ƶ������룩������ʱ����ʼ���л��ն�������cache
	if (freelist->Size() >= freelist->MaxSize())
	{
		//�ֱ���ȫ������:ȱ�ݣ��ڴ��þ�Ҫ�ٴ����룬�����������ӿ�����������Ϊ���ڵĺϲ��ڴ��������̵棬�������ڴ���Ƭ
		ListTooLong(freelist, byte);
	}

	//��Ŀ���㣺�ͷŵ��߼������Ը�ǿһЩ����threadcache��������ܵ��ֽ�������2M����ʼ�ͷ�
}