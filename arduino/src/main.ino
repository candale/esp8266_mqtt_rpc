#include <MQTTClient.h>


extern "C" {
    #include <mqtt.h>
}


class MyMqttClient: MQTTClient {
public:
    MyMqttClient(
            const char* device_id, const char* host,
            uint32_t port, uint32_t keep_alive):
            MQTTClient(device_id, host, port, keep_alive) {

    }

    void onData(String& topic, String& payload) {
        Serial.println(topic);
        Serial.println(payload);
        Serial.println("------------");
    }
};

MyMqttClient client("my_test_device", "mqtt.acandale.com", 1883, 6000);


void setup() {
    Serial.begin(115200);
}

void loop() {

}
