//
// Created by adam on 02.06.23.
//

#ifndef PIE_CHECKER_WORKERMANAGER_H
#define PIE_CHECKER_WORKERMANAGER_H

#include <deque>
#include <mutex>
#include <vector>
#include <thread>

#include "Job.h"

class WorkerManager {
public:
    WorkerManager(size_t workerNum);

    ~WorkerManager();

    void PushFile(const std::string &file);

private:
    void Work();

    std::vector<std::thread> m_workers;
    std::deque<std::string> m_fifo;
    bool m_finish = false;
    std::mutex m_fifoMutex;
};

#endif //PIE_CHECKER_WORKERMANAGER_H
