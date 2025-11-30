/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>

namespace simple_dnsd {

class DnsdApp {
public:
    DnsdApp();
    ~DnsdApp();

    bool initialize(const std::string& config_file);
    bool start();
    void stop();
    void wait();

private:
    void run();
    
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::string config_file_;
};

} // namespace simple_dnsd
