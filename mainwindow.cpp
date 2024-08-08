#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

struct Config {
    std::string folder;
    int age;
    int sleep;
};

std::vector<Config> readConfig(const std::string& filePath) {
    std::ifstream configFile(filePath);

    if (!configFile.is_open()) {
        throw std::runtime_error("Unable to open config file: " + filePath);
    }

    json configJson;
    try {
        configFile >> configJson;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }

    std::vector<Config> configs;
    for (const auto& item : configJson) {
        configs.push_back({ item["Folder"], item["Age"], item["Sleep"] });
    }

    return configs;
}

void cleanDirectory(const Config& config) {
    fs::path path(config.folder);
    std::string extension = path.extension().string();
    auto currentTime = std::chrono::system_clock::now();
    if (!fs::exists(path))
    {
        std::cout << "Directory does not exist: " << path << std::endl;
        return;
    }
    for (auto& p : fs::directory_iterator(path.parent_path()))
    {
        if (p.path().extension() == extension)
        {
            auto fileTime = fs::last_write_time(p.path());
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(fileTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            auto fileAge = std::chrono::duration_cast<std::chrono::hours>(currentTime - sctp).count() / 24;

            if (fileAge > config.age)
            {
                fs::remove(p.path());
                std::cout << "Deleted: " << p.path() << std::endl;
            }
        }
    }
}

void runCleaner(const std::string& configFilePath)
{
    auto configs = readConfig(configFilePath);

    std::vector<std::thread> threads;
    for (const auto& config : configs)
    {
        threads.emplace_back([&config]()
                             {
                                 cleanDirectory(config);
                                 std::this_thread::sleep_for(std::chrono::milliseconds(config.sleep));
                             });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    on_pushButton_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    const std::string configFilePath = "config.json";

    std::thread cleanerThread(runCleaner, configFilePath);
    cleanerThread.detach();

    std::cout << "File cleaner is running. Press Enter to exit." << std::endl;
    std::cin.get();

}
