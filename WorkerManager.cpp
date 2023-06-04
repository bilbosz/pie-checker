//
// Created by adam on 03.06.23.
//
#include "WorkerManager.h"

#include <iostream>
#include <mutex>
#include <thread>

WorkerManager::WorkerManager(size_t workerNum) : m_workers(workerNum) {
    for (auto &worker: m_workers) {
        worker = std::thread(&WorkerManager::Work, this);
    }
}

WorkerManager::~WorkerManager() {
    while (true) {
        std::lock_guard lock(m_fifoMutex);
        if (m_fifo.empty()) {
            m_finish = true;
            break;
        }
    }
    for (auto &worker: m_workers) {
        worker.join();
    }
}

void WorkerManager::PushFile(const std::string &file) {
    std::lock_guard lock(m_fifoMutex);
    m_fifo.push_back(file);
}

void WorkerManager::Work() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_fifoMutex);
        if (m_finish)
            break;
        if (!m_fifo.empty()) {
            auto file = m_fifo.front();
            m_fifo.pop_front();
            lock.unlock();
            Job(file);
            lock.lock();
        }
    }
}
