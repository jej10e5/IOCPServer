#pragma once

template<typename T>
class Singleton
{
public:
	static T& GetInstance()
	{
		// static ������ �����ؼ� instance ������ �ѹ��� �ʱ�ȭ
		static T instance;
		return instance;
	}

protected:
	// default ������, �Ҹ��� ���
	Singleton() = default;
	virtual ~Singleton() = default;

	// ��ü�� �����ϰ� �ϳ��� �����Ǿ�� �ϹǷ� ����, ���� �����ڸ� Ȱ���� ������ ��Ȱ������
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
};

