#pragma once

template<typename T>
class Singleton
{
public:
	static T& GetInstance()
	{
		// static 변수로 선언해서 instance 변수는 한번만 초기화
		static T instance;
		return instance;
	}

protected:
	// default 생성자, 소멸자 사용
	Singleton() = default;
	virtual ~Singleton() = default;

	// 객체는 유일하게 하나만 생성되어야 하므로 복사, 대입 연산자를 활용한 생성자 비활서오하
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
};

