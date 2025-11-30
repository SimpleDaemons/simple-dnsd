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

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <signal.h>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "simple-dnsd/core/app.hpp"

using namespace simple_dnsd;

// Global application instance
std::unique_ptr<DnsdApp> g_app;

// Signal handler
void signal_handler(int signal) {
    if (g_app) {
        g_app->stop();
    }
}

// Daemonize function
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork daemon process" << std::endl;
        exit(1);
    }
    
    if (pid > 0) {
        // Parent process exits
        exit(0);
    }
    
    // Child process continues
    if (setsid() < 0) {
        std::cerr << "Failed to create new session" << std::endl;
        exit(1);
    }
    
    // Change working directory
    chdir("/");
    
    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Reopen standard file descriptors
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool daemon_mode = false;
    std::string config_file = "/etc/simple-dnsd/simple-dnsd.conf";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--daemon") {
            daemon_mode = true;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            }
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -d, --daemon     Run as daemon" << std::endl;
            std::cout << "  -c, --config     Configuration file path" << std::endl;
            std::cout << "  -h, --help       Show this help" << std::endl;
            return 0;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Daemonize if requested
    if (daemon_mode) {
        daemonize();
    }
    
    try {
        // Create and initialize application
        g_app = std::make_unique<DnsdApp>();
        
        if (!g_app->initialize(config_file)) {
            std::cerr << "Failed to initialize DNS daemon" << std::endl;
            return 1;
        }
        
        // Start the daemon
        if (!g_app->start()) {
            std::cerr << "Failed to start DNS daemon" << std::endl;
            return 1;
        }
        
        // Wait for shutdown signal
        g_app->wait();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
