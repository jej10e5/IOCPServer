#pragma once
// GameDispatcher.h
#include "../ServerCommon/pch.h"
#include <functional>
#include "Singleton.h"
class GameDispatcher : public Singleton<GameDispatcher>
{
public:
   // static GameDispatcher& Instance() { static GameDispatcher i; return i; }

    void Post(function<void()> f) {
        lock_guard<mutex> lock(m_mtx);
        m_q.push(move(f));
    }

    // 게임 로직 스레드에서 매 프레임(혹은 고정 주기) 호출
    void Drain(size_t maxJobs = 1000) {
        size_t n = 0;
        for (; n < maxJobs; ++n) {
            function<void()> job;
            {
                lock_guard<mutex> lock(m_mtx);
                if (m_q.empty()) break;
                job = move(m_q.front());
                m_q.pop();
            }
            job(); // 게임 로직 스레드에서 실행됨
        }
    }


    inline void PostToGameThread(std::function<void()> fn)
    {
        if (!fn)
        {
            return;
        }
        GameDispatcher::GetInstance().Post(std::move(fn));
    }



private:
    mutex m_mtx;
    queue<function<void()>> m_q;
};

