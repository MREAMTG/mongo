#include "db.hpp"

static fs::path getHomeDir() {
    char* homeEnv = std::getenv("HOME");
    if (homeEnv)
        return fs::path{homeEnv};
    throw fe::RuntimeException("Error: Unable to get home directory on POSIX.");
}

static void log_mongo_severity(std::string_view msg, std::string_view severity) {
    if(severity == "F")
        fe::Logger::fatal(msg);
    else if(severity == "E")
        fe::Logger::error(msg);
    else if(severity == "W")
        fe::Logger::warning(msg);
    else if(severity == "I")
        fe::Logger::info(msg);
    else
        fe::Logger::debug(msg);
}

MongoDB::MongoDB():
proc{nullptr},
lockName{"mongodb"},
connected{false},
logEvent{"MongoLog"},
startEvent{"MongoStart"},
stopEvent{"MongoStop"},
exePath{fe::appVcpkgStaticFileDir() / "mongo" / "release" / "bin" / "mongod"} {
    addSetting("ShowLogs", showLogs.setDefault(true));
    addSetting("PingInterval", pingInterval.setDefault(1000));
    outHandler = [&](const std::string& msg) {
        fe::Logger::setThreadName("MongoOut");
        logmsg << msg;
        try {
            auto logstr = logmsg.str();
            bsoncxx::document::value doc_value = bsoncxx::from_json(logstr);
            logmsg.str("");
            logmsg.clear();
            logEvent.trigger(doc_value);
            bsoncxx::document::view doc = doc_value.view();
            auto message = doc["msg"].get_string().value;
            auto severity = doc["s"].get_string().value;

            if(showLogs.get())
                log_mongo_severity(message, severity);
            if(doc.find("attr") != doc.end()) {
                auto attr = bsoncxx::to_json(doc["attr"].get_document());
                if(showLogs.get())
                    log_mongo_severity(attr, severity);
            }
        } catch (...) {}
    };
    errHandler = [&](const std::string& msg) {
        fe::Logger::setThreadName("MongoErr");
        fe::Logger::error(msg);
    };
}

void MongoDB::settingsSatisfiedHook() {
    start();
    start_monitor();
}

fe::read_ptr<MongoDB> MongoDB::from_subsystem_name(std::string &name) {
    auto subs = fe::device->getSubsystems();
    auto sit = subs.find(name);
    if(sit == subs.end())
        throw fe::RuntimeException("Cannot find database subsystem: ", name);
    auto sub = std::dynamic_pointer_cast<MongoDB>(sit->second);
    if(sub == nullptr)
        throw fe::RuntimeException("Database subsystem specified is not MongoDB", name);
    return sub;
}

void MongoDB::start() {
    fs::path dataDir = getHomeDir() / ".fedata"/ ".mongodata";
    fs::create_directories(dataDir);

    std::vector<std::string> args{};
    args.push_back("--dbpath");
    args.push_back(dataDir.string());
    proc = std::make_unique<fe::SingletonProcess>(lockName, exePath, args);

    proc->getOutEvent().addListener(outHandler);
    proc->getErrEvent().addListener(errHandler);
    try {
        proc->start();
    } catch (const fe::AlreadyRunningException &ex) {}
}

mongocxx::client MongoDB::make_client() const {
    auto uri = mongocxx::uri{"mongodb://localhost:27017"};
    mongocxx::client client{uri};
    return client;
}

mongocxx::database MongoDB::admin_db(mongocxx::client &client) const {
    auto db = client.database("admin");
    return db;
}

void MongoDB::start_monitor() {
    monitor = std::jthread([this]() {
        fe::Logger::setThreadName("MongoMonitor");
        mongocxx::client client = make_client();
        while(fe::Sys.operate()) {
            try {
                auto db = client.database("admin");
                bsoncxx::builder::stream::document ping_cmd;
                ping_cmd << "ping" << 1;
                auto result = db.run_command(ping_cmd.view());
                if(!connected) startEvent.trigger();
                connected = true;
            } catch (...) {
                if(connected) stopEvent.trigger();
                connected = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(pingInterval.get()));
        }
    });
}

const fe::Event<const bsoncxx::document::value &> & MongoDB::onLogMessageEvent() const {
    return logEvent;
}

const fe::Event<> & MongoDB::onReadyEvent() const { return startEvent; }
const fe::Event<> & MongoDB::onStopEvent() const { return stopEvent; }

const bool MongoDB::is_connected() const { return connected; }