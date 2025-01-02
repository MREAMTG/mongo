#include <fe/Subsystem.hpp>
#include "db.hpp"

class System: public fe::Subsystem {
    fe::read_ptr<MongoDB> mongo;
    std::string db_sub;
    fe::Listener<> dbReady;
public:
    System(std::string db): db_sub{db}, mongo{nullptr} {
        dbReady = fe::Listener<>{&System::setupDB, *this};
    }

    void setupDB() {
        fe::Logger::info("DB is ready!");

        auto client = mongo->make_client();
        auto db = mongo->admin_db(client);
        fe::Logger::success(db.name());
        auto coll = db.create_collection("joeshmo");
        bsoncxx::document::value document = bsoncxx::from_json("{\"boom\": 7}");
        coll.insert_one(document.view());
    }

    void settingsSatisfiedHook() {
        mongo = MongoDB::from_subsystem_name(db_sub);
        mongo->onReadyEvent().addListener(dbReady);
    }
};