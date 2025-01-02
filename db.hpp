#include <fe/Subsystem.hpp>
#include <fe/Events.hpp>
#include <fe/SingletonProcess.hpp>
#include <fe/Paths.hpp>
#include <fe/Close.hpp>
#include <fe/Setting.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <memory>
#include <filesystem>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class MongoDB: public fe::Subsystem {
    mongocxx::instance instance;
    std::unique_ptr<fe::SingletonProcess> proc;

    fe::Event<const bsoncxx::document::value &> logEvent;
    fe::Event<> startEvent;
    fe::Event<> stopEvent;
    fe::Listener<const std::string&> outHandler;

    std::string lockName;
    fs::path exePath;
    std::stringstream logmsg;

    std::jthread monitor;

    fe::Setting<bool> showLogs;
    fe::Setting<int> pingInterval;

    bool connected;
    
    void start();
    void start_monitor();
public:
    MongoDB();
    static fe::read_ptr<MongoDB> from_subsystem_name(std::string &name);
    void settingsSatisfiedHook();
    mongocxx::client make_client() const;
    mongocxx::database admin_db(mongocxx::client &client) const;
    const fe::Event<const bsoncxx::document::value &> & onLogMessageEvent() const;
    const fe::Event<> & onReadyEvent() const;
    const fe::Event<> & onStopEvent() const;
    const bool is_connected() const;
};