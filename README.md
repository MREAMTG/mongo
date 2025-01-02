# mongo

MongoDB support for Factory Engine devices. Runs a `mongod` server instance as part of your device using the `MongoDB` subsystem, and includes libraries required to connect to and query the database from your C++ code using `mongo-cxx-driver`.

Currently supports MongoDB `v8.0.4`.

## Getting Started

### 1. Install  the `mongo` dependency

First, clone this repository and install this app as a dependency in your app project.
```
cd ~
git clone git@github.com:MREAMTG/mongo.git
cd mongo && fe build
cd <path/to/your/project>
fe install ~/mongo
```

### 2. Include the Subsystem in your Device

Next, include Mongo in your device by adding an instance of the `MongoDB` subsystem to your Device YAML file.

#### Example `MyDevice.yaml`
```yaml
subsystems:
    db: mongo.MongoDB
```

Now, when your device starts, an instance of `mongod` will be started and stopped in paralell with the rest of your device, since it is one of your subsystems.

This is great, but probably not all you need!

### 3. Access the Database

Access the database from one of your subsystems by doing something like this:

```cpp
#include <fe/Subsystem.hpp>
#include <apps/mongo/db.hpp>

class System: public fe::Subsystem {
    fe::read_ptr<MongoDB> mongo;
    fe::Listener<> dbReady;
public:
    System(std::string db): mongo{nullptr} {
        dbReady = fe::Listener<>{&System::setupDB, *this};
    }

    void setupDB() {
        fe::Logger::info("DB is ready!");

        // Set up a mongo client
        auto client = mongo->make_client();
        auto db = mongo->admin_db(client);

        // Create a collection
        auto coll = db.create_collection("mycollection");

        // Build and insert a document
        bsoncxx::document::value document = bsoncxx::from_json(
            "{\"boom\": 7}"
        );
        coll.insert_one(document.view());
    }

    void settingsSatisfiedHook() {
        // "db" is the name from our device YAML file
        // This will get your a read_ptr to the MongoDB subsystem
        mongo = MongoDB::from_subsystem_name("db");

        // Link the "onReady" event to our dbReady listener
        // This will run the database is finished starting
        mongo->onReadyEvent().addListener(dbReady);
    }
};
```

This will create a Mongo collection and insert a document into it when the device's settings are satisfied.

**Important note:** Do not (I repeat do not!) call `MongoDB::from_subsystem_name` in the constructor of your subsystem. It needs to be called after all subsystems are constructed (to ensure MongoDB subsystem is created). The most logical place call it and initialize your read_ptr is either in `settingsSatisfiedHook` or `deviceStartHook` (in your subsystem).

Don't forget to add your subsystem to your device YAML blueprint!

### 4. Run Your Device
If you create an instance of your device and start it, it will start a MongoDB instance, and should create a collection and publish a document.

You can validate this by downloading MongoDB compass, and manually connecting to the database running on `mongodb://localhost:27017`. By default, authentication should not be required.

And there you have it! MongoDB running in a Factory Engine device! Many more functions than `db.create_collection` and `collection.insert_one` are available in `mongo-cxx-driver`. You can find them all in the [public Mongo C++ Doxygen documentation](https://mongocxx.org/api/mongocxx-3.11.0/index.html).

## Tips
- Handle any database setup in a listener connected to the `MongoDB` subsystem's `onReady` event. This even will fire only when the database is actually online and ready to accept connections and queries
- Before making queries, if you want to be double sure that the database is up and ready, you can use `mongo.is_connected()` if you have a `read_ptr<MongoDB>` called `mongo` to check if the database online and the built in monitor heartbeat is connected

## Contributing
Many features are currently missing, including:

- `--auth` mode support
- Shortcuts for building queries and monitoring connection status
- Better logging support
- More built-in database setup shortcuts

Feel free to contribute or suggest any new features via a Pull Request. All suggestions & contributions are welcome!